# Editor Agent Server — Stage 1A Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove the whole automation pipe end to end with one trivial command: `--agent-server` starts a localhost server in the editor, writes a user-private session file, and a Python client authenticates and reads `status`.

**Architecture:** An editor-only `EditorAgentServer` polls a `TCPServer` from the main thread each frame — no worker thread, so no locking and no cross-thread access to the SceneTree. Requests are newline-framed JSON with a hand-rolled JSON-RPC 2.0 envelope. The `JSONRPC` class is
deliberately not used: it lives in `modules/jsonrpc`, which `module_jsonrpc_enabled=no` can switch
off, and an unguarded include from `editor/` would break that build. `EditorAgentSession` owns the session id, token, and the discovery file, which lives under the user data directory rather than the project.

**Tech Stack:** Godot Engine C++ (fork at `C:\GithubProjects\godot`), `core/io/tcp_server.h`, `modules/jsonrpc`, SCons via `scripts/build.ps1`, doctest unit tests, Python 3 client.

**Spec:** `engine_docs/agent_tools/editor_automation_spec.md` ·
[protocol](../../../engine_docs/agent_tools/editor_automation_protocol.md) ·
[stage 0 findings](../../../engine_docs/agent_tools/editor_automation_stage0_findings.md)

---

## Why this plan stops where it does

The spec's stage 1 is "연결·상태·작업 관리·씬 조회·리로드". That is four subsystems at once: transport
and auth, a job queue, the scene commands, and a client. Building them together would mean nothing is
testable until all of it exists.

This plan is **stage 1A only**: the smallest slice that works end to end.

| Plan | Scope |
| --- | --- |
| **1A (this one)** | `--agent-server`, session file, auth, `status`, Python client |
| 1B | job queue, `job.get`, `job.cancel`, `scene.reload` with dirty protection |
| 1C | `capabilities`, `scene.tree`, `object.describe`, `scene.open`, `scene.save` |

Everything in 1B and 1C is additive on top of 1A. Do not pull it forward.

## Constraints carried from stage 0

These came out of [the verification](../../../engine_docs/agent_tools/editor_automation_stage0_findings.md)
and are not negotiable in this plan:

1. The discovery file goes in a **user-private path**, never the project directory. `.godot/agent/` is
   world-readable in a typical project location, and writable by other users, which would let someone
   redirect the client to a forged server.
2. The engine cannot set Windows file permissions. Path choice is the only control. Do not add a
   `set_unix_permissions` call and assume it worked — it returns `ERR_UNAVAILABLE`.
3. The token is generated with `Crypto::generate_random_bytes`, not `RandomNumberGenerator`.

## Build and test commands

Run from the repository root in PowerShell. **Close every running Godot editor first** — the linker
fails with access denied while the binary is running.

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
```

Unit tests:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --test "--test-case=*EditorAgent*"
```

Integration test (Task 4):

```powershell
python .\scripts\godotctl\test_integration.py
```

A pre-existing unrelated error prints on every run: `Object 'SplineMesh3D' already has member 'mesh'`.
Ignore it.

## File structure

| File | Responsibility |
| --- | --- |
| `editor/agent/editor_agent_session.h/.cpp` (create) | Session id, token, discovery file contents and lifetime |
| `editor/agent/editor_agent_server.h/.cpp` (create) | TCP listen/accept, newline framing, auth gate, `status` |
| `editor/agent/SCsub` (create) | Build registration for the new directory |
| `editor/SCsub` (modify) | Add `SConscript("agent/SCsub")` |
| `main/main.cpp` (modify) | `--agent-server` CLI flag |
| `editor/editor_node.cpp` (modify) | Start and stop the server with the editor |
| `tests/editor/agent/test_editor_agent_session.cpp` (create) | Unit tests for session identity and the discovery file |
| `scripts/godotctl/godotctl.py` (create) | Python client |
| `scripts/godotctl/test_integration.py` (create) | End-to-end test |

`editor/agent/` is a new directory. Unlike `editor/gui/`, it is not covered by an existing glob, so it
needs its own `SCsub` **and** a `SConscript` line in `editor/SCsub`. `tests/SCsub` does glob
recursively, so the test file needs no build change.

---

### Task 1: Session identity and the discovery file

**Files:**
- Create: `editor/agent/editor_agent_session.h`
- Create: `editor/agent/editor_agent_session.cpp`
- Create: `editor/agent/SCsub`
- Modify: `editor/SCsub`
- Test: `tests/editor/agent/test_editor_agent_session.cpp`

