/**************************************************************************/
/*  editor_agent_session.cpp                                              */
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
