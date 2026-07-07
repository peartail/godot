/**************************************************************************/
/*  runtime_diagnostics_panel.cpp                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "runtime_diagnostics_panel.h"

#include "objectdb_profiler_panel.h"
#include "render_cost_report_panel.h"

#include "scene/gui/tab_container.h"

RuntimeDiagnosticsPanel::RuntimeDiagnosticsPanel() {
	set_name(TTRC("Runtime Diagnostics"));
	set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

	tabs = memnew(TabContainer);
	tabs->set_anchors_preset(LayoutPreset::PRESET_FULL_RECT);
	tabs->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	tabs->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	add_child(tabs);

	memory_snapshots = memnew(ObjectDBProfilerPanel);
	memory_snapshots->set_name(TTRC("Memory Snapshots"));
	tabs->add_child(memory_snapshots);

	render_cost = memnew(RenderCostReportPanel);
	tabs->add_child(render_cost);

	set_debugger_active(false);
}

bool RuntimeDiagnosticsPanel::handle_debug_message(const String &p_message, const Array &p_data, int p_index) {
	return memory_snapshots->handle_debug_message(p_message, p_data, p_index);
}

void RuntimeDiagnosticsPanel::set_debugger_active(bool p_active) {
	memory_snapshots->set_enabled(p_active);
	render_cost->set_capture_enabled(p_active);
}

void RuntimeDiagnosticsPanel::debugger_breaked(bool p_can_debug) {
	memory_snapshots->debugger_breaked(p_can_debug);
}

void RuntimeDiagnosticsPanel::update_monitor_names(const Array &p_names, const PackedInt32Array &p_types) {
	render_cost->update_monitor_names(p_names, p_types);
}

void RuntimeDiagnosticsPanel::add_profile_frame(const PackedFloat32Array &p_values) {
	render_cost->add_profile_frame(p_values);
}