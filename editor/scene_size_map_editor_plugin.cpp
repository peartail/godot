/**************************************************************************/
/*  scene_size_map_editor_plugin.cpp                                      */
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

#include "scene_size_map_editor_plugin.h"

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tree.h"

class SceneSizeMapTreemap : public Control {
	GDCLASS(SceneSizeMapTreemap, Control);

	struct TreemapItem {
		String name;
		String path;
		String type;
		int64_t size = 0;
		Rect2 rect;
		Color color;
	};

	Vector<TreemapItem> items;
	int64_t total_size = 0;

	static Color _get_type_color(const String &p_type) {
		const uint32_t hash = p_type.hash();
		const float hue = (hash % 360) / 360.0f;
		return Color::from_hsv(hue, 0.55f, 0.78f);
	}

	static String _format_size(int64_t p_size) {
		return SceneSizeMapEditor::_format_size(p_size);
	}

	static void _layout_items(Vector<TreemapItem> &r_items, int p_begin, int p_end, const Rect2 &p_rect) {
		if (p_begin >= p_end) {
			return;
		}

		if (p_begin + 1 == p_end) {
			r_items.write[p_begin].rect = p_rect;
			return;
		}

		int64_t sum = 0;
		for (int i = p_begin; i < p_end; i++) {
			sum += MAX<int64_t>(1, r_items[i].size);
		}

		int split = p_begin;
		int64_t first_sum = 0;
		for (int i = p_begin; i < p_end; i++) {
			const int64_t next_sum = first_sum + MAX<int64_t>(1, r_items[i].size);
			if (i > p_begin && Math::abs((double)sum / 2.0 - (double)next_sum) > Math::abs((double)sum / 2.0 - (double)first_sum)) {
				break;
			}
			first_sum = next_sum;
			split = i + 1;
		}

		if (split <= p_begin || split >= p_end) {
			split = p_begin + (p_end - p_begin) / 2;
			first_sum = 0;
			for (int i = p_begin; i < split; i++) {
				first_sum += MAX<int64_t>(1, r_items[i].size);
			}
		}

		const double ratio = sum > 0 ? (double)first_sum / (double)sum : 0.5;
		if (p_rect.size.x >= p_rect.size.y) {
			const float split_width = p_rect.size.x * ratio;
			_layout_items(r_items, p_begin, split, Rect2(p_rect.position, Size2(split_width, p_rect.size.y)));
			_layout_items(r_items, split, p_end, Rect2(Point2(p_rect.position.x + split_width, p_rect.position.y), Size2(p_rect.size.x - split_width, p_rect.size.y)));
		} else {
			const float split_height = p_rect.size.y * ratio;
			_layout_items(r_items, p_begin, split, Rect2(p_rect.position, Size2(p_rect.size.x, split_height)));
			_layout_items(r_items, split, p_end, Rect2(Point2(p_rect.position.x, p_rect.position.y + split_height), Size2(p_rect.size.x, p_rect.size.y - split_height)));
		}
	}

	void _rebuild_layout() {
		if (items.is_empty()) {
			return;
		}
		const float margin = 8.0f * EDSCALE;
		Rect2 layout_rect(Point2(margin, margin), get_size() - Size2(margin * 2.0f, margin * 2.0f));
		if (layout_rect.size.x <= 0 || layout_rect.size.y <= 0) {
			return;
		}
		_layout_items(items, 0, items.size(), layout_rect);
	}

protected:
	void _notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_RESIZED: {
				_rebuild_layout();
				queue_redraw();
			} break;

			case NOTIFICATION_DRAW: {
				const Color bg = get_theme_color(SNAME("dark_color_2"), EditorStringName(Editor));
				const Color mono_color = get_theme_color(SNAME("mono_color"), EditorStringName(Editor));
				const Color border(mono_color.r, mono_color.g, mono_color.b, 0.18f);
				draw_rect(Rect2(Point2(), get_size()), bg);

				if (items.is_empty()) {
					Ref<Font> font = get_theme_font(SceneStringName(font));
					const int font_size = get_theme_font_size(SceneStringName(font_size));
					const String text = TTR("Select a scene and analyze its referenced file sizes.");
					const Size2 text_size = font->get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
					draw_string(font, (get_size() - text_size) * 0.5f + Vector2(0, font->get_ascent(font_size)), text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, get_theme_color(SceneStringName(font_color), EditorStringName(Editor)));
					return;
				}

				Ref<Font> font = get_theme_font(SceneStringName(font));
				const int font_size = get_theme_font_size(SceneStringName(font_size));
				const Color text_color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
				const Color outline_color = Color(0, 0, 0, 0.55f);

				for (const TreemapItem &item : items) {
					Rect2 rect = item.rect.grow(-1.0f * EDSCALE);
					if (rect.size.x <= 0 || rect.size.y <= 0) {
						continue;
					}

					draw_rect(rect, item.color);
					draw_rect(rect, border, false, 1.0f * EDSCALE);

					if (rect.size.x < 72.0f * EDSCALE || rect.size.y < 34.0f * EDSCALE) {
						continue;
					}

					const String label = item.name + "\n" + _format_size(item.size);
					const Vector<String> lines = label.split("\n");
					float y = rect.position.y + 5.0f * EDSCALE + font->get_ascent(font_size);
					for (const String &line : lines) {
						if (y > rect.position.y + rect.size.y - 4.0f * EDSCALE) {
							break;
						}
						const Point2 pos(rect.position.x + 4.0f * EDSCALE, y);
						draw_string(font, pos + Point2(1, 1), line, HORIZONTAL_ALIGNMENT_LEFT, rect.size.x - 8.0f * EDSCALE, font_size, outline_color);
						draw_string(font, pos, line, HORIZONTAL_ALIGNMENT_LEFT, rect.size.x - 8.0f * EDSCALE, font_size, text_color);
						y += font->get_height(font_size);
					}
				}
			} break;
		}
	}

