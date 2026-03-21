# THIS IS A VERY BASIC EXAMPLE OF 'PDFium-GDE' EXTENSION
extends Control

func _ready() -> void:
	# initialize the document
	var doc = PDFDocument.new()
	doc.create_empty_doc()
	
	# create a page (defaults to Vector2(1280, 720))
	var page1 = doc.create_page()
	
	# add a background rectangle (with a blue border and semi-transparent red fill)
	page1.add_rect(Rect2(50, 450, 200, 100), Color(1, 0, 0, 0.5), Color(0, 0, 1, 1), 2.0)
	
	# add an image
	var img = Image.load_from_file("res://icon.svg")
	page1.add_image(img, Rect2(0, 0, 128, 128))
	
	# add text with a standard font, bold, and centered
	var pos = Vector2(1280 / 2.0, 50)
	page1.add_text("Hello World!", pos, "Helvetica", 24.0, Color.BLACK, PDFPage.ALIGN_CENTER, true, false)
	
	# save the generated document
	#doc.save_to_file("user://generated_output.pdf")
	
	# preview the generated page
	$preview_page1.texture = ImageTexture.create_from_image(doc.get_page(0).render_to_image(2.0))
