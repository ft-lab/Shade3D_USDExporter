/**
 * エクスポート時のパラメータ.
 */
#include "ExportParam.h"

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
	optOutputAnimation = true;
	optSubdivision = false;
	optMaterialTexturesBake = true;
}