public:
	void set_items(const Vector<SceneSizeMapEditor::SizeItem> &p_items, int64_t p_total_size) {
		items.clear();
		total_size = p_total_size;
		for (const SceneSizeMapEditor::SizeItem &source_item : p_items) {
			TreemapItem item;
			item.name = source_item.path.get_file();
			item.path = source_item.path;
			item.type = source_item.type;
			item.size = source_item.size;
			item.color = _get_type_color(item.type);
			items.push_back(item);
		}
		_rebuild_layout();
		queue_redraw();
	}
};

struct SceneSizeMapItemSort {
	bool operator()(const SceneSizeMapEditor::SizeItem &p_a, const SceneSizeMapEditor::SizeItem &p_b) const {
		if (p_a.size == p_b.size) {
			return p_a.path < p_b.path;
		}
		return p_a.size > p_b.size;
	}
};

String SceneSizeMapEditor::_format_size(int64_t p_size) {
	const char *units[] = { "B", "KB", "MB", "GB", "TB" };
	double size = MAX<int64_t>(0, p_size);
	int unit = 0;
	while (size >= 1024.0 && unit < 4) {
		size /= 1024.0;
		unit++;
	}
	if (unit == 0) {
		return vformat("%d %s", p_size, units[unit]);
	}
	return vformat("%.2f %s", size, units[unit]);
}

String SceneSizeMapEditor::_get_resolved_dependency_path(const String &p_dependency) {
	if (p_dependency.get_slice_count("::") < 3) {
		return p_dependency.get_slice("::", 0);
	}

	const String uid_text = p_dependency.get_slice("::", 0);
	const ResourceUID::ID uid = ResourceUID::get_singleton()->text_to_id(uid_text);
	if (uid != ResourceUID::INVALID_ID && ResourceUID::get_singleton()->has_id(uid)) {
		return ResourceUID::get_singleton()->get_id_path(uid);
	}

	return p_dependency.get_slice("::", 2);
}

String SceneSizeMapEditor::_get_dependency_type(const String &p_dependency) {
	if (p_dependency.get_slice_count("::") >= 3) {
		return p_dependency.get_slice("::", 1);
	}
	return EditorFileSystem::get_singleton() ? EditorFileSystem::get_singleton()->get_file_type(_get_resolved_dependency_path(p_dependency)) : String("Resource");
}

void SceneSizeMapEditor::_browse_pressed() {
	file_dialog->popup_file_dialog();
}

void SceneSizeMapEditor::_file_selected(const String &p_path) {
	scene_path->set_text(p_path);
	_analyze_scene(p_path);
}

void SceneSizeMapEditor::_analyze_pressed() {
	_analyze_scene(scene_path->get_text().strip_edges());
}

void SceneSizeMapEditor::_analyze_scene(const String &p_scene_path) {
	items.clear();
	total_size = 0;

	if (p_scene_path.is_empty()) {
		_update_summary();
		_rebuild_resource_list();
		treemap->set_items(items, total_size);
		return;
	}

	if (!FileAccess::exists(p_scene_path)) {
		summary_label->set_text(TTR("Scene file does not exist."));
		_rebuild_resource_list();
		treemap->set_items(items, total_size);
		return;
	}

	HashSet<String> unique_paths;
	unique_paths.insert(p_scene_path);

	List<String> dependencies;
	ResourceLoader::get_dependencies(p_scene_path, &dependencies, true);
	for (const String &dependency : dependencies) {
		const String resolved_path = _get_resolved_dependency_path(dependency);
		if (!resolved_path.is_empty()) {
			unique_paths.insert(resolved_path);
		}
	}

	for (const String &path : unique_paths) {
		SizeItem item;
		item.path = path;
		item.type = path == p_scene_path ? String("PackedScene") : EditorFileSystem::get_singleton()->get_file_type(path);
		if (item.type.is_empty()) {
			for (const String &dependency : dependencies) {
				if (_get_resolved_dependency_path(dependency) == path) {
					item.type = _get_dependency_type(dependency);
					break;
				}
			}
		}
		if (item.type.is_empty()) {
			item.type = "Resource";
		}
		item.size = FileAccess::exists(path) ? FileAccess::get_size(path) : 0;
		total_size += item.size;
		items.push_back(item);
	}

	items.sort_custom<SceneSizeMapItemSort>();
	_update_summary();
	_rebuild_resource_list();
	treemap->set_items(items, total_size);
}

