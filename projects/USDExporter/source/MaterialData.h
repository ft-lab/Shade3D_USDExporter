/**
 * マテリアル情報 (USD/Shade3Dで共用).
 */

#ifndef _MATERIALDATA_H
#define _MATERIALDATA_H

#include "USDData.h"
#include "TextureParamData.h"
#include "AlphaModeMaterialData.h"

#include <string>
#include <vector>

//------------------------------------------------------------------.
/**
 * テクスチャマッピング情報.
 */
class CTextureMappingData
{
public:
	USD_DATA::TEXTURE_SOURE textureSource;		// テクスチャの採用要素(RGB/R/G/B).

	CTextureParamData textureParam;				// テクスチャパラメータ.

public:
	CTextureMappingData ();
	~CTextureMappingData ();

	CTextureMappingData (const CTextureMappingData& v) {
		this->textureSource = v.textureSource;
		this->textureParam  = v.textureParam;
	}

    CTextureMappingData& operator = (const CTextureMappingData &v) {
		this->textureSource = v.textureSource;
		this->textureParam  = v.textureParam;
		return (*this);
	}

	void clear ();

	/**
	 * 同じパラメータを持つか.
	 */
	bool isSame (const CTextureMappingData& tmDat) const;
};

//------------------------------------------------------------------.
/**
 * マテリアル情報.
 */
class CMaterialData
{
public:
	std::string name;					// マテリアル名.

	float diffuseColor[3];				// Diffuseカラー.
	float emissiveColor[3];				// 発光色.
	float roughness;					// ラフネス値.
	float metallic;						// メタリック値.
	float ior;							// 屈折率.
	float opacity;						// 不透明度.

	bool useDiffuseAlpha;				// Diffuseのアルファを使用する場合.
	bool doubleSided;					// 両面表示する場合はtrue.
	bool unlitMode;						// 「陰影付けしない」がOnの場合、Unlitとして出力.

	CTextureMappingData diffuseTexture;		// Diffuseテクスチャ (RGB).
	CTextureMappingData normalTexture;		// Normalテクスチャ (RGB).
	CTextureMappingData roughnessTexture;	// Roughnessテクスチャ.
	CTextureMappingData metallicTexture;	// Metallicテクスチャ.
	CTextureMappingData emissiveTexture;	// Emissiveテクスチャ (RGB).
	CTextureMappingData occlusionTexture;	// Occlusionテクスチャ.
	CTextureMappingData opacityTexture;		// Opacityテクスチャ(1.0 - Transparency).

	void* pMasterSurfaceHandle;				// Shade3Dでの対応するマスターサーフェスのハンドル.

	CAlphaModeMaterialData alphaModeParam;		// AlphaModeのパラメータ.

	void* pSurface;								// 表面材質(sxsdk::surface_class)のポインタ。これは外部参照の場合に外部参照内のマテリアルの判別を最適化するため.
	int refCount;								// 形状からの参照数.

public:
	CMaterialData ();
	~CMaterialData ();

	CMaterialData (const CMaterialData& v) {
		this->name          = v.name;
		this->diffuseColor[0] = v.diffuseColor[0];
		this->diffuseColor[1] = v.diffuseColor[1];
		this->diffuseColor[2] = v.diffuseColor[2];

		this->emissiveColor[0] = v.emissiveColor[0];
		this->emissiveColor[1] = v.emissiveColor[1];
		this->emissiveColor[2] = v.emissiveColor[2];

		this->roughness     = v.roughness;
		this->metallic      = v.metallic;
		this->ior           = v.ior;
		this->opacity       = v.opacity;
		this->useDiffuseAlpha = v.useDiffuseAlpha;
		this->doubleSided     = v.doubleSided;
		this->unlitMode       = v.unlitMode;

		this->diffuseTexture   = v.diffuseTexture;
		this->normalTexture    = v.normalTexture;
		this->roughnessTexture = v.roughnessTexture;
		this->metallicTexture  = v.metallicTexture;
		this->emissiveTexture  = v.emissiveTexture;
		this->occlusionTexture = v.occlusionTexture;
		this->opacityTexture   = v.opacityTexture;

		this->pMasterSurfaceHandle = v.pMasterSurfaceHandle;
		this->alphaModeParam = v.alphaModeParam;
		this->pSurface = v.pSurface;
		this->refCount = v.refCount;
	}

    CMaterialData& operator = (const CMaterialData &v) {
		this->name          = v.name;

		this->diffuseColor[0] = v.diffuseColor[0];
		this->diffuseColor[1] = v.diffuseColor[1];
		this->diffuseColor[2] = v.diffuseColor[2];

		this->emissiveColor[0] = v.emissiveColor[0];
		this->emissiveColor[1] = v.emissiveColor[1];
		this->emissiveColor[2] = v.emissiveColor[2];

		this->roughness     = v.roughness;
		this->metallic      = v.metallic;
		this->ior           = v.ior;
		this->opacity       = v.opacity;
		this->useDiffuseAlpha = v.useDiffuseAlpha;
		this->doubleSided     = v.doubleSided;
		this->unlitMode       = v.unlitMode;

		this->diffuseTexture   = v.diffuseTexture;
		this->normalTexture    = v.normalTexture;
		this->roughnessTexture = v.roughnessTexture;
		this->metallicTexture  = v.metallicTexture;
		this->emissiveTexture  = v.emissiveTexture;
		this->occlusionTexture = v.occlusionTexture;
		this->opacityTexture   = v.opacityTexture;

		this->pMasterSurfaceHandle = v.pMasterSurfaceHandle;
		this->alphaModeParam = v.alphaModeParam;
		this->pSurface = v.pSurface;
		this->refCount = v.refCount;

		return (*this);
	}

	void clear ();

	/**
	 * 同じパラメータを持つか.
	 */
	bool isSame (const CMaterialData& mDat) const;
};

#endif
