/**
 * イメージ情報 (USD/Shade3Dで共用).
 */

#ifndef _IMAGEDATA_H
#define _IMAGEDATA_H

#include "USDData.h"
#include "TextureTransform.h"

#include <string>
#include <vector>

//------------------------------------------------------------------.
/**
 * イメージ情報.
 */
class CImageData
{
public:
	std::string fileName;						// イメージファイル名.
	void *pMasterImageHandle;					// Shade3Dのマスターイメージの参照 (ハンドル).

	// USD(19.07)のUsdUVTextureのinputs:bias/inputs.scaleが効かないため、テクスチャにベイク.
	// col' = col * scale + bias の計算をピクセルごとにベイクする.
	float bias[4];
	float scale[4];

	USD_DATA::TEXTURE_SOURE textureSource;	// 使用するテクスチャの要素 (RGB/RGBA/R/G/B/A).

	CTextureTransform texTransform;			// テクスチャの変換要素.

	bool occlusionF;						// オクルージョンとして使用する場合 (scaleが0に近いほど白くする).

public:
	CImageData ();
	~CImageData ();

	CImageData (const CImageData& v) {
		this->fileName = v.fileName;
		this->pMasterImageHandle = v.pMasterImageHandle;

		this->bias[0] = v.bias[0];
		this->bias[1] = v.bias[1];
		this->bias[2] = v.bias[2];
		this->bias[3] = v.bias[3];
		this->scale[0] = v.scale[0];
		this->scale[1] = v.scale[1];
		this->scale[2] = v.scale[2];
		this->scale[3] = v.scale[3];
		this->textureSource = v.textureSource;
		this->texTransform  = v.texTransform;
		this->occlusionF = v.occlusionF;
	}

	CImageData& operator = (const CImageData &v) {
		this->fileName = v.fileName;
		this->pMasterImageHandle = v.pMasterImageHandle;

		this->bias[0] = v.bias[0];
		this->bias[1] = v.bias[1];
		this->bias[2] = v.bias[2];
		this->bias[3] = v.bias[3];
		this->scale[0] = v.scale[0];
		this->scale[1] = v.scale[1];
		this->scale[2] = v.scale[2];
		this->scale[3] = v.scale[3];
		this->textureSource = v.textureSource;
		this->texTransform  = v.texTransform;
		this->occlusionF = v.occlusionF;

		return (*this);
	}

	void clear ();

};

#endif
