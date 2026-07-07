/**************************************************************************/
/*  render_cost_report_panel.cpp                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "render_cost_report_panel.h"

#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/object/callable_mp.h"
#include "editor/gui/editor_file_dialog.h"
#include "main/performance.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tree.h"

int RenderCostReportPanel::_get_monitor_index(const StringName &p_name) const {
	const int *index = monitor_indices.getptr(p_name);
	return index == nullptr ? -1 : *index;
}

double RenderCostReportPanel::_get_value(const FrameSample &p_frame, const StringName &p_name) const {
	const int index = _get_monitor_index(p_name);
	if (index < 0 || index >= p_frame.values.size()) {
		return 0.0;
	}
	return p_frame.values[index];
}

double RenderCostReportPanel::_get_pipeline_total(const FrameSample &p_frame) const {
	Performance *perf = Performance::get_singleton();
	return _get_value(p_frame, perf->get_monitor_name(Performance::PIPELINE_COMPILATIONS_CANVAS)) +
			_get_value(p_frame, perf->get_monitor_name(Performance::PIPELINE_COMPILATIONS_MESH)) +
			_get_value(p_frame, perf->get_monitor_name(Performance::PIPELINE_COMPILATIONS_SURFACE)) +
			_get_value(p_frame, perf->get_monitor_name(Performance::PIPELINE_COMPILATIONS_DRAW)) +
			_get_value(p_frame, perf->get_monitor_name(Performance::PIPELINE_COMPILATIONS_SPECIALIZATION));
}

double RenderCostReportPanel::_get_frame_time_ms(const FrameSample &p_frame) const {
	Performance *perf = Performance::get_singleton();
	double process_time = _get_value(p_frame, perf->get_monitor_name(Performance::TIME_PROCESS));
	if (process_time > 0.0) {
		return process_time * 1000.0;
	}
	double fps = _get_value(p_frame, perf->get_monitor_name(Performance::TIME_FPS));
	return fps > 0.0 ? 1000.0 / fps : 0.0;
}

int RenderCostReportPanel::_get_first_selected_frame() const {
	return CLAMP((int)range_from->get_value(), 0, MAX(0, next_frame_index - 1));
}

int RenderCostReportPanel::_get_last_selected_frame() const {
	return CLAMP((int)range_to->get_value(), _get_first_selected_frame(), MAX(0, next_frame_index - 1));
}

String RenderCostReportPanel::_format_mb(double p_bytes) const {
	return String::num(p_bytes / (1024.0 * 1024.0), 2) + " MiB";
}

Array RenderCostReportPanel::_build_recommendations(int p_from, int p_to) const {
	Array recommendations;
	if (frames.is_empty()) {
		return recommendations;
	}

	Performance *perf = Performance::get_singleton();
	const StringName draw_calls_name = perf->get_monitor_name(Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME);
	const StringName primitives_name = perf->get_monitor_name(Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME);
	const StringName texture_mem_name = perf->get_monitor_name(Performance::RENDER_TEXTURE_MEM_USED);
	const StringName buffer_mem_name = perf->get_monitor_name(Performance::RENDER_BUFFER_MEM_USED);
	const StringName video_mem_name = perf->get_monitor_name(Performance::RENDER_VIDEO_MEM_USED);
	const StringName object_count_name = perf->get_monitor_name(Performance::OBJECT_COUNT);
	const StringName node_count_name = perf->get_monitor_name(Performance::OBJECT_NODE_COUNT);
	const StringName resource_count_name = perf->get_monitor_name(Performance::OBJECT_RESOURCE_COUNT);

	const FrameSample *first = nullptr;
	const FrameSample *last = nullptr;
	double worst_frame_ms = 0.0;
	for (const FrameSample &frame : frames) {
		if (frame.frame_index < p_from || frame.frame_index > p_to) {
			continue;
		}
		if (first == nullptr) {
			first = &frame;
		}
		last = &frame;
		worst_frame_ms = MAX(worst_frame_ms, _get_frame_time_ms(frame));
	}
	if (first == nullptr || last == nullptr) {
		return recommendations;
	}

	if (worst_frame_ms >= threshold_ms->get_value()) {
		recommendations.push_back(vformat(TTR("Frame time exceeded the spike threshold. Worst frame: %s ms."), String::num(worst_frame_ms, 2)));
	}
	if (_get_value(*last, draw_calls_name) > _get_value(*first, draw_calls_name) * 1.25 + 100.0) {
		recommendations.push_back(TTR("Draw calls increased noticeably. Check batching, material variety, and visible object count."));
	}
	if (_get_value(*last, primitives_name) > _get_value(*first, primitives_name) * 1.25 + 10000.0) {
		recommendations.push_back(TTR("Primitive count increased noticeably. Check mesh density, LOD, and culling."));
	}
	if (_get_pipeline_total(*last) > _get_pipeline_total(*first)) {
		recommendations.push_back(TTR("Pipeline compilation occurred in the selected range. Consider warming shaders before gameplay."));
	}
	if (_get_value(*last, texture_mem_name) > _get_value(*first, texture_mem_name) + 8.0 * 1024.0 * 1024.0 ||
			_get_value(*last, buffer_mem_name) > _get_value(*first, buffer_mem_name) + 8.0 * 1024.0 * 1024.0 ||
			_get_value(*last, video_mem_name) > _get_value(*first, video_mem_name) + 8.0 * 1024.0 * 1024.0) {
		recommendations.push_back(TTR("GPU memory counters grew in the selected range. Check newly loaded textures, meshes, buffers, and render targets."));
	}
	if (_get_value(*last, object_count_name) > _get_value(*first, object_count_name) + 100.0 ||
			_get_value(*last, node_count_name) > _get_value(*first, node_count_name) + 50.0 ||
			_get_value(*last, resource_count_name) > _get_value(*first, resource_count_name) + 25.0) {
		recommendations.push_back(TTR("Object, node, or resource counts grew noticeably. Pair this report with a Runtime Memory Snapshot diff."));
	}
	if (recommendations.is_empty()) {
		recommendations.push_back(TTR("No obvious render-cost pattern was detected with the MVP rules."));
	}
	return recommendations;
}

Dictionary RenderCostReportPanel::_build_report_data(int p_from, int p_to) const {
	Dictionary report;
	Dictionary metadata;
	metadata["history_size"] = frames.size();
	metadata["range_from"] = p_from;
	metadata["range_to"] = p_to;
	metadata["spike_threshold_ms"] = threshold_ms->get_value();
	report["metadata"] = metadata;
	report["monitor_names"] = monitor_names;

	Array samples;
	Array spikes;
	for (int i = 0; i < frames.size(); i++) {
		const FrameSample &frame = frames[i];
		if (frame.frame_index < p_from || frame.frame_index > p_to) {
			continue;
		}
		Dictionary sample;
		sample["frame"] = frame.frame_index;
		sample["frame_time_ms"] = _get_frame_time_ms(frame);
		Array values;
		for (int j = 0; j < frame.values.size(); j++) {
			values.push_back(frame.values[j]);
		}
		sample["values"] = values;
		samples.push_back(sample);
		if (_get_frame_time_ms(frame) >= threshold_ms->get_value()) {
			spikes.push_back(sample);
		}
	}
	report["samples"] = samples;
	report["spikes"] = spikes;
	report["recommendations"] = _build_recommendations(p_from, p_to);
	return report;
}

String RenderCostReportPanel::_build_markdown_report(int p_from, int p_to) const {
	Dictionary report = _build_report_data(p_from, p_to);
	Array samples = report["samples"];
	Array spikes = report["spikes"];
	Array recommendations = report["recommendations"];

	double total_ms = 0.0;
	double min_ms = samples.is_empty() ? 0.0 : 1e20;
	double max_ms = 0.0;
	for (const Variant &sample_var : samples) {
		Dictionary sample = sample_var;
		double frame_ms = sample["frame_time_ms"];
		total_ms += frame_ms;
		min_ms = MIN(min_ms, frame_ms);
		max_ms = MAX(max_ms, frame_ms);
	}
	String markdown;
	markdown += "# Render Cost Report\n\n";
	markdown += vformat("- Frame range: %d-%d\n", p_from, p_to);
	markdown += vformat("- Samples: %d\n", samples.size());
	markdown += vformat("- Average frame time: %s ms\n", samples.is_empty() ? String("N/A") : String::num(total_ms / samples.size(), 2));
	markdown += vformat("- Min frame time: %s ms\n", String::num(min_ms, 2));
	markdown += vformat("- Max frame time: %s ms\n", String::num(max_ms, 2));
	markdown += vformat("- Spikes: %d\n\n", spikes.size());
	markdown += "## Recommendations\n\n";
	for (const Variant &recommendation : recommendations) {
		markdown += "- " + String(recommendation) + "\n";
	}
	markdown += "\n## Worst Frames\n\n";
	markdown += "| Frame | Frame Time ms |\n| --- | ---: |\n";
	int emitted = 0;
	for (const Variant &spike_var : spikes) {
		Dictionary spike = spike_var;
		markdown += vformat("| %d | %s |\n", (int)spike["frame"], String::num((double)spike["frame_time_ms"], 2));
		emitted++;
		if (emitted >= 20) {
			break;
		}
	}
	return markdown;
}

void RenderCostReportPanel::_update_range_controls() {
	const int max_frame = MAX(0, next_frame_index - 1);
	range_from->set_max(max_frame);
	range_to->set_max(max_frame);
	if (range_to->get_value() < range_from->get_value()) {
		range_to->set_value(range_from->get_value());
	}
	if (!frames.is_empty()) {
		range_to->set_value(max_frame);
	}
}

void RenderCostReportPanel::_refresh_report() {
	spike_tree->clear();
	TreeItem *root = spike_tree->create_item();

	const int from = _get_first_selected_frame();
	const int to = _get_last_selected_frame();
	Performance *perf = Performance::get_singleton();
	const StringName draw_calls_name = perf->get_monitor_name(Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME);
	const StringName primitives_name = perf->get_monitor_name(Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME);
	const StringName objects_name = perf->get_monitor_name(Performance::RENDER_TOTAL_OBJECTS_IN_FRAME);
	const StringName texture_mem_name = perf->get_monitor_name(Performance::RENDER_TEXTURE_MEM_USED);
	const StringName buffer_mem_name = perf->get_monitor_name(Performance::RENDER_BUFFER_MEM_USED);
	const StringName video_mem_name = perf->get_monitor_name(Performance::RENDER_VIDEO_MEM_USED);

	int samples = 0;
	int spikes = 0;
	double total_ms = 0.0;
	double max_ms = 0.0;
	for (int i = 0; i < frames.size(); i++) {
		const FrameSample &frame = frames[i];
		if (frame.frame_index < from || frame.frame_index > to) {
			continue;
		}
		double frame_ms = _get_frame_time_ms(frame);
		total_ms += frame_ms;
		max_ms = MAX(max_ms, frame_ms);
		samples++;
		if (frame_ms < threshold_ms->get_value()) {
			continue;
		}
		spikes++;
		TreeItem *item = spike_tree->create_item(root);
		item->set_text(0, itos(frame.frame_index));
		item->set_text(1, String::num(frame_ms, 2));
		item->set_text(2, itos((int)_get_value(frame, draw_calls_name)));
		item->set_text(3, itos((int)_get_value(frame, primitives_name)));
		item->set_text(4, itos((int)_get_value(frame, objects_name)));
		item->set_text(5, _format_mb(_get_value(frame, texture_mem_name)));
		item->set_text(6, _format_mb(_get_value(frame, buffer_mem_name)));
		item->set_text(7, _format_mb(_get_value(frame, video_mem_name)));
		const double previous_pipeline_total = i > 0 ? _get_pipeline_total(frames[i - 1]) : _get_pipeline_total(frame);
		item->set_text(8, itos((int)(_get_pipeline_total(frame) - previous_pipeline_total)));
	}

	String text;
	text += vformat(TTR("[b]Frames:[/b] %d  [b]Range:[/b] %d-%d  [b]Spikes:[/b] %d\n"), frames.size(), from, to, spikes);
	text += vformat(TTR("[b]Average:[/b] %s ms  [b]Worst:[/b] %s ms\n\n"), samples == 0 ? String("N/A") : String::num(total_ms / samples, 2), String::num(max_ms, 2));
	text += TTR("[b]Recommendations[/b]\n");
	for (const Variant &recommendation : _build_recommendations(from, to)) {
		text += "- " + String(recommendation) + "\n";
	}
	summary->set_text(text);
}

void RenderCostReportPanel::_clear_history() {
	frames.clear();
	next_frame_index = 0;
	_update_range_controls();
	_refresh_report();
}

void RenderCostReportPanel::_export_json_pressed() {
	pending_export_format = EXPORT_JSON;
	file_dialog->clear_filters();
	file_dialog->add_filter("*.json", TTRC("JSON Report"));
	file_dialog->popup_file_dialog();
}

void RenderCostReportPanel::_export_markdown_pressed() {
	pending_export_format = EXPORT_MARKDOWN;
	file_dialog->clear_filters();
	file_dialog->add_filter("*.md", TTRC("Markdown Report"));
	file_dialog->popup_file_dialog();
}

void RenderCostReportPanel::_file_selected(const String &p_path) {
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_MSG(err != OK, "Could not write Render Cost Report: " + p_path);
	if (pending_export_format == EXPORT_JSON) {
		file->store_string(JSON::stringify(_build_report_data(_get_first_selected_frame(), _get_last_selected_frame()), "\t"));
	} else {
		file->store_string(_build_markdown_report(_get_first_selected_frame(), _get_last_selected_frame()));
	}
}

RenderCostReportPanel::RenderCostReportPanel() {
	set_name(TTRC("Render Cost"));
	set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);

	HBoxContainer *toolbar = memnew(HBoxContainer);
	add_child(toolbar);
	toolbar->add_child(memnew(Label(TTRC("Spike ms:"))));
	threshold_ms = memnew(SpinBox);
	threshold_ms->set_min(1.0);
	threshold_ms->set_max(1000.0);
	threshold_ms->set_step(1.0);
	threshold_ms->set_value(33.3);
	threshold_ms->connect("value_changed", callable_mp(this, &RenderCostReportPanel::_refresh_report).unbind(1));
	toolbar->add_child(threshold_ms);
	toolbar->add_child(memnew(Label(TTRC("From:"))));
	range_from = memnew(SpinBox);
	range_from->set_min(0.0);
	range_from->set_step(1.0);
	range_from->connect("value_changed", callable_mp(this, &RenderCostReportPanel::_refresh_report).unbind(1));
	toolbar->add_child(range_from);
	toolbar->add_child(memnew(Label(TTRC("To:"))));
	range_to = memnew(SpinBox);
	range_to->set_min(0.0);
	range_to->set_step(1.0);
	range_to->connect("value_changed", callable_mp(this, &RenderCostReportPanel::_refresh_report).unbind(1));
	toolbar->add_child(range_to);

	clear_button = memnew(Button(TTRC("Clear")));
	clear_button->connect(SceneStringName(pressed), callable_mp(this, &RenderCostReportPanel::_clear_history));
	toolbar->add_child(clear_button);
	export_json_button = memnew(Button(TTRC("Export JSON")));
	export_json_button->connect(SceneStringName(pressed), callable_mp(this, &RenderCostReportPanel::_export_json_pressed));
	toolbar->add_child(export_json_button);
	export_markdown_button = memnew(Button(TTRC("Export Markdown")));
	export_markdown_button->connect(SceneStringName(pressed), callable_mp(this, &RenderCostReportPanel::_export_markdown_pressed));
	toolbar->add_child(export_markdown_button);

	summary = memnew(RichTextLabel);
	summary->set_use_bbcode(true);
	summary->set_fit_content(true);
	add_child(summary);

	spike_tree = memnew(Tree);
	spike_tree->set_v_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	spike_tree->set_columns(9);
	spike_tree->set_column_titles_visible(true);
	spike_tree->set_hide_root(true);
	const char *titles[9] = { "Frame", "Frame ms", "Draw Calls", "Primitives", "Objects", "Texture", "Buffer", "Video", "Pipeline Delta" };
	for (int i = 0; i < 9; i++) {
		spike_tree->set_column_title(i, String(titles[i]));
	}
	add_child(spike_tree);

	file_dialog = memnew(EditorFileDialog);
	file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	file_dialog->connect("file_selected", callable_mp(this, &RenderCostReportPanel::_file_selected));
	add_child(file_dialog);

	set_capture_enabled(false);
	_update_range_controls();
	_refresh_report();
}

void RenderCostReportPanel::set_capture_enabled(bool p_enabled) {
	capture_enabled = p_enabled;
}

void RenderCostReportPanel::update_monitor_names(const Array &p_names, const PackedInt32Array &p_types) {
	monitor_names = p_names;
	monitor_types = p_types;
	monitor_indices.clear();
	for (int i = 0; i < monitor_names.size(); i++) {
		monitor_indices[StringName(monitor_names[i])] = i;
	}
	_refresh_report();
}

void RenderCostReportPanel::add_profile_frame(const PackedFloat32Array &p_values) {
	if (!capture_enabled) {
		return;
	}
	FrameSample sample;
	sample.frame_index = next_frame_index++;
	sample.values = p_values;
	frames.push_back(sample);
	while (frames.size() > MAX_FRAME_HISTORY) {
		frames.remove_at(0);
	}
	_update_range_controls();
	_refresh_report();
}