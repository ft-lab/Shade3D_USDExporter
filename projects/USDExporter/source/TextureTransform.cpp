/**
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
	multiR = multiG = multiB = 1.0f;
	offsetR = offsetG = offsetB = 0.0f;
	flipColor = false;
	occlusion = false;
	textureWeight = 1.0f;
	textureNormal = false;
}

/**
 * 同一パラメータか.
 */
bool CTextureTransform::isSame (const CTextureTransform& v) const
{
	if (!USD_DATA::isZero(v.multiR - multiR) || !USD_DATA::isZero(v.multiG - multiG) || !USD_DATA::isZero(v.multiB - multiB)) return false;
	if (!USD_DATA::isZero(v.offsetR - offsetR) || !USD_DATA::isZero(v.offsetG - offsetG) || !USD_DATA::isZero(v.offsetB - offsetB)) return false;
	if (v.flipColor != flipColor) return false;
	if (v.occlusion != occlusion) return false;
	if (v.textureNormal != textureNormal) return false;
	if (!USD_DATA::isZero(v.textureWeight - textureWeight)) return false;

	return true;
}

/**
 * デフォルトの指定か.
 */
bool CTextureTransform::isDefault () const
{
	if (!flipColor && !occlusion &&
		USD_DATA::isZero(multiR - 1.0) && USD_DATA::isZero(multiG - 1.0) && USD_DATA::isZero(multiB - 1.0) &&
		USD_DATA::isZero(offsetR) && USD_DATA::isZero(offsetG) && USD_DATA::isZero(offsetB) &&
		USD_DATA::isZero(textureWeight - 1.0f)) return true;
	return false;
}

