/**************************************************************************/
/*  editor_agent_server.h                                                 */
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
