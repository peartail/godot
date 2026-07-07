/**************************************************************************/
/*  runtime_diagnostics_panel.h                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "scene/gui/control.h"

class ObjectDBProfilerPanel;
class RenderCostReportPanel;
class TabContainer;

class RuntimeDiagnosticsPanel : public Control {
	GDCLASS(RuntimeDiagnosticsPanel, Control);

	TabContainer *tabs = nullptr;
	ObjectDBProfilerPanel *memory_snapshots = nullptr;
	RenderCostReportPanel *render_cost = nullptr;

public:
	RuntimeDiagnosticsPanel();

	bool handle_debug_message(const String &p_message, const Array &p_data, int p_index);
	void set_debugger_active(bool p_active);
	void debugger_breaked(bool p_can_debug);
	void update_monitor_names(const Array &p_names, const PackedInt32Array &p_types);
	void add_profile_frame(const PackedFloat32Array &p_values);
};