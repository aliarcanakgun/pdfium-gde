#include "pdf_document.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "fpdf_doc.h"
#include "fpdf_save.h"
#include "fpdf_edit.h"
#include <godot_cpp/classes/file_access.hpp>

namespace {
struct GodotFileWrite : public FPDF_FILEWRITE {
	godot::Ref<godot::FileAccess> file;

	GodotFileWrite() {
		version = 1;
		WriteBlock = &GodotFileWrite::write_block;
	}

	static int write_block(FPDF_FILEWRITE* pThis, const void* pData, unsigned long size) {
		GodotFileWrite* writer = static_cast<GodotFileWrite*>(pThis);
		if (writer->file.is_valid()) {
			godot::PackedByteArray data;
			data.resize(size);
			memcpy(data.ptrw(), pData, size);
			writer->file->store_buffer(data);
			return 1;
		}
		return 0;
	}
};
} // namespace

using namespace godot;

int PDFDocument::_pdfium_ref_count = 0;

void PDFDocument::_ensure_pdfium_init() {
	if (_pdfium_ref_count == 0) {
		FPDF_InitLibrary();
	}
	_pdfium_ref_count++;
}

void PDFDocument::_release_pdfium() {
	_pdfium_ref_count--;
	if (_pdfium_ref_count <= 0) {
		FPDF_DestroyLibrary();
		_pdfium_ref_count = 0;
	}
}

void PDFDocument::_bind_methods() {
	ClassDB::bind_method(D_METHOD("create_empty_doc"), &PDFDocument::create_empty_doc);
	ClassDB::bind_method(D_METHOD("save_to_file", "path"), &PDFDocument::save_to_file);
	ClassDB::bind_method(D_METHOD("create_page", "size"), &PDFDocument::create_page, DEFVAL(Vector2(1280, 720)));
	ClassDB::bind_method(D_METHOD("create_page_from_image", "image"), &PDFDocument::create_page_from_image);
	ClassDB::bind_method(D_METHOD("delete_page", "index"), &PDFDocument::delete_page);

	ClassDB::bind_method(D_METHOD("load_from_file", "path"), &PDFDocument::load_from_file);
	ClassDB::bind_method(D_METHOD("get_page_count"), &PDFDocument::get_page_count);
	ClassDB::bind_method(D_METHOD("get_metadata", "key"), &PDFDocument::get_metadata);
	ClassDB::bind_method(D_METHOD("get_page", "index"), &PDFDocument::get_page);
	ClassDB::bind_method(D_METHOD("is_loaded"), &PDFDocument::is_loaded);

	ClassDB::bind_method(D_METHOD("set_file_path", "path"), &PDFDocument::set_file_path);
	ClassDB::bind_method(D_METHOD("get_file_path"), &PDFDocument::get_file_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "file_path", PROPERTY_HINT_FILE, "*.pdf"), "set_file_path", "get_file_path");
}

PDFDocument::~PDFDocument() {
	if (doc) {
		FPDF_CloseDocument(doc);
		doc = nullptr;
	}
	if (_pdfium_initialized) {
		_release_pdfium();
	}
}

Error PDFDocument::load_from_file(const String &path) {
	// close any previously loaded document
	if (doc) {
		FPDF_CloseDocument(doc);
		doc = nullptr;
	}

	if (!_pdfium_initialized) {
		_ensure_pdfium_init();
		_pdfium_initialized = true;
	}

	file_path = path;

	// pdfium needs an absolute OS path, not a godot virtual path
	String abs_path = path;
	if (abs_path.begins_with("res://") || abs_path.begins_with("user://")) {
		abs_path = ProjectSettings::get_singleton()->globalize_path(abs_path);
	}

	CharString utf8_path = abs_path.utf8();
	doc = FPDF_LoadDocument(utf8_path.get_data(), nullptr);

	if (!doc) {
		unsigned long err = FPDF_GetLastError();
		switch (err) {
			case FPDF_ERR_FILE:
				ERR_PRINT("PDFDocument: file not found or could not be opened: " + path);
				return ERR_FILE_NOT_FOUND;
			case FPDF_ERR_FORMAT:
				ERR_PRINT("PDFDocument: file is not a valid PDF: " + path);
				return ERR_FILE_CORRUPT;
			case FPDF_ERR_PASSWORD:
				ERR_PRINT("PDFDocument: PDF requires a password: " + path);
				return ERR_FILE_CANT_OPEN;
			case FPDF_ERR_SECURITY:
				ERR_PRINT("PDFDocument: unsupported security scheme: " + path);
				return ERR_FILE_CANT_OPEN;
			default:
				ERR_PRINT("PDFDocument: unknown error loading PDF: " + path);
				return ERR_FILE_CANT_OPEN;
		}
	}

	return OK;
}

void PDFDocument::create_empty_doc() {
	if (doc) {
		FPDF_CloseDocument(doc);
		doc = nullptr;
	}

	if (!_pdfium_initialized) {
		_ensure_pdfium_init();
		_pdfium_initialized = true;
	}

	doc = FPDF_CreateNewDocument();
	file_path = "";
}

