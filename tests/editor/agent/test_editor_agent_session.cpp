/**************************************************************************/
/*  test_editor_agent_session.cpp                                         */
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