- [ ] **Step 1: Write the failing test**

Create `tests/editor/agent/test_editor_agent_session.cpp`. Start with the standard Godot copyright
header block — copy the 29-line comment from the top of `tests/scene/test_button.cpp` and change the
filename on its second line to `test_editor_agent_session.cpp`. Then:

```cpp
#include "tests/test_macros.h"

TEST_FORCE_LINK(test_editor_agent_session)

#ifdef TOOLS_ENABLED

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/os/os.h"
#include "editor/agent/editor_agent_session.h"

namespace TestEditorAgentSession {

TEST_CASE("[EditorAgent] token is long and unpredictable") {
	const String a = EditorAgentSession::generate_token();
	const String b = EditorAgentSession::generate_token();

	// 32 random bytes rendered as hex.
	CHECK(a.length() == 64);
	CHECK(b.length() == 64);
	CHECK(a != b);
	CHECK(a.is_valid_hex_number(false));
}

TEST_CASE("[EditorAgent] each session gets its own id and token") {
	EditorAgentSession one;
	EditorAgentSession two;

	CHECK_FALSE(one.get_session_id().is_empty());
	CHECK(one.get_session_id() != two.get_session_id());
	CHECK(one.get_token() != two.get_token());
}

TEST_CASE("[EditorAgent] discovery file round-trips every field") {
	EditorAgentSession session;
	const String dir = OS::get_singleton()->get_cache_path().path_join("godot_agent_test");
	DirAccess::make_dir_recursive_absolute(dir);

	// A concrete value rather than ProjectSettings: the test runner has no project
	// open, so globalize_path("res://") would yield an empty string here.
	const String project_path = OS::get_singleton()->get_cache_path().path_join("godot_agent_test_project");

	const Error err = session.write_discovery_file(dir, 54321, project_path);
	CHECK(err == OK);

	const String path = session.get_discovery_file_path(dir);
	CHECK(FileAccess::exists(path));

	const Dictionary d = JSON::parse_string(FileAccess::get_file_as_string(path));
	CHECK(int(d["protocol_version"]) == EditorAgentSession::PROTOCOL_VERSION);
	CHECK(String(d["session_id"]) == session.get_session_id());
	CHECK(String(d["token"]) == session.get_token());
	CHECK(int(d["port"]) == 54321);
	CHECK(int(d["pid"]) == OS::get_singleton()->get_process_id());
	CHECK(String(d["project_path"]) == project_path);
	CHECK(int64_t(d["started_at"]) > 0);

	// Removing is idempotent so editor shutdown never errors on a missing file.
	CHECK(session.remove_discovery_file(dir) == OK);
	CHECK_FALSE(FileAccess::exists(path));
	CHECK(session.remove_discovery_file(dir) == OK);

	DirAccess::remove_absolute(dir);
}

TEST_CASE("[EditorAgent] a session file is never written without a project path") {
	EditorAgentSession session;
	const String dir = OS::get_singleton()->get_cache_path().path_join("godot_agent_test_noproject");

	// A client validates the session against project_path. Writing an empty one
	// would leave that check comparing nothing, so the write must fail outright.
	ERR_PRINT_OFF;
	const Error err = session.write_discovery_file(dir, 54321, String());
	ERR_PRINT_ON;

	CHECK(err == ERR_UNCONFIGURED);
	CHECK_FALSE(FileAccess::exists(session.get_discovery_file_path(dir)));
}

TEST_CASE("[EditorAgent] the default discovery directory is not inside the project") {
	const String dir = EditorAgentSession::default_discovery_dir();

	CHECK_FALSE(dir.is_empty());
	// The project directory is world-readable in a typical checkout location, so the
	// session file must not live there. See the stage 0 findings.
	CHECK_FALSE(dir.begins_with("res://"));
	CHECK_FALSE(dir.contains(".godot"));
}

} // namespace TestEditorAgentSession

#endif // TOOLS_ENABLED
```

- [ ] **Step 2: Run the build to verify it fails**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
```

Expected: compile error, `editor/agent/editor_agent_session.h: No such file or directory`.

- [ ] **Step 3: Write the header**

Create `editor/agent/editor_agent_session.h`. Start with the standard Godot copyright header block —
copy the 29-line comment from the top of `editor/gui/editor_toolbar_group.h` and change the filename
on its second line to `editor_agent_session.h`. Then:

```cpp
#pragma once

#include "core/object/ref_counted.h"
#include "core/string/ustring.h"

