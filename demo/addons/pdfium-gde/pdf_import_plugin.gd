@tool
extends EditorImportPlugin

func _get_importer_name() -> String:
	return "pdfium_gde.pdf"

func _get_visible_name() -> String:
	return "PDF Document"

func _get_recognized_extensions() -> PackedStringArray:
	return ["pdf"]

func _get_save_extension() -> String:
	return "res"

func _get_resource_type() -> String:
	return "PDFDocument"

func _get_priority() -> float:
	return 1.0

func _get_import_order() -> int:
	return 0

func _get_preset_count() -> int:
	return 1

func _get_preset_name(preset_index: int) -> String:
	return "Default"

func _get_import_options(_path: String, _preset_index: int) -> Array[Dictionary]:
	return []

func _get_option_visibility(_path: String, _option_name: StringName, _options: Dictionary) -> bool:
	return true

func _import(source_file: String, save_path: String, _options: Dictionary, _platform_variants: Array[String], _gen_files: Array[String]) -> Error:
	var doc := PDFDocument.new()
	var err := doc.load_from_file(ProjectSettings.globalize_path(source_file))
	if err != OK:
		return err

	# save the loaded PDFDocument as a Godot resource
	return ResourceSaver.save(doc, "%s.%s" % [save_path, _get_save_extension()])
