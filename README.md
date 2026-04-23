# PDFium-GDE (PDFium GDExtension) for Godot 4.5+

This is a Godot 4.5+ GDExtension that brings Google's **PDFium** library to Godot. It allows you to load PDF files, render pages to images, and extract text data (including bounding boxes) directly within your game or application.

## Features

- **Document Loading**: Load PDFs from `res://`, `user://`, or absolute system paths.
- **High-Quality Rendering**: Convert PDF pages to Godot `Image` resources with adjustable DPI.
- **Text Extraction**: Retrieve text content along with its position/rect on the page (great for highlighting or search).
- **Lazy Loading**: Documents are opened only when accessed, saving memory.
- **Editor Integration**: Full support for `load()` and `preload()` in GDScript via a custom import plugin.
- **PDF Generation & Export**: Create new PDF documents from scratch and save them to the filesystem (`res://`, `user://`, or absolute paths).
- **Document Editing**: Add, remove, extract, or merge pages. Add document-wide watermarks; insert images, text (with custom fonts and alignment), and drawn shapes (rectangles, paths) into existing or new PDFs.

## Installation (Pre-built Release)

1. Go to the [Releases](https://github.com/aliarcanakgun/pdfium-gde/releases) page.
2. Download the latest version.
3. Extract the downloaded zip file into your project's root folder.
4. Enable the plugin in **Project Settings -> Plugins**.

## Building from Source

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
   # for Windows
   scons platform=windows target=template_debug
   scons platform=windows target=template_release
   
   # for Linux
   scons platform=linux target=template_debug
   scons platform=linux target=template_release
   
   # for macOS
   scons platform=macos target=template_debug
   scons platform=macos target=template_release
   ```
   The compiled binaries will be placed in OS-specific folders under `demo/addons/pdfium-gde/`.

3. **Crucial Step**: You must copy the `pdfium.dll` (or `libpdfium.so`/`libpdfium.dylib`) from the corresponding platform folder in `lib/` (e.g., `lib/windows.x86_64/`) to the same OS-specific directory as the compiled GDExtension (e.g., `demo/addons/pdfium-gde/windows/`).

4. **Finalize**: Copy the entire `demo/addons/pdfium-gde/` folder into your project's `addons` folder, and enable the plugin in **Project Settings -> Plugins**.

## Quick Start

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
var extracted = doc1.extract_pages([0, 2])
extracted.save_to_file("user://extracted.pdf")

# append only the 2nd and 5th pages of doc2 to the end of doc1 (pass empty array for all pages)
doc1.merge_with(doc2, [1, 4])
doc1.save_to_file("user://merged.pdf")
```

### Watermarking Pages
```gdscript
var doc = PDFDocument.new()
doc.load_from_file("res://document.pdf")

var logo = Image.load_from_file("res://watermark.png")

# add a 50% transparent watermark exactly at the center of all pages
doc.add_watermark_image(logo, Vector2(0.5, 0.5), Vector2(0, 0), 1.0, 0.5)

# add a small logo at the bottom right of the 1st page, tucked 20px inwards (index 0)
doc.add_watermark_image(logo, Vector2(1.0, 1.0), Vector2(-20, -20), 0.25, 0.8, [0])

doc.save_to_file("user://watermarked_output.pdf")
```

## Contributing
Contributions are welcome! If you find a bug or have a feature request, please open an issue or submit a pull request.

## Support
If you find this project useful, please give it a star! It helps more people discover this addon.

## License

- This GDExtension is released under the **zlib License** (see `LICENSE` for details).
- This project uses **PDFium** which is licensed under the **Apache License 2.0** (see `THIRDPARTY.md` for details).