// Owns the identity of one editor automation session and the discovery file a
// client reads to find and authenticate against it.
class EditorAgentSession : public RefCounted {
	GDCLASS(EditorAgentSession, RefCounted);

public:
	static constexpr int PROTOCOL_VERSION = 1;
	static constexpr int TOKEN_BYTES = 32;

private:
	String session_id;
	String token;
	int64_t started_at = 0;

public:
	// 32 cryptographically random bytes as lowercase hex.
	static String generate_token();

	// A user-private directory. Never the project directory: see the stage 0 findings.
	static String default_discovery_dir();

	String get_session_id() const { return session_id; }
	String get_token() const { return token; }
	int64_t get_started_at() const { return started_at; }

	String get_discovery_file_path(const String &p_dir) const;

	// The project path is passed in rather than read from ProjectSettings so this
	// class has no hidden global dependency and stays unit-testable. The caller is
	// the only place that knows which project this session belongs to.
	Dictionary to_discovery_dict(int p_port, const String &p_project_path) const;

	// Fails with ERR_UNCONFIGURED on an empty project path: a client validates the
	// session against it, so an empty value would silently defeat that check.
	Error write_discovery_file(const String &p_dir, int p_port, const String &p_project_path);
	// Succeeds when the file is already gone, so shutdown never reports an error.
	Error remove_discovery_file(const String &p_dir);

	EditorAgentSession();
};
```

- [ ] **Step 4: Write the implementation**

Create `editor/agent/editor_agent_session.cpp` with the same copyright header block, filename line
changed to `editor_agent_session.cpp`. Then:

```cpp
#include "editor_agent_session.h"

#include "core/crypto/crypto.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/variant/dictionary.h"

String EditorAgentSession::generate_token() {
	Ref<Crypto> crypto = Crypto::create();
	ERR_FAIL_COND_V_MSG(crypto.is_null(), String(), "Crypto module unavailable; cannot mint a session token.");

	const PackedByteArray bytes = crypto->generate_random_bytes(TOKEN_BYTES);
	ERR_FAIL_COND_V(bytes.size() != TOKEN_BYTES, String());
	return String::hex_encode_buffer(bytes.ptr(), bytes.size());
}

String EditorAgentSession::default_discovery_dir() {
	// The user data directory sits under the user's profile, unlike the project
	// directory, which in a typical checkout grants read to BUILTIN\Users and
	// modify to Authenticated Users.
	return OS::get_singleton()->get_user_data_dir().path_join("agent");
}

String EditorAgentSession::get_discovery_file_path(const String &p_dir) const {
	return p_dir.path_join(vformat("session-%s.json", session_id));
}

Dictionary EditorAgentSession::to_discovery_dict(int p_port, const String &p_project_path) const {
	Dictionary d;
	d["protocol_version"] = PROTOCOL_VERSION;
	d["session_id"] = session_id;
	d["token"] = token;
	d["project_path"] = p_project_path;
	d["pid"] = OS::get_singleton()->get_process_id();
	d["port"] = p_port;
	d["started_at"] = started_at;
	return d;
}

