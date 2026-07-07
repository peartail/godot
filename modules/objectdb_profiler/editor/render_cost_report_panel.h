/**************************************************************************/
/*  render_cost_report_panel.h                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "core/templates/hash_map.h"
#include "scene/gui/box_container.h"

class Button;
class EditorFileDialog;
class RichTextLabel;
class SpinBox;
class Tree;

class RenderCostReportPanel : public VBoxContainer {
	GDCLASS(RenderCostReportPanel, VBoxContainer);

	static constexpr int MAX_FRAME_HISTORY = 10000;

	enum ExportFormat {
		EXPORT_JSON,
		EXPORT_MARKDOWN,
	};

	struct FrameSample {
		int frame_index = 0;
		PackedFloat32Array values;
	};

	Vector<FrameSample> frames;
	Array monitor_names;
	PackedInt32Array monitor_types;
	HashMap<StringName, int> monitor_indices;
	int next_frame_index = 0;
	bool capture_enabled = false;
	ExportFormat pending_export_format = EXPORT_JSON;

	Tree *spike_tree = nullptr;
	RichTextLabel *summary = nullptr;
	SpinBox *threshold_ms = nullptr;
	SpinBox *range_from = nullptr;
	SpinBox *range_to = nullptr;
	Button *clear_button = nullptr;
	Button *export_json_button = nullptr;
	Button *export_markdown_button = nullptr;
	EditorFileDialog *file_dialog = nullptr;

	int _get_monitor_index(const StringName &p_name) const;
	double _get_value(const FrameSample &p_frame, const StringName &p_name) const;
	double _get_pipeline_total(const FrameSample &p_frame) const;
	double _get_frame_time_ms(const FrameSample &p_frame) const;
	int _get_first_selected_frame() const;
	int _get_last_selected_frame() const;
	String _format_mb(double p_bytes) const;
	Array _build_recommendations(int p_from, int p_to) const;
	Dictionary _build_report_data(int p_from, int p_to) const;
	String _build_markdown_report(int p_from, int p_to) const;
	void _update_range_controls();
	void _refresh_report();
	void _clear_history();
	void _export_json_pressed();
	void _export_markdown_pressed();
	void _file_selected(const String &p_path);

public:
	RenderCostReportPanel();

	void set_capture_enabled(bool p_enabled);
	void update_monitor_names(const Array &p_names, const PackedInt32Array &p_types);
	void add_profile_frame(const PackedFloat32Array &p_values);
};