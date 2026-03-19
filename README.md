# PDFium-GDE (PDFium GDExtension) for Godot 4.5+

This is a Godot 4.5+ GDExtension that brings Google's **PDFium** library to Godot. It allows you to load PDF files, render pages to images, and extract text data (including bounding boxes) directly within your game or application.

## Features

- **Document Loading**: Load PDFs from `res://`, `user://`, or absolute system paths.
- **High-Quality Rendering**: Convert PDF pages to Godot `Image` resources with adjustable DPI.
- **Text Extraction**: Retrieve text content along with its position/rect on the page (great for highlighting or search).
- **Lazy Loading**: Documents are opened only when accessed, saving memory.
- **Editor Integration**: Full support for `load()` and `preload()` in GDScript via a custom import plugin.

## Planned Features

The current version focuses on PDF reading and rendering. Future updates aim to include:
- **PDF Creation**: Generate new PDF documents from scratch.
- **Editing**: Add, remove, or modify pages and content objects (text, images) in existing PDFs.
- **PDF Saving**: Export modified documents.

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
func _ready():
    var doc = load("res://my_document.pdf")
    print("Total Pages: ", doc.get_page_count())
    
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
```

## Contributing
Contributions are welcome! If you find a bug or have a feature request, please open an issue or submit a pull request.

## Support
If you find this project useful, please give it a star! It helps more people discover this addon.

## License

- This GDExtension is released under the **zlib License** (see `LICENSE` for details).
- This project uses **PDFium** which is licensed under the **Apache License 2.0** (see `THIRDPARTY.md` for details).
