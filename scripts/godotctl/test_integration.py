#!/usr/bin/env python3
"""End-to-end check for stage 1A.

Launches a headless editor with --agent-server against a throwaway project,
authenticates, calls status, and verifies the session file survives the kill.
Run it from anywhere:  python scripts/godotctl/test_integration.py
"""

import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from godotctl import AgentClient, AgentError, find_sessions, same_project  # noqa: E402

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EDITOR = os.path.join(REPO_ROOT, "bin", "godot.windows.editor.dev.x86_64.mono.console.exe")
PROJECT_NAME = "godotctl_probe"
PROTOCOL_VERSION = 1

# The editor takes about twenty seconds to reach the first frame on a dev build,
# and much longer on a cold filesystem cache. Poll for the file, never sleep.
SESSION_TIMEOUT = 120

# Exactly what the engine writes today. A missing key breaks every client.
REQUIRED_FIELDS = ("pid", "port", "project_path", "protocol_version", "session_id", "started_at", "token")


def make_project(root):
    with open(os.path.join(root, "project.godot"), "w", encoding="utf-8") as handle:
        handle.write(f'config_version=5\n\n[application]\n\nconfig/name="{PROJECT_NAME}"\n')


def user_data_dir():
    """Where `user://` lands for the probe project."""
    return os.path.join(os.environ["APPDATA"], "Godot", "app_userdata", PROJECT_NAME)


def discovery_dir():
    return os.path.join(user_data_dir(), "agent")


def read_tail(path, lines=40):
    try:
        with open(path, encoding="utf-8", errors="replace") as handle:
            return "".join(handle.readlines()[-lines:]).strip()
    except OSError as exc:
        return f"(could not read {path}: {exc})"


