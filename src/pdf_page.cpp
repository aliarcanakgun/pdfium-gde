#include "pdf_page.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include "fpdf_text.h"
#include "fpdf_edit.h"
#include "pdf_document.h"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/file_access.hpp>

using namespace godot;

void PDFPage::_bind_methods() {
	ClassDB::bind_method(D_METHOD("render_to_image", "dpi_scale"), &PDFPage::render_to_image, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("get_text_data"), &PDFPage::get_text_data);
	ClassDB::bind_method(D_METHOD("get_page_size"), &PDFPage::get_page_size);

	ClassDB::bind_method(D_METHOD("add_image", "image", "rect", "keep_aspect", "opacity"), &PDFPage::add_image, DEFVAL(true), DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("add_rect", "rect", "fill_color", "border_color", "border_thickness"), &PDFPage::add_rect, DEFVAL(Color(0,0,0,0)), DEFVAL(Color(0,0,0,1)), DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("add_path", "points", "fill_color", "border_color", "border_thickness", "closed"), &PDFPage::add_path, DEFVAL(Color(0,0,0,0)), DEFVAL(Color(0,0,0,1)), DEFVAL(1.0f), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("add_text", "text", "position", "font", "size", "color", "alignment", "is_bold", "is_italic"), &PDFPage::add_text, DEFVAL("Helvetica"), DEFVAL(12.0f), DEFVAL(Color(0,0,0,1)), DEFVAL(ALIGN_LEFT), DEFVAL(false), DEFVAL(false));

	BIND_ENUM_CONSTANT(ALIGN_LEFT);
	BIND_ENUM_CONSTANT(ALIGN_CENTER);
	BIND_ENUM_CONSTANT(ALIGN_RIGHT);
}

PDFPage::~PDFPage() {
	if (page) {
		FPDF_ClosePage(page);
		page = nullptr;
	}
	_owner_doc.unref();
}

void PDFPage::_init_page(FPDF_PAGE p_page, int p_index, Ref<Resource> p_owner) {
	page = p_page;
	page_index = p_index;
	_owner_doc = p_owner;
	page_width = FPDF_GetPageWidthF(page);
	page_height = FPDF_GetPageHeightF(page);
}

Vector2 PDFPage::get_page_size() const {
	return Vector2(page_width, page_height);
}

Ref<Image> PDFPage::render_to_image(float dpi_scale) {
	ERR_FAIL_NULL_V_MSG(page, Ref<Image>(), "PDFPage: no page loaded.");
	ERR_FAIL_COND_V_MSG(dpi_scale <= 0.0f, Ref<Image>(), "PDFPage: dpi_scale must be positive.");

	// PDF points to pixels: 1 point = 1/72 inch, at 72 DPI -> 1:1
	// dpi_scale multiplies the base 72 DPI
	int width = static_cast<int>(page_width * dpi_scale);
	int height = static_cast<int>(page_height * dpi_scale);
	ERR_FAIL_COND_V_MSG(width <= 0 || height <= 0, Ref<Image>(), "PDFPage: computed pixel dimensions are invalid.");

	// create BGRA bitmap
	FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 0);
	ERR_FAIL_NULL_V_MSG(bitmap, Ref<Image>(), "PDFPage: failed to create bitmap.");

	// fill white background
	FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
	// render the page into the bitmap
	FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, FPDF_ANNOT | FPDF_LCD_TEXT);

	// copy raw pixel data
	int stride = FPDFBitmap_GetStride(bitmap);
	const uint8_t *buffer = static_cast<const uint8_t *>(FPDFBitmap_GetBuffer(bitmap));

	PackedByteArray pixel_data;
	pixel_data.resize(width * height * 4);
	uint8_t *dst = pixel_data.ptrw();

	// convert BGRx to RGBA row by row (stride may differ from width * 4)
	for (int y = 0; y < height; y++) {
		const uint8_t *row = buffer + y * stride;
		uint8_t *dst_row = dst + y * width * 4;

		for (int x = 0; x < width; x++) {
			int i = x * 4;
			dst_row[i + 0] = row[i + 2]; // R
			dst_row[i + 1] = row[i + 1]; // G
			dst_row[i + 2] = row[i + 0]; // B
			dst_row[i + 3] = 255;
		}
	}

	FPDFBitmap_Destroy(bitmap);

	return Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, pixel_data);
}

