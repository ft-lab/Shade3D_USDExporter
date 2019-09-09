/**
 * テクスチャマッピング情報 (USD/Shade3Dで共用).
 */

#include "TextureParamData.h"

//------------------------------------------------------------------.
CTextureParamData::CTextureParamData ()
{
	clear();
}

CTextureParamData::~CTextureParamData ()
{
}

void CTextureParamData::clear ()
{
	imageIndex = -1;
	uvLayerIndex = 0;
	wrapRepeat = true;
	repeatU = 1;
	repeatV = 1;
}
