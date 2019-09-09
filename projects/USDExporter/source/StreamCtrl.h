/**
 * streamへの入出力.
 */

#ifndef _STREAMCTRL_H
#define _STREAMCTRL_H

#include "GlobalHeader.h"
#include "OcclusionShaderData.h"
#include "ExportParam.h"

namespace StreamCtrl
{
	/**
	 * Exportダイアログボックスの情報を保存.
	 */
	void saveExportDialogParam (sxsdk::shade_interface& shade, const CExportParam& data);

	/**
	 * Exportダイアログボックスの情報を読み込み.
	 */
	void loadExportDialogParam (sxsdk::shade_interface& shade, CExportParam& data);

	/**
	 * Occlusion Shader(マッピングレイヤ)情報を保存.
	 */
	void saveOcclusionParam (sxsdk::stream_interface* stream, const COcclusionShaderData& data);
	void saveOcclusionParam (sxsdk::mapping_layer_class& mappingLayer, const COcclusionShaderData& data);

	/**
	 * Occlusion Shader(マッピングレイヤ)情報を取得.
	 */
	bool loadOcclusionParam (sxsdk::stream_interface* stream, COcclusionShaderData& data);
	bool loadOcclusionParam (sxsdk::mapping_layer_class& mappingLayer, COcclusionShaderData& data);
}

#endif
