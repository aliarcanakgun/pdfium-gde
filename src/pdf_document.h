#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

#include "fpdfview.h"
#include "pdf_page.h"

namespace godot {

class PDFDocument : public Resource {
	GDCLASS(PDFDocument, Resource)

private:
	FPDF_DOCUMENT doc = nullptr;
	String file_path;
	bool _pdfium_initialized = false;
	Array _loaded_fonts;

	void _ensure_loaded();

	static int _pdfium_ref_count;
	static void _ensure_pdfium_init();
	static void _release_pdfium();

protected:
	static void _bind_methods();

public:
	FPDF_DOCUMENT get_doc() const { return doc; }
	void keep_font_data(const PackedByteArray &data) { _loaded_fonts.push_back(data); }
	Error load_from_file(const String &path);
	void create_empty_doc();
	Error save_to_file(const String &path);
	Ref<PDFPage> create_page(const Vector2 &size = Vector2(1280, 720));
	Ref<PDFPage> create_page_from_image(Ref<Image> image);
	void delete_page(int index);

	int get_page_count() const;
	String get_metadata(const String &key) const;
	Ref<PDFPage> get_page(int index);
	bool is_loaded() const;

	void set_file_path(const String &path);
	String get_file_path() const;

	~PDFDocument();
};

} // namespace godot