Array PDFPage::get_text_data() {
	Array result;
	ERR_FAIL_NULL_V_MSG(page, result, "PDFPage: no page loaded.");

	FPDF_TEXTPAGE text_page = FPDFText_LoadPage(page);
	ERR_FAIL_NULL_V_MSG(text_page, result, "PDFPage: failed to load text page.");

	int char_count = FPDFText_CountChars(text_page);

	// build words from individual characters
	String current_word;
	double word_left = 0.0, word_top = 0.0, word_right = 0.0, word_bottom = 0.0;
	bool has_word = false;

	auto flush_word = [&]() {
		if (has_word && !current_word.is_empty()) {
			Dictionary entry;
			entry["text"] = current_word;

			// flip Y axis: pdfium origin is bottom-left, godot is top-left
			float godot_top = page_height - word_top;
			float godot_bottom = page_height - word_bottom;

			entry["rect"] = Rect2(
				static_cast<float>(word_left),
				godot_top,
				static_cast<float>(word_right - word_left),
				godot_bottom - godot_top
			);

			result.push_back(entry);
		}
		current_word = String();
		has_word = false;
	};

	for (int i = 0; i < char_count; i++) {
		unsigned int unicode = FPDFText_GetUnicode(text_page, i);

		// skip generated newlines, form feeds, etc.
		if (unicode == 0 || unicode == '\r' || unicode == '\n' || unicode == '\f') {
			flush_word();
			continue;
		}

		// spaces terminate the current word
		if (unicode == ' ' || unicode == '\t') {
			flush_word();
			continue;
		}

		// get the bounding box for this character
		double left, right, bottom, top;
		if (!FPDFText_GetCharBox(text_page, i, &left, &right, &bottom, &top)) {
			continue; // skip characters with no valid box
		}

		current_word += String::chr(static_cast<char32_t>(unicode));

		if (!has_word) {
			word_left = left;
			word_right = right;
			word_bottom = bottom;
			word_top = top;
			has_word = true;
		} else {
			if (left < word_left) word_left = left;
			if (right > word_right) word_right = right;
			if (bottom < word_bottom) word_bottom = bottom;
			if (top > word_top) word_top = top;
		}
	}

	flush_word();
	FPDFText_ClosePage(text_page);

	return result;
}

void PDFPage::add_image(Ref<Image> image, const Rect2 &rect, bool keep_aspect, float opacity) {
	ERR_FAIL_NULL_MSG(page, "PDFPage: no page loaded.");
	ERR_FAIL_COND_MSG(image.is_null() || image->is_empty(), "PDFPage: invalid image.");
	
	Ref<Image> local_img = image;
	if (local_img->get_format() != Image::FORMAT_RGBA8) {
		local_img->convert(Image::FORMAT_RGBA8);
	}
	int width = local_img->get_width();
	int height = local_img->get_height();
	PackedByteArray pixel_data = local_img->get_data();
	const uint8_t *src = pixel_data.ptr();
	
	PackedByteArray bgra_data;
	bgra_data.resize(width * height * 4);
	uint8_t *dst = bgra_data.ptrw();
	for (int i = 0; i < width * height; i++) {
		int idx = i * 4;
		dst[idx + 0] = src[idx + 2]; // B
		dst[idx + 1] = src[idx + 1]; // G
		dst[idx + 2] = src[idx + 0]; // R
		dst[idx + 3] = static_cast<uint8_t>(src[idx + 3] * opacity); // A
	}
	
	PDFDocument *doc_res = Object::cast_to<PDFDocument>(_owner_doc.ptr());
	ERR_FAIL_NULL(doc_res);
	FPDF_DOCUMENT doc = doc_res->get_doc();
	
	FPDF_PAGEOBJECT img_obj = FPDFPageObj_NewImageObj(doc);
	FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(width, height, FPDFBitmap_BGRA, dst, width * 4);
	FPDFImageObj_SetBitmap(&page, 1, img_obj, bitmap);
	
	float y_bottom = page_height - rect.position.y - rect.size.height;
	FPDFPageObj_Transform(img_obj, rect.size.width, 0, 0, rect.size.height, rect.position.x, y_bottom);
	
	FPDFPage_InsertObject(page, img_obj);
	FPDFPage_GenerateContent(page);
	FPDFBitmap_Destroy(bitmap);
}

void PDFPage::add_rect(const Rect2 &rect, const Color &fill_color, const Color &border_color, float border_thickness) {
	ERR_FAIL_NULL_MSG(page, "PDFPage: no page loaded.");
	
	float y_bottom = page_height - rect.position.y - rect.size.height;
	FPDF_PAGEOBJECT rect_obj = FPDFPageObj_CreateNewRect(rect.position.x, y_bottom, rect.size.width, rect.size.height);
	
	bool do_fill = fill_color.a > 0.001f;
	bool do_stroke = border_color.a > 0.001f;
	
	if (do_fill) {
		FPDFPageObj_SetFillColor(rect_obj, (unsigned int)(fill_color.r * 255.0f), (unsigned int)(fill_color.g * 255.0f), (unsigned int)(fill_color.b * 255.0f), (unsigned int)(fill_color.a * 255.0f));
	}
	if (do_stroke) {
		FPDFPageObj_SetStrokeColor(rect_obj, (unsigned int)(border_color.r * 255.0f), (unsigned int)(border_color.g * 255.0f), (unsigned int)(border_color.b * 255.0f), (unsigned int)(border_color.a * 255.0f));
		FPDFPageObj_SetStrokeWidth(rect_obj, border_thickness);
	}
	
	FPDFPath_SetDrawMode(rect_obj, do_fill ? 1 : 0, do_stroke ? 1 : 0);
	
	FPDFPage_InsertObject(page, rect_obj);
	FPDFPage_GenerateContent(page);
}

