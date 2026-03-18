#include "pdf_page.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include "fpdf_text.h"

using namespace godot;

void PDFPage::_bind_methods() {
	ClassDB::bind_method(D_METHOD("render_to_image", "dpi_scale"), &PDFPage::render_to_image, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("get_text_data"), &PDFPage::get_text_data);
	ClassDB::bind_method(D_METHOD("get_page_size"), &PDFPage::get_page_size);
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
