#!/bin/sh

# Shade3Dのアプリケーションのフルパス.
Shade3D_APP="/Volumes/HD-PEU2/Application/Shade 3D ver.14.app"

# Shade3Dのドキュメントディレクトリのフルパス.
Shade3D_PLUGINS_PATH="/Users/UserName/Documents/Shade 3D ver.14/plugins"

export DYLD_LIBRARY_PATH="${Shade3D_PLUGINS_PATH}/USDExporter.shdplugin/Contents/Frameworks"

"${Shade3D_APP}/Contents/MacOS/xshade"