Error PDFDocument::save_to_file(const String &path) {
	_ensure_loaded();
	ERR_FAIL_COND_V_MSG(!doc, Error::ERR_UNCONFIGURED, "PDFDocument: no document loaded.");

	String abs_path = path;
	if (abs_path.begins_with("res://") || abs_path.begins_with("user://")) {
		abs_path = ProjectSettings::get_singleton()->globalize_path(abs_path);
	}

	Ref<FileAccess> file = FileAccess::open(abs_path, FileAccess::WRITE);
	if (file.is_null()) {
		ERR_PRINT("PDFDocument: could not open file for writing: " + abs_path);
		return Error::ERR_FILE_CANT_WRITE;
	}

	GodotFileWrite writer;
	writer.file = file;

	if (FPDF_SaveAsCopy(doc, &writer, 0)) {
		file->flush();
		return OK;
	} else {
		ERR_PRINT("PDFDocument: FPDF_SaveAsCopy failed.");
		return Error::FAILED;
	}
}

Ref<PDFPage> PDFDocument::create_page(const Vector2 &size) {
	_ensure_loaded();
	ERR_FAIL_COND_V_MSG(!doc, Ref<PDFPage>(), "PDFDocument: no document loaded.");
	
	int index = FPDF_GetPageCount(doc);
	FPDF_PAGE page = FPDFPage_New(doc, index, size.width, size.height);
	ERR_FAIL_NULL_V_MSG(page, Ref<PDFPage>(), "PDFDocument: failed to create new page.");

	Ref<PDFPage> pdf_page;
	pdf_page.instantiate();
	pdf_page->_init_page(page, index, Ref<Resource>(this));

	return pdf_page;
}

Ref<PDFPage> PDFDocument::create_page_from_image(Ref<Image> image) {
	ERR_FAIL_COND_V_MSG(image.is_null() || image->is_empty(), Ref<PDFPage>(), "PDFDocument: invalid image.");
	Vector2 size(image->get_width(), image->get_height());
	Ref<PDFPage> new_page = create_page(size);
	if (new_page.is_valid()) {
		new_page->add_image(image, Rect2(0, 0, size.x, size.y), true);
	}
	return new_page;
}

void PDFDocument::delete_page(int index) {
	_ensure_loaded();
	ERR_FAIL_COND_MSG(!doc, "PDFDocument: no document loaded.");
	int count = FPDF_GetPageCount(doc);
	ERR_FAIL_INDEX_MSG(index, count, "PDFDocument: page index out of range.");
	
	FPDFPage_Delete(doc, index);
}

void PDFDocument::set_file_path(const String &path) {
	file_path = path;
}

String PDFDocument::get_file_path() const {
	return file_path;
}

void PDFDocument::_ensure_loaded() {
	if (doc) {
		return;
	}

	// try file_path first, fall back to the resource path set by Godot's loader
	String path_to_load = file_path;
	if (path_to_load.is_empty()) {
		path_to_load = get_path();
	}
	if (path_to_load.is_empty()) {
		return;
	}

	load_from_file(path_to_load);
}

int PDFDocument::get_page_count() const {
	const_cast<PDFDocument *>(this)->_ensure_loaded();
	ERR_FAIL_COND_V_MSG(!doc, 0, "PDFDocument: no document loaded.");
	return FPDF_GetPageCount(doc);
}

String PDFDocument::get_metadata(const String &key) const {
	const_cast<PDFDocument *>(this)->_ensure_loaded();
	ERR_FAIL_COND_V_MSG(!doc, String(), "PDFDocument: no document loaded.");

	CharString utf8_key = key.utf8();

	unsigned long len = FPDF_GetMetaText(doc, utf8_key.get_data(), nullptr, 0);
	if (len <= 2) {
		// no data
		return String();
	}

	// allocate buffer and retrieve the UTF-16LE encoded value
	PackedByteArray buffer;
	buffer.resize(len);
	FPDF_GetMetaText(doc, utf8_key.get_data(), buffer.ptrw(), len);

	// convert UTF-16LE buffer to godot::String
	// the buffer length is in bytes, each char16_t is 2 bytes, including the null terminator
	int char_count = (int)(len / 2) - 1; // exclude null terminator
	const char16_t *utf16_data = reinterpret_cast<const char16_t *>(buffer.ptr());
	
	return String::utf16(utf16_data, char_count);
}

Ref<PDFPage> PDFDocument::get_page(int index) {
	_ensure_loaded();
	ERR_FAIL_COND_V_MSG(!doc, Ref<PDFPage>(), "PDFDocument: no document loaded.");
	ERR_FAIL_INDEX_V_MSG(index, FPDF_GetPageCount(doc), Ref<PDFPage>(), "PDFDocument: page index out of range.");

	FPDF_PAGE page = FPDF_LoadPage(doc, index);
	ERR_FAIL_NULL_V_MSG(page, Ref<PDFPage>(), "PDFDocument: failed to load page " + itos(index) + ".");

	Ref<PDFPage> pdf_page;
	pdf_page.instantiate();
	pdf_page->_init_page(page, index, Ref<Resource>(this));

	return pdf_page;
}

bool PDFDocument::is_loaded() const {
	return doc != nullptr;
}
