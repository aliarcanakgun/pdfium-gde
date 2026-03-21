# PDFium-GDE (PDFium GDExtension) for Godot 4.5+

This is a Godot 4.5+ GDExtension that brings Google's **PDFium** library to Godot. It allows you to load PDF files, render pages to images, and extract text data (including bounding boxes) directly within your game or application.

## Features

- **Document Loading**: Load PDFs from `res://`, `user://`, or absolute system paths.
- **High-Quality Rendering**: Convert PDF pages to Godot `Image` resources with adjustable DPI.
- **Text Extraction**: Retrieve text content along with its position/rect on the page (great for highlighting or search).
- **Lazy Loading**: Documents are opened only when accessed, saving memory.
- **Editor Integration**: Full support for `load()` and `preload()` in GDScript via a custom import plugin.
- **PDF Generation & Export**: Create new PDF documents from scratch and save them to the filesystem (`res://`, `user://`, or absolute paths).
- **Document Editing**: Add, remove, extract, or merge pages. Insert images, text (with custom fonts and alignment), and drawn shapes (rectangles, paths) into existing or new PDFs.

## How to Build

### Prerequisites
- [Python 3.6+](https://www.python.org/)
- [SCons](https://scons.org/)
- A C++20 compatible compiler (MSVC, GCC, or Clang)

### Compilation
1. Clone the repository recursively to get the `godot-cpp` submodule:
   ```bash
   git clone --recursive https://github.com/pdfium-gde/pdfium-gde.git
   ```
2. Run SCons in the root directory:
   ```bash
   # For Windows
   scons platform=windows target=template_debug
   scons platform=windows target=template_release
   ```
   The compiled binaries will be placed in `demo/addons/pdfium-gde/bin/`.

3. **Crucial Step**: You must copy the `pdfium.dll` (or `.so`/`.dylib`) from the `lib/` folder to the same directory as the compiled GDExtension (`demo/addons/pdfium-gde/bin/`).

## Quick Start

After enabling the plugin in **Project Settings -> Plugins**, you can use it like this:

### Loading via Import System
Simply drop a `.pdf` into your project. It will be automatically recognized.
```gdscript
var doc = load("res://my_document.pdf")
var page = doc.get_page(0)
var img = page.render_to_image(2.0) # Render at 2x scale (144 DPI)

$TextureRect.texture = ImageTexture.create_from_image(img)
```

### Loading External Files
```gdscript
var doc = PDFDocument.new()
var err = doc.load_from_file("C:/Users/Name/Downloads/manual.pdf")
if err == OK:
    var metadata = doc.get_metadata("Title")

var page = doc.get_page(0)
var img = page.render_to_image(2.0) # Render at 2x scale (144 DPI)

$TextureRect.texture = ImageTexture.create_from_image(img)
```

### Creating and Exporting a New PDF
```gdscript
# initialize the document
var doc = PDFDocument.new()
doc.create_empty_doc()

# create a page (defaults to Vector2(1280, 720))
var page1 = doc.create_page(Vector2(1280, 720))

# add a background rectangle (with a blue border and semi-transparent red fill)
page1.add_rect(Rect2(50, 450, 200, 100), Color(1, 0, 0, 0.5), Color(0, 0, 1, 1), 2.0)

# add an image
var img = Image.load_from_file("res://icon.png")
page1.add_image(img, Rect2(0, 0, 128, 128))

# add text with a standard font, bold, and centered
var pos = Vector2(1280 / 2.0, 50)
page1.add_text("Hello World!", pos, "Helvetica", 24.0, Color.BLACK, PDFPage.ALIGN_CENTER, true, false)

# save the generated document
doc.save_to_file("user://generated_output.pdf")
```

### Creating a PDFPage from a Viewport
```gdscript
# grab a viewport as an Image
var img = viewport.get_texture().get_image()

# initialize the document
var doc = PDFDocument.new()
doc.create_empty_doc()

# automatically create a page matching the image size
doc.create_page_from_image(img)

# save the generated document
doc.save_to_file("user://viewport_capture.pdf")
```

### Editing an Existing PDF
```gdscript
var doc = PDFDocument.new()
if doc.load_from_file("res://existing.pdf") != OK:
    return

# delete the second page (index 1)
doc.delete_page(1)

# get the first page and add a custom TTF font text
var page = doc.get_page(0)
page.add_text("Watermark", Vector2(50, 100), "res://fonts/MyFont.ttf", 16.0, Color.RED)

# export the modified PDF
doc.save_to_file("user://edited_output.pdf")
```

### Extracting and Merging Pages
```gdscript
var doc1 = PDFDocument.new()
doc1.load_from_file("res://doc1.pdf")

var doc2 = PDFDocument.new()
doc2.load_from_file("res://doc2.pdf")

# extract the 1st and 3rd pages from doc1 into a new document (pass empty array for all pages)
var extracted = doc1.extract_pages(PackedInt32Array([0, 2]))
extracted.save_to_file("user://extracted.pdf")

# append only the 2nd and 5th pages of doc2 to the end of doc1 (pass empty array for all pages)
doc1.merge_with(doc2, PackedInt32Array([1, 4]))
doc1.save_to_file("user://merged.pdf")
```

## Contributing
Contributions are welcome! If you find a bug or have a feature request, please open an issue or submit a pull request.

## Support
If you find this project useful, please give it a star! It helps more people discover this addon.

## License

- This GDExtension is released under the **zlib License** (see `LICENSE` for details).
- This project uses **PDFium** which is licensed under the **Apache License 2.0** (see `THIRDPARTY.md` for details).
