/**
 * マテリアル情報 (USD/Shade3Dで共用).
 */
#include "MaterialData.h"
#include "MathUtil.h"

//------------------------------------------------------------------.
CTextureMappingData::CTextureMappingData ()
{
	clear();
}

CTextureMappingData::~CTextureMappingData ()
{
}

void CTextureMappingData::clear ()
{
	textureSource = USD_DATA::TEXTURE_SOURE::texture_source_rgb;
	textureParam.clear();
}

/**
 * 同じパラメータを持つか.
 */
bool CTextureMappingData::isSame (const CTextureMappingData& tmDat) const
{
	if (textureSource != tmDat.textureSource) return false;
	if (textureParam.uvLayerIndex != tmDat.textureParam.uvLayerIndex) return false;
	if (textureParam.repeatU != tmDat.textureParam.repeatU) return false;
	if (textureParam.repeatV != tmDat.textureParam.repeatV) return false;
	if (textureParam.wrapRepeat != tmDat.textureParam.wrapRepeat) return false;
	if (textureParam.imageIndex != tmDat.textureParam.imageIndex) return false;

	return true;
}

//------------------------------------------------------------------.
CMaterialData::CMaterialData ()
{
	clear();
}

CMaterialData::~CMaterialData ()
{
}

void CMaterialData::clear ()
{
	name = "";
	diffuseColor[0] = 1.0f;
	diffuseColor[1] = 1.0f;
	diffuseColor[2] = 1.0f;

	emissiveColor[0] = 0.0f;
	emissiveColor[1] = 0.0f;
	emissiveColor[2] = 0.0f;

	roughness = 0.0f;
	metallic  = 0.0f;
	ior       = 1.0f;
	opacity   = 1.0f;
	useDiffuseAlpha = false;
	doubleSided = false;
	unlitMode = false;

	diffuseTexture.clear();
	normalTexture.clear();
	roughnessTexture.clear();
	metallicTexture.clear();
	emissiveTexture.clear();
	occlusionTexture.clear();
	opacityTexture.clear();

	pMasterSurfaceHandle = NULL;
	alphaModeParam.clear();
	pSurface = NULL;
	refCount = 0;

	useTransparency = false;
	transparency = 0.0f;
	transparencyColor[0] = 1.0f;
	transparencyColor[1] = 1.0f;
	transparencyColor[2] = 1.0f;
	transparencyTexture.clear();
}

/**
 * 同じパラメータを持つか.
 */
bool CMaterialData::isSame (const CMaterialData& mDat) const
{
	if (!MathUtil::isZero(diffuseColor[0] - mDat.diffuseColor[0])) return false;
	if (!MathUtil::isZero(diffuseColor[1] - mDat.diffuseColor[1])) return false;
	if (!MathUtil::isZero(diffuseColor[2] - mDat.diffuseColor[2])) return false;
	if (!MathUtil::isZero(emissiveColor[0] - mDat.emissiveColor[0])) return false;
	if (!MathUtil::isZero(emissiveColor[1] - mDat.emissiveColor[1])) return false;
	if (!MathUtil::isZero(emissiveColor[2] - mDat.emissiveColor[2])) return false;
	if (!MathUtil::isZero(roughness - mDat.roughness)) return false;
	if (!MathUtil::isZero(metallic - mDat.metallic)) return false;
	if (!MathUtil::isZero(ior - mDat.ior)) return false;
	if (!MathUtil::isZero(opacity - mDat.opacity)) return false;
	if (useDiffuseAlpha != mDat.useDiffuseAlpha) return false;
	if (doubleSided != mDat.doubleSided) return false;
	if (unlitMode != mDat.unlitMode) return false;

	if (!diffuseTexture.isSame(mDat.diffuseTexture)) return false;
	if (!normalTexture.isSame(mDat.normalTexture)) return false;
	if (!roughnessTexture.isSame(mDat.roughnessTexture)) return false;
	if (!metallicTexture.isSame(mDat.metallicTexture)) return false;
	if (!emissiveTexture.isSame(mDat.emissiveTexture)) return false;
	if (!occlusionTexture.isSame(mDat.occlusionTexture)) return false;
	if (!opacityTexture.isSame(mDat.opacityTexture)) return false;

	if (alphaModeParam.alphaModeType != mDat.alphaModeParam.alphaModeType) return false;
	if (!MathUtil::isZero(alphaModeParam.alphaCutoff - mDat.alphaModeParam.alphaCutoff)) return false;

	if (useTransparency != mDat.useTransparency) return false;
	if (!MathUtil::isZero(useTransparency - mDat.useTransparency)) return false;
	if (!MathUtil::isZero(transparencyColor[0] - mDat.transparencyColor[0])) return false;
	if (!MathUtil::isZero(transparencyColor[1] - mDat.transparencyColor[1])) return false;
	if (!MathUtil::isZero(transparencyColor[2] - mDat.transparencyColor[2])) return false;
	if (!transparencyTexture.isSame(mDat.transparencyTexture)) return false;

	return true;
}