void SceneSizeMapEditor::_rebuild_resource_list() {
	resource_list->clear();

	TreeItem *root = resource_list->create_item();
	for (const SizeItem &item : items) {
		TreeItem *tree_item = resource_list->create_item(root);
		tree_item->set_text(0, item.path.get_file());
		tree_item->set_text(1, _format_size(item.size));
		tree_item->set_text(2, item.type);
		tree_item->set_text(3, item.path);
		tree_item->set_metadata(0, item.path);

		Ref<Texture2D> icon = EditorNode::get_singleton()->get_class_icon(item.type);
		if (icon.is_valid()) {
			tree_item->set_icon(0, icon);
		}
	}
}

void SceneSizeMapEditor::_update_summary() {
	if (items.is_empty()) {
		summary_label->set_text(TTR("No scene analyzed."));
		return;
	}
	summary_label->set_text(vformat(TTR("%s across %d unique files."), _format_size(total_size), items.size()));
}

void SceneSizeMapEditor::_update_theme() {
	browse_button->set_button_icon(get_editor_theme_icon(SNAME("Folder")));
	analyze_button->set_button_icon(get_editor_theme_icon(SNAME("Reload")));
}

void SceneSizeMapEditor::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		_update_theme();
	}
}

SceneSizeMapEditor::SceneSizeMapEditor() {
	set_title(TTRC("Scene Size Map"));
	set_layout_key("scene_size_map");
	set_icon_name(SNAME("PackedScene"));
	set_default_slot(EditorDock::DOCK_SLOT_RIGHT_BL);

	VBoxContainer *main_vbox = memnew(VBoxContainer);
	main_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
	main_vbox->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(main_vbox);

	HBoxContainer *toolbar = memnew(HBoxContainer);
	main_vbox->add_child(toolbar);

	scene_path = memnew(LineEdit);
	scene_path->set_h_size_flags(SIZE_EXPAND_FILL);
	scene_path->set_placeholder(TTR("Scene path"));
	toolbar->add_child(scene_path);

	browse_button = memnew(Button);
	browse_button->set_tooltip_text(TTR("Select Scene"));
	browse_button->connect(SceneStringName(pressed), callable_mp(this, &SceneSizeMapEditor::_browse_pressed));
	toolbar->add_child(browse_button);

	analyze_button = memnew(Button);
	analyze_button->set_text(TTR("Analyze"));
	analyze_button->connect(SceneStringName(pressed), callable_mp(this, &SceneSizeMapEditor::_analyze_pressed));
	toolbar->add_child(analyze_button);

	summary_label = memnew(Label);
	summary_label->set_text(TTR("No scene analyzed."));
	main_vbox->add_child(summary_label);

	HSplitContainer *split = memnew(HSplitContainer);
	split->set_v_size_flags(SIZE_EXPAND_FILL);
	main_vbox->add_child(split);

	treemap = memnew(SceneSizeMapTreemap);
	treemap->set_custom_minimum_size(Size2(320, 220) * EDSCALE);
	treemap->set_h_size_flags(SIZE_EXPAND_FILL);
	treemap->set_v_size_flags(SIZE_EXPAND_FILL);
	split->add_child(treemap);

	resource_list = memnew(Tree);
	resource_list->set_columns(4);
	resource_list->set_column_title(0, TTR("File"));
	resource_list->set_column_title(1, TTR("Size"));
	resource_list->set_column_title(2, TTR("Type"));
	resource_list->set_column_title(3, TTR("Path"));
	resource_list->set_column_titles_visible(true);
	resource_list->set_hide_root(true);
	resource_list->set_column_expand(0, true);
	resource_list->set_column_expand(1, false);
	resource_list->set_column_custom_minimum_width(1, 82 * EDSCALE);
	resource_list->set_column_expand(2, false);
	resource_list->set_column_custom_minimum_width(2, 90 * EDSCALE);
	resource_list->set_custom_minimum_size(Size2(260, 220) * EDSCALE);
	split->add_child(resource_list);

	file_dialog = memnew(EditorFileDialog);
	file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->set_title(TTR("Select Scene"));
	List<String> extensions;
	ResourceLoader::get_recognized_extensions_for_type("PackedScene", &extensions);
	for (const String &extension : extensions) {
		file_dialog->add_filter("*." + extension, extension.to_upper());
	}
	file_dialog->connect("file_selected", callable_mp(this, &SceneSizeMapEditor::_file_selected));
	add_child(file_dialog);
}

void SceneSizeMapEditorPlugin::make_visible(bool p_visible) {
	if (p_visible) {
		scene_size_map->make_visible();
	} else {
		scene_size_map->close();
	}
}

SceneSizeMapEditorPlugin::SceneSizeMapEditorPlugin() {
	scene_size_map = memnew(SceneSizeMapEditor);
	EditorDockManager::get_singleton()->add_dock(scene_size_map);
	scene_size_map->close();
}




