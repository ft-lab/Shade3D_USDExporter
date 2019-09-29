/**
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
	optTextureType = USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_use_image_name;
	optMaxTextureSize = USD_DATA::EXPORT::MAX_TEXTURE_SIZE::texture_size_2048;
	optOutputBoneSkin = true;
	optOutputVertexColor = true;
	optSubdivision = false;

	texOptConvGrayscale     = false;
	texOptBakeMultiTextures = false;

	animKeyframeMode = USD_DATA::EXPORT::ANIM_KEYFRAME_MODE::anim_keyframe_only;
	animStep = 1;
}