void PDFPage::add_path(const PackedVector2Array &points, const Color &fill_color, const Color &border_color, float border_thickness, bool closed) {
	ERR_FAIL_NULL_MSG(page, "PDFPage: no page loaded.");
	if (points.size() < 2) return;
	
	FPDF_PAGEOBJECT path_obj = FPDFPageObj_CreateNewPath(points[0].x, page_height - points[0].y);
	for (int i = 1; i < points.size(); i++) {
		FPDFPath_LineTo(path_obj, points[i].x, page_height - points[i].y);
	}
	if (closed) {
		FPDFPath_Close(path_obj);
	}
	
	bool do_fill = fill_color.a > 0.001f;
	bool do_stroke = border_color.a > 0.001f;
	
	if (do_fill) FPDFPageObj_SetFillColor(path_obj, (unsigned int)(fill_color.r * 255.0f), (unsigned int)(fill_color.g * 255.0f), (unsigned int)(fill_color.b * 255.0f), (unsigned int)(fill_color.a * 255.0f));
	if (do_stroke) {
		FPDFPageObj_SetStrokeColor(path_obj, (unsigned int)(border_color.r * 255.0f), (unsigned int)(border_color.g * 255.0f), (unsigned int)(border_color.b * 255.0f), (unsigned int)(border_color.a * 255.0f));
		FPDFPageObj_SetStrokeWidth(path_obj, border_thickness);
	}
	
	FPDFPath_SetDrawMode(path_obj, do_fill ? 1 : 0, do_stroke ? 1 : 0);
	
	FPDFPage_InsertObject(page, path_obj);
	FPDFPage_GenerateContent(page);
}

void PDFPage::add_text(const String &text, const Vector2 &position, const String &font_name, float size, const Color &color, int alignment, bool is_bold, bool is_italic) {
	ERR_FAIL_NULL_MSG(page, "PDFPage: no page loaded.");
	
	PDFDocument *doc_res = Object::cast_to<PDFDocument>(_owner_doc.ptr());
	ERR_FAIL_NULL(doc_res);
	FPDF_DOCUMENT doc = doc_res->get_doc();
	
	FPDF_FONT font = nullptr;
	if (font_name.ends_with(".ttf") || font_name.ends_with(".otf")) {
		String abs_path = ProjectSettings::get_singleton()->globalize_path(font_name);
		Ref<FileAccess> fa = FileAccess::open(abs_path, FileAccess::READ);
		if (fa.is_valid()) {
			PackedByteArray font_data = fa->get_buffer(fa->get_length());
			doc_res->keep_font_data(font_data);
			font = FPDFText_LoadFont(doc, font_data.ptr(), font_data.size(), 1, 0);
		}
	} else {
		String final_font = font_name;
		if (is_bold && is_italic) final_font += "-BoldOblique";
		else if (is_bold) final_font += "-Bold";
		else if (is_italic) final_font += "-Oblique";
		
		font = FPDFText_LoadStandardFont(doc, final_font.utf8().get_data());
	}
	
	if (!font) {
		ERR_PRINT("PDFPage: Failed to load font: " + font_name);
		return;
	}
	
	FPDF_PAGEOBJECT text_obj = FPDFPageObj_CreateTextObj(doc, font, size);
	
	int len = text.length();
	PackedByteArray utf16le_data;
	utf16le_data.resize((len + 1) * 2);
	char16_t* ptr = (char16_t*)utf16le_data.ptrw();
	for (int i = 0; i < len; i++) {
		ptr[i] = text[i];
	}
	ptr[len] = 0;
	
	FPDFText_SetText(text_obj, reinterpret_cast<FPDF_WIDESTRING>(utf16le_data.ptr()));
	FPDFPageObj_SetFillColor(text_obj, (unsigned int)(color.r * 255.0f), (unsigned int)(color.g * 255.0f), (unsigned int)(color.b * 255.0f), (unsigned int)(color.a * 255.0f));
	
	float x = position.x;
	float y = page_height - position.y;
	
	FPDFPageObj_Transform(text_obj, 1, 0, 0, 1, x, y);
	
	if (alignment != ALIGN_LEFT) {
		float left, bottom, right, top;
		if (FPDFPageObj_GetBounds(text_obj, &left, &bottom, &right, &top)) {
			float width = right - left;
			if (alignment == ALIGN_CENTER) {
				FPDFPageObj_Transform(text_obj, 1, 0, 0, 1, -width / 2.0f, 0);
			} else if (alignment == ALIGN_RIGHT) {
				FPDFPageObj_Transform(text_obj, 1, 0, 0, 1, -width, 0);
			}
		}
	}
	
	FPDFPage_InsertObject(page, text_obj);
	FPDFPage_GenerateContent(page);
}
