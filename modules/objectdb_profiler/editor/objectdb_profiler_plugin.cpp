/**************************************************************************/
/*  objectdb_profiler_plugin.cpp                                          */
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

#include "objectdb_profiler_plugin.h"

#include "runtime_diagnostics_panel.h"

#include "core/input/shortcut.h"
#include "core/object/callable_mp.h"

bool ObjectDBProfilerDebuggerPlugin::has_capture(const String &p_capture) const {
	return p_capture == "snapshot";
}

bool ObjectDBProfilerDebuggerPlugin::capture(const String &p_message, const Array &p_data, int p_index) {
	ERR_FAIL_NULL_V(runtime_diagnostics, false);
	return runtime_diagnostics->handle_debug_message(p_message, p_data, p_index);
}

void ObjectDBProfilerDebuggerPlugin::setup_session(int p_session_id) {
	Ref<EditorDebuggerSession> session = get_session(p_session_id);
	ERR_FAIL_COND(session.is_null());
	ERR_FAIL_NULL(runtime_diagnostics);
	session->connect("started", callable_mp(runtime_diagnostics, &RuntimeDiagnosticsPanel::set_debugger_active).bind(true));
	session->connect("stopped", callable_mp(runtime_diagnostics, &RuntimeDiagnosticsPanel::set_debugger_active).bind(false));
	session->connect("breaked", callable_mp(runtime_diagnostics, &RuntimeDiagnosticsPanel::debugger_breaked));
	session->connect("performance_profile_names", callable_mp(runtime_diagnostics, &RuntimeDiagnosticsPanel::update_monitor_names));
	session->connect("performance_profile_frame", callable_mp(runtime_diagnostics, &RuntimeDiagnosticsPanel::add_profile_frame));
}

ObjectDBProfilerPlugin::ObjectDBProfilerPlugin() {
	debugger.instantiate();
}

void ObjectDBProfilerPlugin::_notification(int p_what) {
	switch (p_what) {
		case Node::NOTIFICATION_ENTER_TREE: {
			runtime_diagnostics = memnew(RuntimeDiagnosticsPanel);
			debugger->set_runtime_diagnostics_panel(runtime_diagnostics);
			add_control_to_bottom_panel(runtime_diagnostics, TTRC("Runtime Diagnostics"));
			add_debugger_plugin(debugger);
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			remove_debugger_plugin(debugger);
			if (runtime_diagnostics) {
				remove_control_from_bottom_panel(runtime_diagnostics);
				memdelete(runtime_diagnostics);
				runtime_diagnostics = nullptr;
			}
			debugger->set_runtime_diagnostics_panel(nullptr);
		}
	}
}