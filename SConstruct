#!/usr/bin/env python
import os
import sys
import shutil

# You can find documentation for SCons and SConstruct files at:
# https://scons.org/documentation.html

# This lets SCons know that we're using godot-cpp, from the godot-cpp folder.
env = SConscript("godot-cpp/SConstruct")

# Configures the 'src' directory and PDFium headers as include paths.
env.Append(CPPPATH=["src/", "include/"])

# PDFium library linking — resolve platform-specific lib directory.
platform = env["platform"]
arch = env["arch"]
pdfium_lib_path = f"lib/{platform}.{arch}"
env.Append(LIBPATH=[pdfium_lib_path])
if env["platform"] == "windows":
    env.Append(LIBS=["pdfium.dll.lib"])
else:
    env.Append(LIBS=["pdfium"])

# Collects all .cpp files in the 'src' folder as compile targets.
sources = Glob("src/*.cpp")

# The filename for the dynamic library for this GDExtension.
# $SHLIBPREFIX is a platform specific prefix for the dynamic library ('lib' on Unix, '' on Windows).
# $SHLIBSUFFIX is the platform specific suffix for the dynamic library (for example '.dll' on Windows).
# env["suffix"] includes the build's feature tags (e.g. '.windows.template_debug.x86_64')
# (see https://docs.godotengine.org/en/stable/tutorials/export/feature_tags.html).
# The final path should match a path in the '.gdextension' file.
name = "pdfium-gde"
lib_filename = f"{env.subst('$SHLIBPREFIX')}{name}{env['suffix']}{env.subst('$SHLIBSUFFIX')}"

if env["target"] in ["editor", "template_debug"]:
    doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob(f"demo/addons/{name}/docs/*.xml"))
    sources.append(doc_data)

# Creates a SCons target for the path with our sources.
library = env.SharedLibrary(
    f"demo/addons/{name}/{platform}/{lib_filename}",
    source=sources,
)

# Copy the PDFium binaries to the addon folder.

def copy_pdfium_binaries(target, source, env):
    src_dir = env.Dir(f"lib/{platform}.{arch}").abspath
    dst_dir = env.Dir(f"demo/addons/{name}/{platform}/{arch}").abspath
    os.makedirs(dst_dir, exist_ok=True)
    for filename in os.listdir(src_dir):
        shutil.copy2(os.path.join(src_dir, filename), os.path.join(dst_dir, filename))

env.AddPostAction(library, copy_pdfium_binaries)

# Selects the shared library as the default target.
Default(library)
