﻿/**
 *  @file   main.cpp
 *  @brief  USD Exporter.
 */


#include "GlobalHeader.h"
#include "USDExporterInterface.h"
#include "OcclusionShaderInterface.h"
#include "AlphaModeMaterialAttributeInterface.h"

//**************************************************//
//	グローバル関数									//
//**************************************************//
/**
 * プラグインインターフェースの生成.
 */
extern "C" SXSDKEXPORT void STDCALL create_interface (const IID &iid, int i, void **p, sxsdk::shade_interface *shade, void *) {
	unknown_interface *u = NULL;
	
	if (iid == attribute_iid) {
		if (i == 0) {
			u = new CAlphaModeMaterialInterface(*shade);
		}
	}

	if (iid == exporter_iid) {
		if (i == 0) {
			u = new CUSDExporterInterface(*shade);
		}
	}

	if (iid == shader_iid) {
		if (i == 0) {
			u = new COcclusionTextureShaderInterface(*shade);
		}
	}

	if (u) {
		u->AddRef();
		*p = (void *)u;
	}
}

/**
 * インターフェースの数を返す.
 */
extern "C" SXSDKEXPORT int STDCALL has_interface (const IID &iid, sxsdk::shade_interface *shade) {

	if (iid == attribute_iid) return 1;
	if (iid == exporter_iid) return 1;
	if (iid == shader_iid) return 1;

	return 0;
}

/**
 * インターフェース名を返す.
 */
extern "C" SXSDKEXPORT const char * STDCALL get_name (const IID &iid, int i, sxsdk::shade_interface *shade, void *) {
	// SXULより、プラグイン名を取得して渡す.
	if (iid == attribute_iid) {
		if (i == 0) {
			return CAlphaModeMaterialInterface::name(shade);
		}
	}
	if (iid == exporter_iid) {
		if (i == 0) {
			return CUSDExporterInterface::name(shade);
		}
	}
	if (iid == shader_iid) {
		if (i == 0) {
			return COcclusionTextureShaderInterface::name(shade);
		}
	}

	return 0;
}

/**
 * プラグインのUUIDを返す.
 */
extern "C" SXSDKEXPORT sx::uuid_class STDCALL get_uuid (const IID &iid, int i, void *) {
	if (iid == attribute_iid) {
		if (i == 0) {
			return ALPHA_MODE_INTERFACE_ID;
		}
	}
	if (iid == exporter_iid) {
		if (i == 0) {
			return USD_EXPORTER_INTERFACE_ID;
		}
	}
	if (iid == shader_iid) {
		if (i == 0) {
			return OCCLUSION_SHADER_INTERFACE_ID;
		}
	}

	return sx::uuid_class(0, 0, 0, 0);
}

/**
 * バージョン情報.
 */
extern "C" SXSDKEXPORT void STDCALL get_info (sxsdk::shade_plugin_info &info, sxsdk::shade_interface *shade, void *) {
	info.sdk_version = SHADE_BUILD_NUMBER;
	info.recommended_shade_version = 410000;		// ver.16の場合は49xxxx。ver.15の場合は48xxxx.
	info.major_version = USD_SHADE3D_PLUGIN_MAJOR_VERSION;
	info.minor_version = USD_SHADE3D_PLUGIN_MINOR_VERSION;
	info.micro_version = USD_SHADE3D_PLUGIN_MICRO_VERSION;
	info.build_number =  USD_SHADE3D_PLUGIN_BUILD_NUMBER;
}

/**
 * 常駐プラグイン.
 */
extern "C" SXSDKEXPORT bool STDCALL is_resident (const IID &iid, int i, void *) {
	return true;
}

