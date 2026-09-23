/**************************************************************************/
/*  editor_agent_session.h                                                */
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
