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

// アルファモードの指定はglTF Converterと共有.
#define ALPHA_MODE_INTERFACE_ID sx::uuid_class("F92DECA6-AC2C-4B6C-8C89-A3D2AD0F6A43")

/**
 * プラグインのバージョン.
 */
#define USD_SHADE3D_PLUGIN_MAJOR_VERSION 0
#define USD_SHADE3D_PLUGIN_MINOR_VERSION 1
#define USD_SHADE3D_PLUGIN_MICRO_VERSION 0
#define USD_SHADE3D_PLUGIN_BUILD_NUMBER  0

/**
 * streamに保存する際のバージョン.
 */
#define OCCLUSION_PARAM_DLG_STREAM_VERSION 0x100		// Occlusion ShaderのStreamバージョン.

#define USD_EXPORTER_DLG_STREAM_VERSION 0x103			// エクスポートダイアログボックスのStreamバージョン.
#define USD_EXPORTER_DLG_STREAM_VERSION_100 0x100
#define USD_EXPORTER_DLG_STREAM_VERSION_101 0x101
#define USD_EXPORTER_DLG_STREAM_VERSION_102 0x102
#define USD_EXPORTER_DLG_STREAM_VERSION_103 0x103

#define ALPHA_MODE_DLG_STREAM_VERSION			0x100

#define MAPPING_TYPE_OPACITY  ((sxsdk::enums::mapping_type)24)				// sxsdk::enums::mapping_typeでの「不透明マスク」.
#define MAPPING_TYPE_USD_OCCLUSION  ((sxsdk::enums::mapping_type)1001)		// 「オクルージョン」これはUSDで割り当てたカスタムの種類.

#endif
