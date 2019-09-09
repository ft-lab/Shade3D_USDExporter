/**
 * テクスチャマッピング情報 (USD/Shade3Dで共用).
 */

#ifndef _TEXTUREPARAMDATA_H
#define _TEXTUREPARAMDATA_H

#include "USDData.h"

#include <string>
#include <vector>

//------------------------------------------------------------------.
/**
 * テクスチャパラメータ.
 */
class CTextureParamData
{
public:
	int uvLayerIndex;				// 割り当てるUVレイヤ番号 (0 or 1).
	bool wrapRepeat;				// テクスチャの繰り返し指定 (trueで繰り返し、falseでclamp).
	int repeatU, repeatV;			// 繰り返し回数.
	int imageIndex;					// 参照する画像番号.

public:
	CTextureParamData ();
	~CTextureParamData ();

	CTextureParamData (const CTextureParamData& v) {
		this->imageIndex = v.imageIndex;
		this->uvLayerIndex = v.uvLayerIndex;
		this->wrapRepeat = v.wrapRepeat;
		this->repeatU = v.repeatU;
		this->repeatV = v.repeatV;
	}

    CTextureParamData& operator = (const CTextureParamData &v) {
		this->imageIndex = v.imageIndex;
		this->uvLayerIndex = v.uvLayerIndex;
		this->wrapRepeat = v.wrapRepeat;
		this->repeatU = v.repeatU;
		this->repeatV = v.repeatV;
		return (*this);
	}

	void clear ();
};

#endif
