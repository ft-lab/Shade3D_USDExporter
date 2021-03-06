﻿/**
 * テクスチャの2D変換のパラメータ (USD/Shade3Dで共用).
 */
#include "TextureTransform.h"

CTextureTransform::CTextureTransform ()
{
	clear();
}

CTextureTransform::~CTextureTransform ()
{
}

void CTextureTransform::clear ()
{
	multiR = multiG = multiB = multiA = 1.0f;
	offsetR = offsetG = offsetB = offsetA = 0.0f;
	flipColor = false;
	occlusion = false;
	textureWeight = 1.0f;
	textureNormal = false;
	factor[0] = factor[1] = factor[2] = factor[3] = 1.0f;
	convGrayscale = false;
}

/**
 * 同一パラメータか.
 */
bool CTextureTransform::isSame (const CTextureTransform& v) const
{
	if (!USD_DATA::isZero(v.multiR - multiR) || !USD_DATA::isZero(v.multiG - multiG) || !USD_DATA::isZero(v.multiB - multiB) || !USD_DATA::isZero(v.multiA - multiA)) return false;
	if (!USD_DATA::isZero(v.offsetR - offsetR) || !USD_DATA::isZero(v.offsetG - offsetG) || !USD_DATA::isZero(v.offsetB - offsetB) || !USD_DATA::isZero(v.offsetA - offsetA)) return false;
	if (v.flipColor != flipColor) return false;
	//if (v.occlusion != occlusion) return false;
	if (v.textureNormal != textureNormal) return false;
	if (!USD_DATA::isZero(v.textureWeight - textureWeight)) return false;
	if (v.convGrayscale != convGrayscale) return false;

	return true;
}

/**
 * デフォルトの指定か.
 */
bool CTextureTransform::isDefault () const
{
	if (!flipColor && 
		USD_DATA::isZero(multiR - 1.0) && USD_DATA::isZero(multiG - 1.0) && USD_DATA::isZero(multiB - 1.0) && USD_DATA::isZero(multiA - 1.0) &&
		USD_DATA::isZero(offsetR) && USD_DATA::isZero(offsetG) && USD_DATA::isZero(offsetB) && USD_DATA::isZero(offsetA) &&
		USD_DATA::isZero(textureWeight - 1.0f) && !convGrayscale) return true;
	return false;
}