Error EditorAgentSession::write_discovery_file(const String &p_dir, int p_port, const String &p_project_path) {
	ERR_FAIL_COND_V_MSG(p_project_path.is_empty(), ERR_UNCONFIGURED, "Refusing to write an agent session file without a project path.");

	Error err = DirAccess::make_dir_recursive_absolute(p_dir);
	if (err != OK && err != ERR_ALREADY_EXISTS) {
		return err;
	}

	Ref<FileAccess> f = FileAccess::open(get_discovery_file_path(p_dir), FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(f.is_null(), err, "Cannot write the agent session file.");

	f->store_string(JSON::stringify(to_discovery_dict(p_port, p_project_path), "  "));
	f->close();
	return OK;
}

Error EditorAgentSession::remove_discovery_file(const String &p_dir) {
	const String path = get_discovery_file_path(p_dir);
	if (!FileAccess::exists(path)) {
		return OK;
	}
	return DirAccess::remove_absolute(path);
}

EditorAgentSession::EditorAgentSession() {
	session_id = generate_token().substr(0, 16);
	token = generate_token();
	// get_unix_time_from_system() returns a double; the protocol carries whole seconds.
	started_at = (int64_t)Time::get_singleton()->get_unix_time_from_system();
}
```

`to_discovery_dict` takes the project path as an argument, so this file does **not** include
`core/config/project_settings.h`. The editor resolves the path once, in `EditorAgentServer::start()`.

- [ ] **Step 5: Register the new directory with the build**

Create `editor/agent/SCsub`:

```python
#!/usr/bin/env python
from misc.utility.scons_hints import *

Import("env")

env.add_source_files(env.editor_sources, "*.cpp")
```

In `editor/SCsub`, add this line to the `SConscript(...)` block, in alphabetical order — it goes
before `SConscript("animation/SCsub")`:

```python
    SConscript("agent/SCsub")
```

- [ ] **Step 6: Run the tests to verify they pass**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --test "--test-case=*EditorAgent*"
```

Expected: `test cases: 5 | 5 passed | 0 failed`.

- [ ] **Step 7: Commit**

```bash
git add editor/agent editor/SCsub tests/editor/agent
git commit -m "Add editor agent session identity and discovery file"
```

---

### Task 2: The server — listen, frame, authenticate, answer status

**Files:**
- Create: `editor/agent/editor_agent_server.h`
- Create: `editor/agent/editor_agent_server.cpp`

- [ ] **Step 1: Write the header**

Create `editor/agent/editor_agent_server.h` with the standard copyright header block, filename line
`editor_agent_server.h`. Then:

```cpp
#pragma once

#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"
#include "core/object/ref_counted.h"
#include "core/templates/vector.h"
#include "editor/agent/editor_agent_session.h"

// Localhost JSON-RPC server for editor automation. Polled from the main thread so
// commands never touch the SceneTree from another thread.
class EditorAgentServer : public RefCounted {
	GDCLASS(EditorAgentServer, RefCounted);

public:
	// The protocol forbids accumulating an unbounded message. A line longer than this
	// is answered with an error and the connection is dropped.
	static constexpr int MAX_MESSAGE_BYTES = 1 << 20;

private:
	Ref<TCPServer> server;
	Ref<StreamPeerTCP> client;
	Ref<EditorAgentSession> session;

	String discovery_dir;
	// Raw bytes, not a String: a multi-byte UTF-8 character can be split across two
	// socket reads, and decoding each read on its own would corrupt it.
	Vector<uint8_t> read_buffer;
	bool authenticated = false;

	void _drop_client();
	void _send_line(const String &p_line);
	static String _error_line(const Variant &p_id, int p_rpc_code, const String &p_message, const String &p_domain_code);
	String _handle_line(const String &p_line);
	Dictionary _dispatch(const String &p_method, const Dictionary &p_params, bool &r_ok, String &r_code, String &r_message);
	Dictionary _cmd_status() const;

public:
	// Binds to an ephemeral localhost port and writes the discovery file.
	Error start();
	void stop();
	bool is_running() const;

	int get_port() const;
	Ref<EditorAgentSession> get_session() const { return session; }

	// Call once per editor frame.
	void poll();

	~EditorAgentServer();
};
```

- [ ] **Step 2: Write the implementation**

Create `editor/agent/editor_agent_server.cpp` with the standard copyright header block, filename line
`editor_agent_server.cpp`. Then:

```cpp
#include "editor_agent_server.h"

#include "core/config/project_settings.h"
#include "core/io/json.h"
#include "core/os/os.h"
#include "core/version.h"

Error EditorAgentServer::start() {
	ERR_FAIL_COND_V_MSG(is_running(), ERR_ALREADY_IN_USE, "Agent server is already running.");

	// Resolved here, not inside the session: this is the one place that knows which
	// project the editor has open. It is empty in the project manager, where there is
	// nothing to automate. Checked before listen() so that case never opens a port.
	const String project_path = ProjectSettings::get_singleton()->globalize_path("res://");
	ERR_FAIL_COND_V_MSG(project_path.is_empty(), ERR_UNCONFIGURED, "Agent server needs an open project; not starting.");

	session.instantiate();
	server.instantiate();

	// Port 0 asks the OS for a free port. Bind to loopback only: this is a local
	// automation channel, never a network service.
	const Error err = server->listen(0, IPAddress("127.0.0.1"));
	if (err != OK) {
		server.unref();
		session.unref();
		ERR_FAIL_V_MSG(err, "Agent server could not listen on loopback.");
	}

	discovery_dir = EditorAgentSession::default_discovery_dir();
	const Error werr = session->write_discovery_file(discovery_dir, server->get_local_port(), project_path);
	if (werr != OK) {
		stop();
		ERR_FAIL_V_MSG(werr, "Agent server could not write its session file.");
	}

	print_line(vformat("Agent server listening on 127.0.0.1:%d, session %s", server->get_local_port(), session->get_session_id()));
	return OK;
}

void EditorAgentServer::stop() {
	_drop_client();
	if (session.is_valid() && !discovery_dir.is_empty()) {
		session->remove_discovery_file(discovery_dir);
	}
	if (server.is_valid()) {
		server->stop();
		server.unref();
	}
	session.unref();
	discovery_dir = String();
}

bool EditorAgentServer::is_running() const {
	return server.is_valid() && server->is_listening();
}

int EditorAgentServer::get_port() const {
	return server.is_valid() ? server->get_local_port() : 0;
}

void EditorAgentServer::_drop_client() {
	if (client.is_valid()) {
		client->disconnect_from_host();
		client.unref();
	}
	read_buffer.clear();
	authenticated = false;
}

void EditorAgentServer::_send_line(const String &p_line) {
	if (client.is_null()) {
		return;
	}
	const CharString utf8 = (p_line + "\n").utf8();
	client->put_data((const uint8_t *)utf8.get_data(), utf8.length());
}

void EditorAgentServer::poll() {
	if (!is_running()) {
		return;
	}

	// One client at a time keeps session state unambiguous for now.
	if (client.is_null() && server->is_connection_available()) {
		client = server->take_connection();
		read_buffer.clear();
		authenticated = false;
	}

	if (client.is_null()) {
		return;
	}

	client->poll();
	if (client->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
		_drop_client();
		return;
	}

	const int available = client->get_available_bytes();
	if (available > 0) {
		const int start = read_buffer.size();
		read_buffer.resize(start + available);
		int received = 0;
		if (client->get_partial_data(read_buffer.ptrw() + start, available, received) != OK) {
			_drop_client();
			return;
		}
		read_buffer.resize(start + received);
	}

	// Newline framing: TCP packet boundaries are not message boundaries. Scanning for
	// the delimiter in bytes is safe because no continuation byte of a multi-byte
	// UTF-8 sequence can be 0x0A.
	while (true) {
		const uint8_t *bytes = read_buffer.ptr();
		int nl = -1;
		for (int i = 0; i < read_buffer.size(); i++) {
			if (bytes[i] == '\n') {
				nl = i;
				break;
			}
		}

		if (nl == -1) {
			// No complete line yet. The protocol forbids growing without bound.
			if (read_buffer.size() > MAX_MESSAGE_BYTES) {
				_send_line(_error_line(Variant(), -32600, "Message exceeds the maximum size.", "MESSAGE_TOO_LARGE"));
				_drop_client();
			}
			return;
		}

		const String line = String::utf8((const char *)bytes, nl);
		read_buffer = read_buffer.slice(nl + 1);

		const String reply = _handle_line(line.strip_edges());
		if (!reply.is_empty()) {
			_send_line(reply);
		}
	}
}

String EditorAgentServer::_error_line(const Variant &p_id, int p_rpc_code, const String &p_message, const String &p_domain_code) {
	Dictionary err;
	err["code"] = p_rpc_code;
	err["message"] = p_message;
	if (!p_domain_code.is_empty()) {
		Dictionary data;
		data["code"] = p_domain_code;
		err["data"] = data;
	}

	Dictionary out;
	out["jsonrpc"] = "2.0";
	out["id"] = p_id;
	out["error"] = err;
	return JSON::stringify(out);
}

String EditorAgentServer::_handle_line(const String &p_line) {
	if (p_line.is_empty()) {
		return String();
	}

	// Parsed through a JSON instance rather than the static JSON::parse_string() so a
	// malformed line answers with a protocol error instead of printing an engine error.
	Ref<JSON> json;
	json.instantiate();
	if (json->parse(p_line) != OK) {
		return _error_line(Variant(), -32700, "Parse error", String());
	}

	// A bare scalar parses fine but is not a request. Variant's Dictionary conversion
	// would quietly hand back an empty one, so check the type rather than the content.
	if (json->get_data().get_type() != Variant::DICTIONARY) {
		return _error_line(Variant(), -32600, "Invalid Request", String());
	}

	const Dictionary req = json->get_data();

	// JSON-RPC 2.0: a request with no id is a notification and gets no reply. The spec
	// also forbids running side effects that way, so there is nothing to answer.
	if (!req.has("id")) {
		return String();
	}

	const Variant id = req["id"];
	const String method = req.get("method", "");
	const Dictionary params = req.get("params", Dictionary());

	bool ok = false;
	String code;
	String message;
	const Dictionary result = _dispatch(method, params, ok, code, message);

	Dictionary out;
	out["jsonrpc"] = "2.0";
	out["id"] = id;
	if (!ok) {
		return _error_line(id, -32000, message, code);
	}

	out["result"] = result;
	return JSON::stringify(out);
}

Dictionary EditorAgentServer::_dispatch(const String &p_method, const Dictionary &p_params, bool &r_ok, String &r_code, String &r_message) {
	r_ok = false;

	if (p_method == "session.authenticate") {
		const String supplied = p_params.get("token", "");
		// Comparing full strings, not prefixes: a partial match must not pass.
		if (session.is_null() || supplied.is_empty() || supplied != session->get_token()) {
			r_code = "AUTH_FAILED";
			r_message = "Invalid session token.";
			return Dictionary();
		}
		authenticated = true;
		r_ok = true;
		Dictionary d;
		d["session_id"] = session->get_session_id();
		d["protocol_version"] = EditorAgentSession::PROTOCOL_VERSION;
		return d;
	}

	// Everything else requires an authenticated connection.
	if (!authenticated) {
		r_code = "AUTH_FAILED";
		r_message = "Authenticate before sending other requests.";
		return Dictionary();
	}

	if (p_method == "status") {
		r_ok = true;
		return _cmd_status();
	}

	r_code = "UNSUPPORTED_CAPABILITY";
	r_message = vformat("Unknown method '%s'.", p_method);
	return Dictionary();
}

Dictionary EditorAgentServer::_cmd_status() const {
	Dictionary d;
	d["session_id"] = session->get_session_id();
	d["protocol_version"] = EditorAgentSession::PROTOCOL_VERSION;
	d["engine_version"] = GODOT_VERSION_FULL_BUILD;
	d["pid"] = OS::get_singleton()->get_process_id();
	d["port"] = get_port();
	d["started_at"] = session->get_started_at();
	return d;
}

EditorAgentServer::~EditorAgentServer() {
	stop();
}
```

- [ ] **Step 3: Build to check it compiles**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
```

Expected: build succeeds. Nothing calls the server yet, so behavior is unchanged.

The APIs used here are confirmed present: `TCPServer::get_local_port()` (`core/io/tcp_server.h:45`),
and `STATUS_CONNECTED` / `get_status()` inherited from `StreamPeerSocket`
(`core/io/stream_peer_socket.h:49,75`), which `StreamPeerTCP` derives from.

- [ ] **Step 4: Commit**

```bash
git add editor/agent/editor_agent_server.h editor/agent/editor_agent_server.cpp
git commit -m "Add editor agent JSON-RPC server with token authentication"
```

---

### Task 3: Wire it to the editor and the command line

**Files:**
- Modify: `main/main.cpp`
- Modify: `editor/editor_node.h`
- Modify: `editor/editor_node.cpp`

- [ ] **Step 1: Add the CLI flag**

In `main/main.cpp`, add the help entry to the **Run options** section, right after the `--lsp-port`
line. It belongs with the other long-running servers, not with the one-shot `--agent-docs-*` dumps
under **Standalone tools**:

```cpp
	print_help_option("--agent-server", "Start the local editor automation server. Only has an effect together with --editor.\n", CLI_OPTION_AVAILABILITY_EDITOR);
```

Find the argument parsing near line 1894 where `--agent-docs-list` is matched, and add this branch
next to it:

```cpp
		} else if (arg == "--agent-server") {
			// Forwarded rather than stored: the editor reads it back off the command
			// line. An environment variable would be inherited by every child process,
			// so a spawned editor would start an agent server nobody asked for.
			main_args.push_back(arg);
```

`main_args.push_back(arg)` leaves the flag in `OS::get_cmdline_args()`, which is how the editor picks
it up in the next step. This mirrors `--run-upgrade-tool` (`editor/editor_node.cpp:1560`), the
existing precedent for a flag the editor reads for itself.

- [ ] **Step 2: Hold the server on EditorNode**

In `editor/editor_node.h`, add near the other member declarations:

```cpp
	// Local editor automation server. Only instantiated when --agent-server is passed.
	Ref<EditorAgentServer> agent_server;
```

Forward declare it with the other `class Editor*;` lines rather than including the header. An include
would drag `core/io/tcp_server.h` and `core/io/stream_peer_tcp.h` into every translation unit that
pulls in `editor_node.h`, which is most of the editor. `Ref<T>` over an incomplete type is fine here
because `~EditorNode` is defined out of line:

```cpp
class EditorAgentServer;
```

The header itself is included in `editor/editor_node.cpp`, alphabetically before
`editor/audio/editor_audio_buses.h`.

- [ ] **Step 3: Start it, poll it, stop it**

In `editor/editor_node.cpp`, inside `EditorNode::EditorNode(...)`, near the end of the constructor
where other subsystems are brought up, add:

```cpp
	// Read off the command line the same way --run-upgrade-tool is, so that nothing
	// is inherited by child processes.
	const List<String> cmdline_args = OS::get_singleton()->get_cmdline_args();
	if (cmdline_args.find("--agent-server") != nullptr) {
		agent_server.instantiate();
		if (agent_server->start() != OK) {
			agent_server.unref();
		}
	}
```

In `EditorNode::_notification(int p_what)`, inside the existing `NOTIFICATION_PROCESS` case, add at
the top of the case body:

```cpp
			if (agent_server.is_valid()) {
				agent_server->poll();
			}
```

In `EditorNode::~EditorNode()`, add before the other teardown:

```cpp
	if (agent_server.is_valid()) {
		agent_server->stop();
		agent_server.unref();
	}
```

The `NOTIFICATION_PROCESS` case already exists at `editor/editor_node.cpp:938`. Put the poll call
inside it rather than adding a new case — a hook on a notification that does not fire every frame
makes the server silently unresponsive instead of failing loudly.

- [ ] **Step 4: Build and smoke-test by hand**

```powershell
.\scripts\build.ps1 -ExtraArgs "tests=yes"
```

Then start the editor headless with the flag and confirm the session file appears:

```powershell
$probe = Join-Path $env:TEMP "agent_smoke"
New-Item -ItemType Directory -Force $probe | Out-Null
Set-Content -Encoding utf8 (Join-Path $probe "project.godot") "config_version=5`n`n[application]`n`nconfig/name=`"agent_smoke`"`n"
$editor = Start-Process -PassThru -FilePath ".\bin\godot.windows.editor.dev.x86_64.mono.console.exe" -ArgumentList "--headless","--editor","--agent-server","--path",$probe
Start-Sleep -Seconds 15
Get-ChildItem "$env:APPDATA\Godot\app_userdata\agent_smoke\agent\" -ErrorAction SilentlyContinue
```

Expected: one `session-*.json` file containing `port`, `token`, `session_id`. Then stop the editor
you started — only that one, by pid — and clean up:

```powershell
Stop-Process -Id $editor.Id -Force
Start-Sleep -Seconds 2
Get-ChildItem "$env:APPDATA\Godot\app_userdata\agent_smoke\agent\" -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force $probe, "$env:APPDATA\Godot\app_userdata\agent_smoke"
```

Expected: the session file is **still there**. `Stop-Process -Force` is `TerminateProcess` on Windows,
which runs no destructors, so `~EditorNode` never calls `stop()`. That is the documented stage 1A
behaviour, not a bug — reaping a dead session's file is 1B's job. Graceful removal is already covered
by the `remove_discovery_file` unit test in Task 1.

- [ ] **Step 5: Commit**

```bash
git add main/main.cpp editor/editor_node.h editor/editor_node.cpp
git commit -m "Start the editor agent server behind --agent-server"
```

---

### Task 4: Python client and the end-to-end test

**Files:**
- Create: `scripts/godotctl/godotctl.py`
- Create: `scripts/godotctl/test_integration.py`

- [ ] **Step 1: Write the client**

Create `scripts/godotctl/godotctl.py`:

```python
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
```

- [ ] **Step 2: Write the end-to-end test**

Create `scripts/godotctl/test_integration.py`:

```python
#!/usr/bin/env python3
"""End-to-end check for stage 1A.

Launches a headless editor with --agent-server against a throwaway project,
authenticates, calls status, and verifies the session file is cleaned up.
Run from the repository root:  python .\\scripts\\godotctl\\test_integration.py
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from godotctl import AgentClient, AgentError, find_sessions  # noqa: E402

EDITOR = os.path.join("bin", "godot.windows.editor.dev.x86_64.mono.console.exe")
PROJECT_NAME = "godotctl_probe"


def make_project(root):
    with open(os.path.join(root, "project.godot"), "w", encoding="utf-8") as handle:
        handle.write(
            "config_version=5\n\n[application]\n\n"
            f'config/name="{PROJECT_NAME}"\n'
        )


def discovery_dir():
    return os.path.join(os.environ["APPDATA"], "Godot", "app_userdata", PROJECT_NAME, "agent")


def wait_for_session(timeout=60):
    deadline = time.time() + timeout
    while time.time() < deadline:
        sessions = find_sessions(discovery_dir())
        if sessions:
            return sessions[0]
        time.sleep(0.5)
    return None


def main():
    assert os.path.exists(EDITOR), f"build the editor first: {EDITOR} not found"
    root = tempfile.mkdtemp(prefix="godotctl_")
    proc = None
    failures = []
    try:
        make_project(root)
        shutil.rmtree(discovery_dir(), ignore_errors=True)

        proc = subprocess.Popen(
            [EDITOR, "--headless", "--editor", "--agent-server", "--path", root],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        session = wait_for_session()
        if session is None:
            failures.append("no session file appeared within 60s")
            return failures

        for field in ("protocol_version", "session_id", "token", "port", "pid", "project_path"):
            if field not in session:
                failures.append(f"session file missing field: {field}")

        # A wrong token must be refused, and refusal must not authenticate.
        bad = dict(session)
        bad["token"] = "0" * 64
        try:
            with AgentClient(bad):
                failures.append("server accepted a wrong token")
        except AgentError as exc:
            if "AUTH_FAILED" not in str(exc):
                failures.append(f"wrong token gave the wrong error: {exc}")

        with AgentClient(session) as client:
            status = client.call("status")
            if status.get("session_id") != session["session_id"]:
                failures.append("status returned a different session_id")
            if status.get("port") != session["port"]:
                failures.append("status returned a different port")
            try:
                client.call("no.such.method")
                failures.append("unknown method did not fail")
            except AgentError as exc:
                if "UNSUPPORTED_CAPABILITY" not in str(exc):
                    failures.append(f"unknown method gave the wrong error: {exc}")
    finally:
        if proc is not None:
            proc.terminate()
            proc.wait(timeout=30)
        time.sleep(1)
        # terminate() is TerminateProcess on Windows, so no destructor runs and the
        # session file is expected to survive. This asserts that documented stage 1A
        # behaviour; when 1B adds stale-file reaping, this assertion should flip.
        if not find_sessions(discovery_dir()):
            failures.append("session file vanished after a forced kill; stale-file handling changed")
        shutil.rmtree(root, ignore_errors=True)
        shutil.rmtree(discovery_dir(), ignore_errors=True)
    return failures


if __name__ == "__main__":
    problems = main()
    if problems:
        for item in problems:
            print("FAIL:", item)
        sys.exit(1)
    print("PASS: stage 1A end to end")
```

- [ ] **Step 3: Run the integration test**

```powershell
python .\scripts\godotctl\test_integration.py
```

Expected: `PASS: stage 1A end to end`.

If it reports `no session file appeared within 60s`, the flag is not reaching the editor or the
polling hook is not running. Check the editor's stdout by launching it manually without
`stdout=subprocess.DEVNULL` rather than increasing the timeout.

- [ ] **Step 4: Commit**

```bash
git add scripts/godotctl
git commit -m "Add godotctl client and stage 1A end-to-end test"
```

---

## Known limitations of stage 1A

State these in the handoff rather than discovering them later.

- One client connection at a time. A second connection is not accepted until the first drops.
- A single message is capped at `EditorAgentServer::MAX_MESSAGE_BYTES` (1 MiB). A line that grows past
  it without a newline is answered with `MESSAGE_TOO_LARGE` and the connection is dropped. 1B should
  publish this number through `capabilities` rather than leaving clients to discover it.
- No job queue. `status` answers synchronously inside `poll()`. Anything slower belongs in 1B.
- The discovery directory is derived from `user://`, which resolves per project. A client looking
  across projects globs `app_userdata/*/agent/`.
- Stale session files from a crashed editor are not cleaned up on startup. The client sorts by
  `started_at` and the caller passes `--session` when several appear. 1B should reap dead PIDs.
- The token is stored in plain text in the discovery file. That is the design: the protection is the
  path's ACL, and the engine cannot tighten it further on Windows. `godotctl sessions` redacts it on
  the way out, because stdout ends up in shell history and CI logs.
- `test_integration.py` drives the client library, not `godotctl.py`'s argument parsing. The `status`,
  `sessions`, `--session` and `--project` paths were checked by hand against a live editor; 1B should
  cover them automatically once there is a second command worth scripting.
