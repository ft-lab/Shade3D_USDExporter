/**
 *  @file   GlobalHeader.h
 *  @brief  共通して使用する変数など.
 */

#ifndef _GLOBALHEADER_H
#define _GLOBALHEADER_H

#include "sxsdk.cxx"

/**
 * プラグインインターフェイス派生クラスのプラグインID.
 */
#define USD_EXPORTER_INTERFACE_ID sx::uuid_class("C1DDE62B-AC0A-435B-857A-AAF5FB7C5BEA")
#define OCCLUSION_SHADER_INTERFACE_ID sx::uuid_class("2A52E63A-B8BB-4A49-95E7-A8AAD8971FDE")

/**
 * プラグインのバージョン.
 */
#define USD_SHADE3D_PLUGIN_MAJOR_VERSION 0
#define USD_SHADE3D_PLUGIN_MINOR_VERSION 0
#define USD_SHADE3D_PLUGIN_MICRO_VERSION 1
#define USD_SHADE3D_PLUGIN_BUILD_NUMBER  4

/**
 * streamに保存する際のバージョン.
 */
#define OCCLUSION_PARAM_DLG_STREAM_VERSION 0x100		// Occlusion ShaderのStreamバージョン.

#define USD_EXPORTER_DLG_STREAM_VERSION 0x102			// エクスポートダイアログボックスのStreamバージョン.
#define USD_EXPORTER_DLG_STREAM_VERSION_100 0x100
#define USD_EXPORTER_DLG_STREAM_VERSION_101 0x101
#define USD_EXPORTER_DLG_STREAM_VERSION_102 0x102

#endif
