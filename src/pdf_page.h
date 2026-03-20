#pragma once

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include "fpdfview.h"

namespace godot {

class PDFPage : public RefCounted {
	GDCLASS(PDFPage, RefCounted)

public:
	enum Alignment {
		ALIGN_LEFT = 0,
		ALIGN_CENTER = 1,
		ALIGN_RIGHT = 2
	};

private:
	FPDF_PAGE page = nullptr;
	int page_index = -1;
	float page_width = 0.0f;
	float page_height = 0.0f;
	Ref<Resource> _owner_doc;

protected:
	static void _bind_methods();

public:
	Ref<Image> render_to_image(float dpi_scale = 1.0f);
	Array get_text_data();
	Vector2 get_page_size() const;

	void add_image(Ref<Image> image, const Rect2 &rect, bool keep_aspect = true);
	void add_rect(const Rect2 &rect, const Color &fill_color = Color(0,0,0,0), const Color &border_color = Color(0,0,0,1), float border_thickness = 1.0f);
	void add_path(const PackedVector2Array &points, const Color &fill_color = Color(0,0,0,0), const Color &border_color = Color(0,0,0,1), float border_thickness = 1.0f, bool closed = false);
	void add_text(const String &text, const Vector2 &position, const String &font = "Helvetica", float size = 12.0f, const Color &color = Color(0,0,0,1), int alignment = ALIGN_LEFT, bool is_bold = false, bool is_italic = false);

	void _init_page(FPDF_PAGE p_page, int p_index, Ref<Resource> p_owner);

	~PDFPage();
};

} // namespace godot

VARIANT_ENUM_CAST(godot::PDFPage::Alignment);
