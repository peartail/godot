/**************************************************************************/
/*  editor_agent_server.cpp                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "editor_agent_server.h"

#include "core/config/project_settings.h"
#include "core/io/json.h"
#include "core/math/math_funcs.h"
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

	// Godot's JSON reads every number as a double, so an id of 1 would echo back as
	// 1.0 and a strict client would not match it to its request. Narrow an integral
	// float back to an int; anything else (string, null, fractional) passes through.
	Variant id = req["id"];
	if (id.get_type() == Variant::FLOAT) {
		const double raw = id;
		if (raw == Math::floor(raw) && Math::abs(raw) < 9007199254740992.0) {
			id = (int64_t)raw;
		}
	}
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
