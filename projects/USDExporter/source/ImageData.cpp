/**
 * イメージ情報 (USD/Shade3Dで共用).
 */
#include "ImageData.h"

//------------------------------------------------------------------.
CImageData::CImageData ()
{
	clear();
}

CImageData::~CImageData ()
{
}

void CImageData::clear ()
{
	fileName = "";
	pMasterImageHandle = NULL;

	bias[0] = bias[1] = bias[2] = bias[3] = 0.0f;
	scale[0] = scale[1] = scale[2] = scale[3] = 1.0f;
	textureSource = USD_DATA::TEXTURE_SOURE::texture_source_rgb;
	texTransform.clear();
	occlusionF = false;

	imageWidth = imageHeight = 0;
	rgbaBuff.clear();
}

