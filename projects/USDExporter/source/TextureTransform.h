/**
 * テクスチャの2D変換のパラメータ (USD/Shade3Dで共用).
 */

#ifndef _TEXTURETRANSFORM_H
#define _TEXTURETRANSFORM_H

#include "USDData.h"

#include <string>
#include <vector>

//------------------------------------------------------------------.
class CTextureTransform
{
public:
	float multiR, multiG, multiB;		// 色に乗算.
	float offsetR, offsetG, offsetB;	// 色に加算.
	bool flipColor;						// 色反転.
	bool occlusion;						// オクルージョンレイヤの場合.
	bool textureNormal;					// 法線マップの場合.
	float textureWeight;				// テクスチャマッピングでのウエイト値 (0.0に近づくほどテクスチャの影響が弱くなる).

public:
	CTextureTransform ();
	~CTextureTransform ();

	CTextureTransform (const CTextureTransform& v) {
		this->flipColor = v.flipColor;
		this->multiR    = v.multiR;
		this->multiG    = v.multiG;
		this->multiB    = v.multiB;
		this->offsetR   = v.offsetR;
		this->offsetG   = v.offsetG;
		this->offsetB   = v.offsetB;
		this->occlusion = v.occlusion;
		this->textureWeight = v.textureWeight;
		this->textureNormal = v.textureNormal;
	}

    CTextureTransform& operator = (const CTextureTransform &v) {
		this->flipColor = v.flipColor;
		this->multiR    = v.multiR;
		this->multiG    = v.multiG;
		this->multiB    = v.multiB;
		this->offsetR   = v.offsetR;
		this->offsetG   = v.offsetG;
		this->offsetB   = v.offsetB;
		this->occlusion = v.occlusion;
		this->textureWeight = v.textureWeight;
		this->textureNormal = v.textureNormal;
		return (*this);
	}

	void clear ();

	/**
	 * 同一パラメータか.
	 */
	bool isSame (const CTextureTransform& v) const;

	/**
	 * デフォルトの指定か.
	 */
	bool isDefault () const;
};

#endif
