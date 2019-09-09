/**
 * マテリアル情報 (USD/Shade3Dで共用).
 */

#ifndef _MATERIALDATA_H
#define _MATERIALDATA_H

#include "USDData.h"
#include "TextureParamData.h"

#include <string>
#include <vector>

//------------------------------------------------------------------.
/**
 * テクスチャマッピング情報.
 */
class CTextureMappingData
{
public:
	std::string fileName;						// テクスチャファイル名.
	USD_DATA::TEXTURE_SOURE textureSource;		// テクスチャの採用要素(RGB/R/G/B).

	CTextureParamData textureParam;				// テクスチャパラメータ.

public:
	CTextureMappingData ();
	~CTextureMappingData ();

	CTextureMappingData (const CTextureMappingData& v) {
		this->fileName      = v.fileName;
		this->textureSource = v.textureSource;
		this->textureParam  = v.textureParam;
	}

    CTextureMappingData& operator = (const CTextureMappingData &v) {
		this->fileName      = v.fileName;
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

	CTextureMappingData diffuseTexture;		// Diffuseテクスチャ (RGB).
	CTextureMappingData normalTexture;		// Normalテクスチャ (RGB).
	CTextureMappingData roughnessTexture;	// Roughnessテクスチャ.
	CTextureMappingData metallicTexture;	// Metallicテクスチャ.
	CTextureMappingData emissiveTexture;	// Emissiveテクスチャ (RGB).
	CTextureMappingData occlusionTexture;	// Occlusionテクスチャ.

	void* pMasterSurfaceHandle;				// Shade3Dでの対応するマスターサーフェスのハンドル.

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


		this->diffuseTexture   = v.diffuseTexture;
		this->normalTexture    = v.normalTexture;
		this->roughnessTexture = v.roughnessTexture;
		this->metallicTexture  = v.metallicTexture;
		this->emissiveTexture  = v.emissiveTexture;
		this->occlusionTexture = v.occlusionTexture;

		this->pMasterSurfaceHandle = v.pMasterSurfaceHandle;
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

		this->diffuseTexture   = v.diffuseTexture;
		this->normalTexture    = v.normalTexture;
		this->roughnessTexture = v.roughnessTexture;
		this->metallicTexture  = v.metallicTexture;
		this->emissiveTexture  = v.emissiveTexture;
		this->occlusionTexture = v.occlusionTexture;

		this->pMasterSurfaceHandle = v.pMasterSurfaceHandle;

		return (*this);
	}

	void clear ();

	/**
	 * 同じパラメータを持つか.
	 */
	bool isSame (const CMaterialData& mDat) const;
};

#endif
