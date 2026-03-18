#pragma once

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "fpdfview.h"

namespace godot {

class PDFPage : public RefCounted {
	GDCLASS(PDFPage, RefCounted)

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

	void _init_page(FPDF_PAGE p_page, int p_index, Ref<Resource> p_owner);

	~PDFPage();
};

} // namespace godot
