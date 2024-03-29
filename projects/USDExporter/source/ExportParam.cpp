﻿/**
 * エクスポート時のパラメータ.
 */
#include "ExportParam.h"

/**
 * 最大テクスチャサイズを数値で取得.
 */
int USD_DATA::EXPORT::getTextureSize (const USD_DATA::EXPORT::MAX_TEXTURE_SIZE size) {
	if (size == USD_DATA::EXPORT::texture_size_256) return 256;
	if (size == USD_DATA::EXPORT::texture_size_512) return 512;
	if (size == USD_DATA::EXPORT::texture_size_1024) return 1024;
	if (size == USD_DATA::EXPORT::texture_size_2048) return 2048;
	if (size == USD_DATA::EXPORT::texture_size_4096) return 4096;
	return 1024;
}


CExportParam::CExportParam ()
{
	clear();
}

CExportParam::~CExportParam ()
{
}

void CExportParam::clear ()
{
	exportAppleUSDZ = false;
	exportFileType = USD_DATA::EXPORT::FILE_TYPE::file_type_usdc;
	exportUSDZ = true;
	exportOutputTempFiles = true;
	materialShaderType = USD_DATA::EXPORT::MATERIAL_SHADER_TYPE::material_shader_type_UsdPreviewSurface;
	separateOpacityAndTransmission = false;

	optTextureType = USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_use_image_name;
	optMaxTextureSize = USD_DATA::EXPORT::MAX_TEXTURE_SIZE::texture_size_2048;
	optOutputBoneSkin = true;
	optOutputVertexColor = true;
	optSubdivision = false;
	optDividePolyTriQuad = true;
	optDividePolyTri = false;
	optKind = USD_DATA::EXPORT::KIND_TYPE::kind_none;

	texOptConvGrayscale     = false;
	bakeWithoutProcessingTextures = false;
	grayscaleTexturesColorSpace = USD_DATA::EXPORT::TEXTURE_COLOR_SPACE::texture_colorspace_raw;

	animKeyframeMode = USD_DATA::EXPORT::ANIM_KEYFRAME_MODE::anim_keyframe_only;
	animStep = 3;
}

/**
 * Shaderの種類としてMDLを使用するか.
 * これは、Macのusdz出力ではないこと、materialShaderTypeでmaterial_shader_type_NVIDIA_MDL_omniverseが選択されていることが条件.
 */
bool CExportParam::useShaderMDL () const
{
	if (materialShaderType == USD_DATA::EXPORT::MATERIAL_SHADER_TYPE::material_shader_type_UsdPreviewSurface) return false;
	if (exportAppleUSDZ) return false;

	return true;
}

