#!/usr/bin/env python3
"""Minimal client for the Godot editor automation server.

Stage 1A speaks two methods: session.authenticate and status. The protocol is
newline-framed JSON-RPC 2.0 over a loopback TCP socket.
"""

import argparse
import glob
import json
import os
import socket
import sys

# status is answered synchronously inside the editor's frame poll, so a reply is
# normally instant. The headroom is for a busy editor, not for a slow command.
DEFAULT_TIMEOUT = 15.0

# The server caps a single message at 1 MiB and drops the connection past it.
MAX_MESSAGE_BYTES = 1 << 20


class AgentError(Exception):
    """A JSON-RPC error reply, or a frame the server could not be made to answer."""


def default_discovery_glob():
    """Every project's agent directory under the current user's app data.

    `user://` resolves per project, so a client looking across projects has to
    glob. Returns None when APPDATA is unset, which is every non-Windows host;
    the server is Windows-only today.
    """
    appdata = os.environ.get("APPDATA")
    if not appdata:
        return None
    return os.path.join(appdata, "Godot", "app_userdata", "*", "agent")


def normalize_project_path(path):
    """Canonical form for comparing two spellings of the same directory.

    The engine reports `project_path` exactly as it received it: forward slashes,
    a trailing slash, and on Windows often an 8.3 short name
    (`C:/Users/RUNNER~1.000/...`) when that is what the editor was launched with.
    A caller's own spelling of the same directory will differ in all three ways,
    so a raw string compare is always wrong. `realpath` expands the short name and
    fixes the separators, `normcase` folds the case, and the trailing separator is
    dropped so `C:\\probe` and `C:/probe/` compare equal.

    Short-name expansion only works while the directory still exists; compare
    before deleting a throwaway project, not after.
    """
    if not path:
        return ""
    try:
        resolved = os.path.realpath(path)
    except OSError:
        resolved = os.path.abspath(path)
    resolved = os.path.normcase(resolved)
    trimmed = resolved.rstrip("\\/")
    # Keep a bare drive or filesystem root intact rather than emptying it.
    return trimmed if trimmed else resolved


def same_project(left, right):
    """True when both paths name the same directory. Never a raw string compare."""
    if not left or not right:
        return False
    return normalize_project_path(left) == normalize_project_path(right)


def find_sessions(discovery_dir=None):
    """Returns every session dict found, newest first."""
    if discovery_dir is None:
        discovery_dir = default_discovery_glob()
        if discovery_dir is None:
            return []
    sessions = []
    for path in glob.glob(os.path.join(discovery_dir, "session-*.json")):
        try:
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
        except (OSError, ValueError):
            # A half-written or unreadable file is not a usable session.
            continue
        if not isinstance(data, dict) or "port" not in data or "token" not in data:
            continue
        data["_path"] = path
        sessions.append(data)
    sessions.sort(key=lambda s: s.get("started_at", 0), reverse=True)
    return sessions


class AgentClient:
    """One authenticated connection to an editor.

    The server serves a single client at a time, so the socket is closed even
    when authentication fails. Leaving a rejected connection open would lock the
    next client out until the editor noticed the drop.
    """

    def __init__(self, session, timeout=DEFAULT_TIMEOUT):
        self.session = session
        self.timeout = timeout
        self.sock = None
        self.buffer = b""
        self.next_id = 1

    def __enter__(self):
        self.sock = socket.create_connection(("127.0.0.1", self.session["port"]), timeout=self.timeout)
        try:
            self.call("session.authenticate", {"token": self.session["token"]})
        except BaseException:
            self.close()
            raise
        return self

    def __exit__(self, *exc):
        self.close()
        return False

    def close(self):
        if self.sock is not None:
            self.sock.close()
            self.sock = None

    def call(self, method, params=None):
        if self.sock is None:
            raise AgentError("not connected")

        request = {
            "jsonrpc": "2.0",
            # Always send an id. A request without one is a JSON-RPC notification
            # and the server answers nothing at all, which would hang the read.
            "id": self.next_id,
            "method": method,
            "params": params or {},
        }
        self.next_id += 1
        self.sock.sendall((json.dumps(request) + "\n").encode("utf-8"))

        while b"\n" not in self.buffer:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise AgentError("server closed the connection")
            self.buffer += chunk
            if len(self.buffer) > MAX_MESSAGE_BYTES:
                raise AgentError("reply exceeded the maximum message size without a newline")

        line, self.buffer = self.buffer.split(b"\n", 1)
        response = json.loads(line.decode("utf-8"))

        if "error" in response:
            err = response["error"] or {}
            # `data` is absent on parse errors (-32700) and invalid requests
            # (-32600), so it can never be indexed unconditionally.
            data = err.get("data")
            code = data.get("code", "UNKNOWN") if isinstance(data, dict) else "UNKNOWN"
            raise AgentError(f"{code}: {err.get('message')}")

        if response.get("id") != request["id"]:
            raise AgentError(f"reply id {response.get('id')!r} does not answer request {request['id']}")
        return response["result"]


def redacted(session):
    """A copy safe to print. The token is a live credential for this editor, and the
    protocol forbids leaving it in ordinary logs, which is where stdout ends up.
    """
    out = dict(session)
    if out.get("token"):
        out["token"] = "<redacted>"
    return out


def main(argv=None):
    parser = argparse.ArgumentParser(prog="godotctl", description="Talk to a running Godot editor agent server.")
    parser.add_argument("command", choices=["status", "sessions"])
    parser.add_argument("--session", help="session_id to target")
    parser.add_argument("--project", help="only sessions whose project_path names this directory")
    args = parser.parse_args(argv)

    sessions = find_sessions()
    if args.project:
        sessions = [s for s in sessions if same_project(s.get("project_path"), args.project)]

    if args.command == "sessions":
        print(json.dumps([redacted(s) for s in sessions], indent=2))
        return 0

    if args.session:
        sessions = [s for s in sessions if s.get("session_id") == args.session]
        if not sessions:
            print(f"session {args.session} not found", file=sys.stderr)
            return 3
    elif not sessions:
        print("no running editor with --agent-server found", file=sys.stderr)
        return 3
    elif len(sessions) > 1:
        # Never guess which editor the user meant.
        print("multiple sessions found; pass --session", file=sys.stderr)
        return 2

    try:
        with AgentClient(sessions[0]) as client:
            print(json.dumps(client.call("status"), indent=2))
    except (OSError, AgentError) as exc:
        print(str(exc), file=sys.stderr)
        return 3
    return 0


if __name__ == "__main__":
    sys.exit(main())
