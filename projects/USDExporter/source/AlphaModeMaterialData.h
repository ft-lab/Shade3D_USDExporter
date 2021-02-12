/**
 * アニメーション情報.
 */

#ifndef _ALPHAMODEPARAMDATA_H
#define _ALPHAMODEPARAMDATA_H

#include "USDData.h"

/**
 * AlphaModeの情報.
 * これは、表面材質の属性として指定.
 */
class CAlphaModeMaterialData
{
public:
	CommonParam::alpha_mode_type alphaModeType;		// AlphaModeの種類.
	float alphaCutoff;

	CAlphaModeMaterialData () {
		clear();
	}
	~CAlphaModeMaterialData () { }

	void clear () {
		alphaModeType = CommonParam::alpha_mode_opaque;
		alphaCutoff   = 0.5f;
	}

	CAlphaModeMaterialData (const CAlphaModeMaterialData& v) {
		this->alphaModeType = v.alphaModeType;
		this->alphaCutoff   = v.alphaCutoff;
	}

	CAlphaModeMaterialData& operator = (const CAlphaModeMaterialData &v) {
		this->alphaModeType = v.alphaModeType;
		this->alphaCutoff   = v.alphaCutoff;
		return (*this);
	}
};

#endif
