# THIS IS A VERY BASIC EXAMPLE OF 'PDFium-GDE' EXTENSION
extends Control

func _ready() -> void:
	# "res://content/motorsports.pdf" file is downloaded from "https://www.scribd.com/document/984376224/Motorsports"
	var doc: PDFDocument = preload("res://content/motorsports.pdf")
	if !doc: return
	
	%page_count.text = "/ " + str(doc.get_page_count())
	%pages.add_spacer(true)
	
	for i in doc.get_page_count():
		var page := doc.get_page(i)
		var img := page.render_to_image(2.0) # 144 DPI
		
		var rect := TextureRect.new()
		rect.texture = ImageTexture.create_from_image(img)
		rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		rect.custom_minimum_size.y = page.get_page_size().y
		
		%pages.add_child(rect)

func _on_page_num_text_submitted(text: String) -> void:
	var i := text.to_int()
	%page_num.text = str(i)
	
	# TODO: jump to that page