def kill_pid(pid):
    """TerminateProcess by pid.

    `os.kill` on Windows is TerminateProcess for every signal that is not a
    console control event, which is what stage 1A's teardown expects: no
    destructors run, so the session file is left behind on purpose.
    """
    try:
        os.kill(pid, signal.SIGTERM)
        return True
    except OSError:
        pass
    if os.name != "nt":
        return False
    # taskkill can reach a process os.kill could not open, and /T takes the tree.
    completed = subprocess.run(
        ["taskkill", "/PID", str(pid), "/T", "/F"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return completed.returncode == 0


def shutdown(proc, session):
    """Stops the editor without leaving an orphan.

    `.console.exe` is a launcher wrapper: the real editor runs as a different
    pid, so killing the Popen handle alone leaves the editor running and holding
    the port. The pid in the session file is the one that matters; the wrapper
    exits by itself once its child is gone.
    """
    if session is not None:
        pid = session.get("pid")
        if isinstance(pid, int) and pid > 0 and pid != proc.pid:
            kill_pid(pid)

    try:
        proc.wait(timeout=15)
        return
    except subprocess.TimeoutExpired:
        proc.kill()

    try:
        proc.wait(timeout=15)
    except subprocess.TimeoutExpired:
        # Last resort; never hang the test on a wrapper that will not die.
        kill_pid(proc.pid)


def wait_for_session(proc, stale_paths, timeout=SESSION_TIMEOUT):
    """Polls for a session file this run produced. Gives up early if the editor exits."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        for session in find_sessions(discovery_dir()):
            if session.get("_path") not in stale_paths:
                return session
        if proc.poll() is not None:
            # The wrapper exits when the editor does, so there is nothing coming.
            return None
        time.sleep(0.5)
    return None


def check_session_file(session, project_root):
    failures = []
    for field in REQUIRED_FIELDS:
        if field not in session:
            failures.append(f"session file missing field: {field}")
    if session.get("protocol_version") != PROTOCOL_VERSION:
        failures.append(f"protocol_version is {session.get('protocol_version')!r}, expected {PROTOCOL_VERSION}")
    # Compared through same_project, not ==: the engine echoes the path it was
    # given, with forward slashes, a trailing slash and an 8.3 short name.
    if not same_project(session.get("project_path"), project_root):
        failures.append(f"project_path {session.get('project_path')!r} is not the probe project {project_root!r}")
    return failures


def check_wrong_token_is_refused(session):
    """A wrong token must be refused, and refusal must not authenticate."""
    bad = dict(session)
    bad["token"] = "0" * 64
    try:
        with AgentClient(bad):
            return ["server accepted a wrong token"]
    except AgentError as exc:
        if "AUTH_FAILED" not in str(exc):
            return [f"wrong token gave the wrong error: {exc}"]
    except OSError as exc:
        return [f"could not reach the server with a wrong token: {exc}"]
    return []


def check_authenticated_calls(session):
    failures = []
    with AgentClient(session) as client:
        status = client.call("status")
        if status.get("session_id") != session["session_id"]:
            failures.append("status returned a different session_id")
        if status.get("port") != session["port"]:
            failures.append("status returned a different port")
        if status.get("pid") != session["pid"]:
            failures.append("status returned a different pid")
        if status.get("protocol_version") != PROTOCOL_VERSION:
            failures.append(f"status protocol_version is {status.get('protocol_version')!r}")
        if not status.get("engine_version"):
            failures.append("status did not report an engine_version")

        try:
            client.call("no.such.method")
            failures.append("unknown method did not fail")
        except AgentError as exc:
            if "UNSUPPORTED_CAPABILITY" not in str(exc):
                failures.append(f"unknown method gave the wrong error: {exc}")
    return failures


def main():
    if not os.path.exists(EDITOR):
        return [f"build the editor first: {EDITOR} not found"]

    root = tempfile.mkdtemp(prefix="godotctl_")
    log_fd, log_path = tempfile.mkstemp(prefix="godotctl_", suffix=".log")
    failures = []
    proc = None
    session = None
    try:
        make_project(root)
        # A run that was killed can leave a session file behind. Clear what we
        # can, and remember the rest so it is never mistaken for this run's.
        shutil.rmtree(discovery_dir(), ignore_errors=True)
        stale_paths = {s.get("_path") for s in find_sessions(discovery_dir())}

        proc = subprocess.Popen(
            [EDITOR, "--headless", "--editor", "--agent-server", "--path", root],
            stdout=log_fd,
            stderr=subprocess.STDOUT,
        )
        os.close(log_fd)
        log_fd = -1

        session = wait_for_session(proc, stale_paths)
        if session is None:
            failures.append(f"no session file appeared within {SESSION_TIMEOUT}s")
        else:
            failures += check_session_file(session, root)
            failures += check_wrong_token_is_refused(session)
            failures += check_authenticated_calls(session)
    except (OSError, AgentError) as exc:
        failures.append(f"unexpected failure: {type(exc).__name__}: {exc}")
    finally:
        if log_fd != -1:
            os.close(log_fd)
        if proc is not None:
            shutdown(proc, session)
        if session is not None:
            time.sleep(1)
            # A forced kill runs no destructors, so ~EditorNode never calls
            # stop() and the file stays. That is the documented stage 1A
            # behavior; when 1B reaps stale sessions, flip this assertion.
            if not find_sessions(discovery_dir()):
                failures.append("session file vanished after a forced kill; stale-file handling changed")
        if failures:
            print("--- editor output (tail) ---", file=sys.stderr)
            print(read_tail(log_path), file=sys.stderr)
            print("--- end editor output ---", file=sys.stderr)
        shutil.rmtree(root, ignore_errors=True)
        shutil.rmtree(user_data_dir(), ignore_errors=True)
        try:
            os.unlink(log_path)
        except OSError:
            pass
    return failures


if __name__ == "__main__":
    problems = main()
    if problems:
        for item in problems:
            print("FAIL:", item)
        sys.exit(1)
    print("PASS: stage 1A end to end")
