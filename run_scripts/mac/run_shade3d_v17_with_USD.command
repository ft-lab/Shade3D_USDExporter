#!/bin/sh

# Shade3Dのアプリケーションのフルパス.
Shade3D_APP="/Volumes/HD-PEU2/Application/Shade3D ver.17.app"

# Shade3Dのドキュメントディレクトリのフルパス.
Shade3D_PLUGINS_PATH="/Users/UserName/Documents/Shade3D/Shade3D ver.17/plugins"

export DYLD_LIBRARY_PATH="${Shade3D_PLUGINS_PATH}/USDExporter.shdplugin/Contents/Frameworks"

"${Shade3D_APP}/Contents/MacOS/xshade"
