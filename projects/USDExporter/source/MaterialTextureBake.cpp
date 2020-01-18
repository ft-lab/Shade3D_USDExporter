﻿/**
 * Shade3Dの表面材質より、テクスチャをPBR向けにベイク.
 * このクラス内に他とかぶらないようにしたテクスチャイメージのリストを持つ.
 */
#include "MaterialTextureBake.h"
#include "Shade3DUtil.h"
#include "StringUtil.h"
#include "StreamCtrl.h"
#include "MathUtil.h"
#include "ImagesBlend.h"
#include "OcclusionShaderData.h"

/*
	＜＜ Memo ＞＞

	Shade3Dの表面材質からのマテリアル情報の取得。
	基本設定は以下。

	diffuseColor  = 拡散反射値 x 拡散反射色
	emissiveColor = 発光値 x 発光色
	metallic      = 反射値
	roughness     = 荒さ
	opacity       = 1.0 - 透明
	ior           = 屈折率

	マッピングレイヤは、単純な1テクスチャのみを使用するものと、複数をベイクするモードを設けている。.

*/

#define MATERIAL_ROOT_PATH  "/root/Materials"

//------------------------------------------------------------------.
CImageRefData::CImageRefData ()
{
}

CImageRefData::~CImageRefData ()
{
	clear();
}

void CImageRefData::clear ()
{
	pMasterImageHandle = NULL;
	useOrg = true;
}

//------------------------------------------------------------------.
CCheckImageRef::CCheckImageRef ()
{
	clear();
}

void CCheckImageRef::clear ()
{
	m_imageRefData.clear();
}

/**
 * シーンを走査し、マスターイメージの参照情報を取得.
 */
void CCheckImageRef::checkMasterImages (sxsdk::scene_interface* scene)
{
	clear();
	m_pScene = scene;

	sxsdk::shape_class* pRootShape = &(scene->get_shape());
	m_checkMasterImages(pRootShape);
}

/**
 * 形状を再帰的に走査し、マスターイメージの参照情報を取得.
 */
void CCheckImageRef::m_checkMasterImages (sxsdk::shape_class* shape)
{
	if (shape->has_surface()) {
		sxsdk::surface_class* surface = shape->get_surface();
		m_checkMappingInSurface(surface);
	}

	// 再帰的にたどる.
	if (shape->has_son()) {
		sxsdk::shape_class* pS = shape->get_son();
		while (pS->has_bro()) {
			pS = pS->get_bro();
			if (!pS) break;
			m_checkMasterImages(pS);
		}
	}
}

/**
 * 指定の表面材質のマッピングレイヤにて、参照するマスターイメージをたどる.
 */
void CCheckImageRef::m_checkMappingInSurface (sxsdk::surface_class* surface)
{
	const int mappingLayerCou = surface->get_number_of_mapping_layers();
	if (mappingLayerCou == 0) return;

	for (int i = 0; i < mappingLayerCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = surface->mapping_layer(i);
		if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;

		const int mType = mappingLayer.get_type();
		CTextureTransform texTransform;
		texTransform.flipColor     = mappingLayer.get_flip_color();
		texTransform.textureWeight = mappingLayer.get_weight();

		// マテリアルの乗算値を取得.
		if (mType == sxsdk::enums::diffuse_mapping) {
			if (surface->get_has_diffuse()) {
				const sxsdk::rgb_class col = (surface->get_diffuse_color()) * (surface->get_diffuse());
				texTransform.multiR = col.red;
				texTransform.multiG = col.green;
				texTransform.multiB = col.blue;
			}
		}
		if (mType == sxsdk::enums::glow_mapping) {
			if (surface->get_has_glow()) {
				const sxsdk::rgb_class col = (surface->get_glow_color()) * (surface->get_glow());
				texTransform.multiR = col.red;
				texTransform.multiG = col.green;
				texTransform.multiB = col.blue;
			}
		}
		if (mType == sxsdk::enums::reflection_mapping) {
			if (surface->get_has_reflection()) {
				texTransform.multiR = texTransform.multiG = texTransform.multiB =surface->get_reflection();
			}
		}
		if (mType == sxsdk::enums::roughness_mapping) {
			if (surface->get_has_roughness()) {
				texTransform.multiR = texTransform.multiG = texTransform.multiB = surface->get_roughness();
			}
		}
		if (mType == sxsdk::enums::transparency_mapping) {
			if (surface->get_has_transparency()) {
				texTransform.multiR = texTransform.multiG = texTransform.multiB = surface->get_transparency();
			}
			texTransform.flipColor = !texTransform.flipColor;
		}

		if (mType != sxsdk::enums::diffuse_mapping && mType != sxsdk::enums::glow_mapping &&
			mType != sxsdk::enums::reflection_mapping && mType != sxsdk::enums::roughness_mapping &&
			mType != sxsdk::enums::transparency_mapping && !Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) continue;

		try {
			compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
			if (image && image->has_image()) {
				sxsdk::master_image_class* masterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);
				if (masterImage) {
					void* imgHandle = masterImage->get_handle();

					int index = -1;
					for (size_t j = 0; j < m_imageRefData.size(); ++j) {
						if (m_imageRefData[j].pMasterImageHandle == imgHandle) {
							index = (int)j;
							break;
						}
					}
					if (index < 0) {
						index = (int)m_imageRefData.size();
						CImageRefData dd;
						dd.pMasterImageHandle = imgHandle;
						dd.useOrg = true;
						m_imageRefData.push_back(dd);
					}
					CImageRefData& imgD = m_imageRefData[index];
					if (!imgD.useOrg) continue;

					if (!texTransform.isDefault()) {
						imgD.useOrg = false;
						continue;
					}
				}
			}
		} catch (...) { }
	}
}

/**
 * 指定のマスターイメージがオリジナルのまま加工せずに使用できるか.
 */
bool CCheckImageRef::isNoProcessingImage (sxsdk::master_image_class* masterImage)
{
	if (!masterImage) return false;
	if (m_imageRefData.empty()) return false;

	void* imgHandle = masterImage->get_handle();

	int index = -1;
	for (size_t j = 0; j < m_imageRefData.size(); ++j) {
		if (m_imageRefData[j].pMasterImageHandle == imgHandle) {
			index = (int)j;
			break;
		}
	}
	if (index < 0) return false;

	return m_imageRefData[index].useOrg;
}

//------------------------------------------------------------------.
CMaterialTextureBake::CMaterialTextureBake (sxsdk::scene_interface* scene, const CExportParam& exportParam)
{
	m_pScene = scene;
	m_exportParam = exportParam;
	m_checkImageRef.checkMasterImages(m_pScene);

	clear();
}

CMaterialTextureBake::~CMaterialTextureBake ()
{
}

void CMaterialTextureBake::clear ()
{
	m_imagesList.clear();
	m_findImageFileNames.clear();
}

/**
 * 指定の形状でマテリアルを取得.
 * @param[in]  shape         対象の形状.
 * @param[out] materialData  マテリアル情報が返る.
 */
bool CMaterialTextureBake::getMaterialDataFromShape (sxsdk::shape_class* shape, CMaterialData& materialData)
{
	materialData.clear();
	materialData.name = std::string(MATERIAL_ROOT_PATH) + std::string("/material");

	if (!shape) return false;

	// 表面材質情報を持つ形状をたどる (なければshape自身).
	sxsdk::shape_class* pS = Shade3DUtil::getHasSurfaceParentShape(shape);
	if (!pS) return false;
	if (!(pS->has_surface())) return false;

	sxsdk::master_surface_class* pMasterSurface = pS->get_master_surface();

	return m_getMaterialDataFromShape(pMasterSurface, pS->get_surface(), materialData);
}

/**
 * 指定のマスターサーフェスから、表面材質情報を取得.
 * @param[in]  masterSurface  対象のマスターサーフェス.
 * @param[in]  surface        masterSurfaceがnullの場合はこのsurfaceを参照する.
 * @param[out] materialDat   マテリアル情報が返る.
 * @return マテリアル情報を取得した場合はtrue.
 */
bool CMaterialTextureBake::getMaterialFromMasterSurface (sxsdk::master_surface_class* masterSurface, sxsdk::surface_class* surface, CMaterialData& materialData)
{
	materialData.clear();
	materialData.name = std::string(MATERIAL_ROOT_PATH) + std::string("/material");

	return m_getMaterialDataFromShape(masterSurface, NULL, materialData);
}


/**
 * 指定の形状でマテリアルを取得.
 * @param[in]  masterSurface     マスターサーフェスクラス.
 * @param[in]  surface           表面材質クラス.
 * @param[out] materialData  マテリアル情報が返る.
 */
bool CMaterialTextureBake::m_getMaterialDataFromShape (sxsdk::master_surface_class* masterSurface, sxsdk::surface_class* surface, CMaterialData& materialData)
{
	try {
		// マスターサーフェス名を取得.
		if (masterSurface) {
			std::string mName = std::string(masterSurface->get_name());
			materialData.name = std::string(MATERIAL_ROOT_PATH) + std::string("/") + mName;
			materialData.pMasterSurfaceHandle = masterSurface->get_handle();

			// doublesidedが含まれる場合は、doubleSidedを持つとする.
			std::transform(mName.begin(), mName.end(), mName.begin(), ::tolower);
			if (mName.find("doublesided") != std::string::npos) {
				materialData.doubleSided = true;
			}
			surface = masterSurface->get_surface();
		}
		if (!surface) return false;

		// 「陰影付けしない」.
		materialData.unlitMode = surface->get_no_shading();

		// 表面材質情報を取得.
		if (surface->get_has_diffuse()) {
			const sxsdk::rgb_class col = (surface->get_diffuse_color()) * (surface->get_diffuse());
			materialData.diffuseColor[0] = col.red;
			materialData.diffuseColor[1] = col.green;
			materialData.diffuseColor[2] = col.blue;
		}

		if (surface->get_has_glow()) {
			const sxsdk::rgb_class col = (surface->get_glow_color()) * (surface->get_glow());
			materialData.emissiveColor[0] = col.red;
			materialData.emissiveColor[1] = col.green;
			materialData.emissiveColor[2] = col.blue;
		}

		if (surface->get_has_reflection()) {
			materialData.metallic = surface->get_reflection();
		}

		if (surface->get_has_roughness()) {
			materialData.roughness = surface->get_roughness();
		}
		if (surface->get_has_transparency()) {
			materialData.opacity = 1.0f - (surface->get_transparency());
		}
		materialData.ior = surface->get_refraction();

		// マッピングレイヤ情報を取得.
		bool retF = false;
		if (!m_exportParam.texOptBakeMultiTextures) {
			// 単純なベイクを行う.
			retF = m_getSimpleMaterialMappingFromSurface(surface, materialData);
		} else {
			// 複数マッピングを合成.
			retF = m_getMaterialMultiMappingFromSurface(surface, materialData);
		}

		// Opacityテクスチャが存在しrougnhessを1.0にしている場合、半透明を完全透明に近づけるにはmetallicを1.0にする (iOS13.1).
		if (materialData.opacityTexture.textureParam.imageIndex >= 0) {
			if (materialData.roughnessTexture.textureParam.imageIndex < 0 && USD_DATA::isZero(materialData.roughness - 1.0f)) {
				if (materialData.metallicTexture.textureParam.imageIndex < 0) {
					materialData.metallic = 1.0f;
				}
			}
		}

		// 「陰影付けしない」を考慮して、Unlitになるように置き換え.
		// iOS 13.1では、roughness 1 / metallic 1 にすると、完全透明部分は透過になっている.
		if (materialData.unlitMode) {
			materialData.roughness = 1.0f;
			materialData.metallic  = 1.0f;
			materialData.ior       = 1.0f;
			materialData.metallicTexture.clear();
			materialData.roughnessTexture.clear();
			materialData.normalTexture.clear();

			materialData.emissiveColor[0] = materialData.diffuseColor[0];
			materialData.emissiveColor[1] = materialData.diffuseColor[1];
			materialData.emissiveColor[2] = materialData.diffuseColor[2];

			materialData.diffuseColor[0] = 0.0f;
			materialData.diffuseColor[1] = 0.0f;
			materialData.diffuseColor[2] = 0.0f;
			if (materialData.diffuseTexture.textureParam.imageIndex >= 0) {
				materialData.emissiveTexture = materialData.diffuseTexture;
			}
			materialData.diffuseTexture.clear();
		}

		return retF;

	} catch (...) { }

	return false;
}

/**
 * マッピングレイヤで、単純に1テクスチャで構成.
 * @param[in]  surface           表面材質クラス.
 * @param[out] materialData  マテリアル情報が返る.
 */
bool CMaterialTextureBake::m_getSimpleMaterialMappingFromSurface (sxsdk::surface_class* surface, CMaterialData& materialData)
{
	try {
		// マッピングレイヤ情報を取得.
		const int mappingLayersCou = surface->get_number_of_mapping_layers();
		if (!(surface->get_has_mapping_layers()) || mappingLayersCou == 0) return true;

		bool useTransparencyTexture = false;
		bool useEmissiveTexture     = false;
		for (int mLoop = 0; mLoop < mappingLayersCou; ++mLoop) {
			sxsdk::mapping_layer_class& mappingLayer = surface->mapping_layer(mLoop);
			const int mType = mappingLayer.get_type();
			if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
			if (mappingLayer.get_weight() < 0.0001f) continue;

			if (mType == sxsdk::enums::transparency_mapping) {		// 透明度テクスチャを持つ.
				compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
				if (image && image->has_image()) {
					sxsdk::master_image_class* masterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);
					if (masterImage) {
						useTransparencyTexture = true;
					}
				}
			}

			if (mType == sxsdk::enums::glow_mapping) {		// 発光テクスチャを持つ.
				compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
				if (image && image->has_image()) {
					sxsdk::master_image_class* masterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);
					if (masterImage) {
						useEmissiveTexture = true;
					}
				}
			}
		}

		// 「アルファ透明」を使用し、かつ、「透明度」マッピングが存在しない場合は、.
		// USD出力時にDiffuseテクスチャを強制的にpngで出力する.
		bool useTransparentAlpha = false;
		sxsdk::mapping_layer_class* diffuseMappingLayer = NULL;
		for (int mLoop = 0; mLoop < mappingLayersCou; ++mLoop) {
			sxsdk::mapping_layer_class& mappingLayer = surface->mapping_layer(mLoop);
			const int mType = mappingLayer.get_type();
			if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;

			if (mType == sxsdk::enums::diffuse_mapping) {
				// アルファ透明を使用.
				if (mappingLayer.get_channel_mix() == sxsdk::enums::mapping_transparent_alpha_mode) {
					try {
						compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
						if (image && image->has_image()) {
							sxsdk::master_image_class* masterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);
							if (masterImage) {
								if (Shade3DUtil::hasImageAlpha(masterImage)) {
									diffuseMappingLayer = &mappingLayer;
									useTransparentAlpha = true;
								}
							}
						}
					} catch (...) { }
				}
			}
		}
		if (useTransparencyTexture) {
			useTransparentAlpha = false;
			diffuseMappingLayer = NULL;
		}
		if (useTransparentAlpha) {
			materialData.useDiffuseAlpha = true;
		}

		// テクスチャを取得.
		for (int mLoop = 0; mLoop < mappingLayersCou; ++mLoop) {
			sxsdk::mapping_layer_class& mappingLayer = surface->mapping_layer(mLoop);
			const int mType = mappingLayer.get_type();
			CTextureTransform texTransform;
			texTransform.flipColor     = mappingLayer.get_flip_color();
			texTransform.textureWeight = mappingLayer.get_weight();
			texTransform.factor[0] = texTransform.factor[1] = texTransform.factor[2] = texTransform.factor[3] = 1.0f;

			// オクルージョンレイヤの場合.
			if (Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) {
				texTransform.occlusion = true;
				m_setTextureMappingData(mappingLayer, texTransform, materialData.occlusionTexture, true, false, useEmissiveTexture);
			}

			if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;

			if (mType == sxsdk::enums::diffuse_mapping) {
				texTransform.multiR = materialData.diffuseColor[0];
				texTransform.multiG = materialData.diffuseColor[1];
				texTransform.multiB = materialData.diffuseColor[2];
				m_setTextureMappingData(mappingLayer, texTransform, materialData.diffuseTexture, false, materialData.useDiffuseAlpha, useEmissiveTexture);
			}

			if (mType == sxsdk::enums::glow_mapping) {
				texTransform.multiR = materialData.emissiveColor[0];
				texTransform.multiG = materialData.emissiveColor[1];
				texTransform.multiB = materialData.emissiveColor[2];
				m_setTextureMappingData(mappingLayer, texTransform, materialData.emissiveTexture, false, false, useEmissiveTexture);
			}
			if (mType == sxsdk::enums::normal_mapping) {
				const float weight = mappingLayer.get_weight();
				texTransform.textureNormal = true;
				m_setTextureMappingData(mappingLayer, texTransform, materialData.normalTexture, false, false, useEmissiveTexture);
			}
			if (mType == sxsdk::enums::roughness_mapping) {
				texTransform.multiR = texTransform.multiG = texTransform.multiB = materialData.roughness;
				m_setTextureMappingData(mappingLayer, texTransform, materialData.roughnessTexture, false, false, useEmissiveTexture);
			}
			if (mType == sxsdk::enums::reflection_mapping) {
				texTransform.multiR = texTransform.multiG = texTransform.multiB = materialData.metallic;
				m_setTextureMappingData(mappingLayer, texTransform, materialData.metallicTexture, false, false, useEmissiveTexture);
			}
			if (mType == sxsdk::enums::transparency_mapping) {
				texTransform.multiR = texTransform.multiG = texTransform.multiB = 1.0f;
				texTransform.flipColor = !texTransform.flipColor;		// 透明度とOpacityは逆になる.
				texTransform.factor[0] = texTransform.factor[1] = texTransform.factor[2] = texTransform.factor[3] = materialData.opacity;
				m_setTextureMappingData(mappingLayer, texTransform, materialData.opacityTexture, false, false, useEmissiveTexture);
			}
		}

		// アルファ透明を使用し、透明度マップが存在しない場合.
		if (diffuseMappingLayer && materialData.useDiffuseAlpha && !useTransparencyTexture) {
			materialData.opacity = 0.0f;
			CTextureTransform texTransform;
			texTransform.multiR = texTransform.multiG = texTransform.multiB = 1.0f;
			//texTransform.flipColor = true;		// 透明度とOpacityは逆になる.
			texTransform.factor[0] = texTransform.factor[1] = texTransform.factor[2] = texTransform.factor[3] = materialData.opacity;
			m_setTextureMappingOpacityData(*diffuseMappingLayer, texTransform, materialData.opacityTexture, useEmissiveTexture);
		}

		return true;

	} catch (...) { }

	return false;
}

/**
 * テクスチャマッピング情報を追加.
 * @param[in]  mappingLayer    マッピングレイヤ情報.
 * @param[in/out]  texTransform    マッピングの変換情報.
 * @param[out] texMappingData  マッピング情報の格納先.
 * @param[in]  occlusionF           Occlusionのテクスチャか.
 * @param[in]  useTransparentAlpha  アルファ透明を使用するか.
 * @param[in]  useEmissiveTexture   発光テクスチャを使用するか.
 */
void CMaterialTextureBake::m_setTextureMappingData (sxsdk::mapping_layer_class& mappingLayer, CTextureTransform& texTransform, CTextureMappingData& texMappingData, const bool occlusionF, const bool useTransparentAlpha, const bool useEmissiveTexture)
{
	if (texMappingData.textureParam.imageIndex >= 0) return;

	try {
		compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
		if (!image) return;
		if (!image->has_image()) return;

		// imageからマスターイメージを取得.
		sxsdk::master_image_class* masterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);
		if (!masterImage) return;

		std::string masterImageName = "";
		int channelMix = mappingLayer.get_channel_mix();

		// Occlusion情報を取得.
		COcclusionShaderData occlusionD;
		if (StreamCtrl::loadOcclusionParam(mappingLayer, occlusionD)) {
			switch (occlusionD.channelMix) {
			case 0:
				channelMix = sxsdk::enums::mapping_grayscale_red_mode;
				break;
			case 1:
				channelMix = sxsdk::enums::mapping_grayscale_green_mode;
				break;
			case 2:
				channelMix = sxsdk::enums::mapping_grayscale_blue_mode;
				break;
			case 3:
				channelMix = sxsdk::enums::mapping_grayscale_alpha_mode;
				break;
			}
		}

		// オリジナルの画像を加工なしでそのまま使えるかチェック.
		const bool isNoProcessingF = m_checkImageRef.isNoProcessingImage(masterImage);

		if (channelMix == sxsdk::enums::mapping_grayscale_red_mode || channelMix == sxsdk::enums::mapping_grayscale_green_mode ||
			channelMix == sxsdk::enums::mapping_grayscale_blue_mode || channelMix == sxsdk::enums::mapping_grayscale_alpha_mode) {
			// 発光がある場合は、強制的にAlpha要素をグレイスケールで出さないと、iOS13.1.2段階のusdz表示では正しくならない.
			const int mType = mappingLayer.get_type();
			if (m_exportParam.texOptConvGrayscale || (useEmissiveTexture && mType == sxsdk::enums::transparency_mapping)) {
				texTransform.convGrayscale = true;
			}

			if (!isNoProcessingF) {
				texTransform.convGrayscale = true;
			}
		}

		// 同一のマスターイメージが格納済みか.
		int imageIndex = m_findMasterImageInImagesList(masterImage, texTransform, channelMix);
		if (imageIndex >= 0) {
			const CImageData& imageD = m_imagesList[imageIndex];
			masterImageName = imageD.fileName;
		} else {
			// イメージ名をASCIIのファイル名にする.
			masterImageName = masterImage->get_name();
			if (!StringUtil::checkASCII(masterImageName)) {
				masterImageName = "texture";
			}

			if (useTransparentAlpha && isNoProcessingF) {
				// 「アルファ透明」使用時でデフォルトのパラメータの時は、強制的にpng出力.
				masterImageName = StringUtil::SetFileImageExtension(masterImageName, "png", true);
			} else {
				// エクスポートオプションm_exportParam.optTextureTypeにより、pngにするかjpgにするか決める.
				if (m_exportParam.optTextureType == USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_use_image_name) {
					// 拡張子がある場合はそれを採用し、ない場合はpngにする.
					masterImageName = StringUtil::SetFileImageExtension(masterImageName, "png");
				} else {
					const bool usePng = (m_exportParam.optTextureType == USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_replace_png);
					masterImageName = StringUtil::SetFileImageExtension(masterImageName, usePng ? "png" : "jpg", true);
				}
			}

			// ユニークファイル名として追加.
			// 同一名がある場合は、連番付きで返す.
			masterImageName = m_findImageFileNames.appendName(masterImageName, USD_DATA::NODE_TYPE::texture_node, true);

			imageIndex = (int)m_imagesList.size();
			m_imagesList.push_back(CImageData());
		}

		if (imageIndex >= 0) {
			CImageData& imageD = m_imagesList[imageIndex];
			imageD.pMasterImageHandle = masterImage->get_handle();
			imageD.fileName = masterImageName;
			imageD.occlusionF = occlusionF;

			texMappingData.textureParam.imageIndex = imageIndex;
			texMappingData.textureParam.uvLayerIndex = mappingLayer.get_uv_mapping();
			if (texMappingData.textureParam.uvLayerIndex < 0 || texMappingData.textureParam.uvLayerIndex > 1) {
				texMappingData.textureParam.uvLayerIndex = 0;
			}

			int occlusionChannelMix = 0;
			if (occlusionF) {
				texMappingData.textureParam.uvLayerIndex = occlusionD.uvIndex;
				occlusionChannelMix = occlusionD.channelMix;		// チャンネル合成（0:Red、1:Green、2:Blue、3:Alpha）.
			}

			texMappingData.textureParam.repeatU    = mappingLayer.get_repetition_x();
			texMappingData.textureParam.repeatV    = mappingLayer.get_repetition_y();
			texMappingData.textureParam.wrapRepeat = mappingLayer.get_repeat_image();

			bool useAlpha = false;

			switch (channelMix) {
			case sxsdk::enums::mapping_grayscale_red_mode:
				texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_r;
				imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_r;
				break;

			case sxsdk::enums::mapping_grayscale_green_mode:
				texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_g;
				imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_g;
				break;

			case sxsdk::enums::mapping_grayscale_blue_mode:
				texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_b;
				imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_b;
				break;

			case sxsdk::enums::mapping_grayscale_alpha_mode:
				texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_a;
				imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_a;
				useAlpha = true;
				break;

			default:
				imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_rgb;
			}

			if (occlusionF) {
				switch (occlusionChannelMix) {
				case 0:
					texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_r;
					imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_r;
					break;
				case 1:
					texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_g;
					imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_g;
					break;
				case 2:
					texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_b;
					imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_b;
					break;
				case 3:
					texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_a;
					imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_a;
					useAlpha = true;
					break;
				}
			}

			// アルファ要素を持つ場合でファイル名がjpgの場合は、pngに置き換え.
			if (useAlpha && !texTransform.convGrayscale && isNoProcessingF) {
				if (StringUtil::getFileExtension(masterImageName) == "jpg") {
					masterImageName = StringUtil::SetFileImageExtension(masterImageName, "png", true);
					masterImageName = m_findImageFileNames.appendName(masterImageName, USD_DATA::NODE_TYPE::texture_node, true);
					imageD.fileName = masterImageName;
				}
			}

			if (imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_rgb) {
				const int mType = mappingLayer.get_type();
				if (mType == sxsdk::enums::roughness_mapping || mType == sxsdk::enums::reflection_mapping || mType == sxsdk::enums::transparency_mapping) {
					imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_r;
				}
			}

			imageD.texTransform = texTransform;
		}
	} catch (...) { }
}

/**
 * アルファ透明使用時の、テクスチャマッピング情報を追加.
 * @param[in]  mappingLayer    マッピングレイヤ情報.
 * @param[in]  texTransform    マッピングの変換情報.
 * @param[out] texMappingData  マッピング情報の格納先.
 * @param[in]  useEmissiveTexture   発光テクスチャを使用するか.
 */
void CMaterialTextureBake::m_setTextureMappingOpacityData (sxsdk::mapping_layer_class& diffuseMappingLayer, CTextureTransform& texTransform, CTextureMappingData& texMappingData, const bool useEmissiveTexture)
{
	if (texMappingData.textureParam.imageIndex >= 0) return;

	try {
		compointer<sxsdk::image_interface> image(diffuseMappingLayer.get_image_interface());
		if (!image) return;
		if (!image->has_image()) return;

		// imageからマスターイメージを取得.
		sxsdk::master_image_class* masterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);
		if (!masterImage) return;

		std::string masterImageName = "";

		// 発光がある場合は、強制的にAlpha要素をグレイスケールで出さないと、iOS13.1.2段階のusdz表示では正しくならない.
		if (useEmissiveTexture || m_exportParam.texOptConvGrayscale) {
			texTransform.convGrayscale = true;
		}

		// オリジナルの画像を加工なしでそのまま使えるかチェック.
		const bool isNoProcessingF = m_checkImageRef.isNoProcessingImage(masterImage);
		if (!isNoProcessingF) {
			texTransform.convGrayscale = true;
		}

		// 同一のマスターイメージが格納済みか.
		int imageIndex = m_findMasterImageInImagesList(masterImage, texTransform, sxsdk::enums::mapping_grayscale_alpha_mode);
		bool existImage = false;
		if (imageIndex >= 0) {
			const CImageData& imageD = m_imagesList[imageIndex];
			if (!texTransform.convGrayscale) {
				if (StringUtil::getFileExtension(imageD.fileName) == "png") {
					masterImageName = imageD.fileName;
					existImage = true;
				}
			}
		}

		if (!existImage) {
			// イメージ名をASCIIのファイル名にする.
			masterImageName = masterImage->get_name();
			if (!StringUtil::checkASCII(masterImageName)) {
				masterImageName = "texture";
			}
			texTransform.convGrayscale = true;

			// エクスポートオプションm_exportParam.optTextureTypeにより、pngにするかjpgにするか決める.
			if (m_exportParam.optTextureType == USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_use_image_name) {
				// 拡張子がある場合はそれを採用し、ない場合はpngにする.
				masterImageName = StringUtil::SetFileImageExtension(masterImageName, "png");
			} else {
				const bool usePng = (m_exportParam.optTextureType == USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_replace_png);
				masterImageName = StringUtil::SetFileImageExtension(masterImageName, usePng ? "png" : "jpg", true);
			}

			// ユニークファイル名として追加.
			// 同一名がある場合は、連番付きで返す.
			masterImageName = m_findImageFileNames.appendName(masterImageName, USD_DATA::NODE_TYPE::texture_node, true);

			imageIndex = (int)m_imagesList.size();
			m_imagesList.push_back(CImageData());
		}

		if (imageIndex >= 0) {
			CImageData& imageD = m_imagesList[imageIndex];
			imageD.pMasterImageHandle = masterImage->get_handle();
			imageD.fileName = masterImageName;

			texMappingData.textureParam.imageIndex = imageIndex;
			texMappingData.textureParam.uvLayerIndex = diffuseMappingLayer.get_uv_mapping();
			if (texMappingData.textureParam.uvLayerIndex < 0 || texMappingData.textureParam.uvLayerIndex > 1) {
				texMappingData.textureParam.uvLayerIndex = 0;
			}

			texMappingData.textureParam.repeatU    = diffuseMappingLayer.get_repetition_x();
			texMappingData.textureParam.repeatV    = diffuseMappingLayer.get_repetition_y();
			texMappingData.textureParam.wrapRepeat = diffuseMappingLayer.get_repeat_image();
			texMappingData.textureSource           = USD_DATA::TEXTURE_SOURE::texture_source_a;

			if (existImage) {
				if (imageD.textureSource != USD_DATA::TEXTURE_SOURE::texture_source_rgb) {
					imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_a;
				}
			} else {
				imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_a;
			}

			imageD.texTransform = texTransform;
		}
	} catch (...) { }
}

/**
 * 同一のマスターイメージがすでにimagesListに格納済みか.
 * @param[in]  masterImage     追加するMasterImage.
 * @param[in]  texTransform    マッピングの変換情報.
 * @param[in]  channelMix      mapping_layerのchanelMixの指定.
 */
int CMaterialTextureBake::m_findMasterImageInImagesList (sxsdk::master_image_class* masterImage, const CTextureTransform& texTransform, const int channelMix)
{
	if (!masterImage || m_imagesList.empty()) return -1;
	void* mHandle = masterImage->get_handle();

	int retI = -1;
	for (size_t i = 0; i < m_imagesList.size(); ++i) {
		const CImageData& imageD = m_imagesList[i];

		// 有効なRGBA要素をチェック.
		bool chkF = false;
		if (m_exportParam.texOptConvGrayscale) {		// グレイスケールとして出力する場合.
			switch (channelMix) {
				case sxsdk::enums::mapping_grayscale_red_mode:
					if (imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_r && imageD.texTransform.isSame(texTransform)) {
						chkF = true;
					}
					break;

				case sxsdk::enums::mapping_grayscale_green_mode:
					if (imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_g && imageD.texTransform.isSame(texTransform)) {
						chkF = true;
					}
					break;

				case sxsdk::enums::mapping_grayscale_blue_mode:
					if (imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_b && imageD.texTransform.isSame(texTransform)) {
						chkF = true;
					}
					break;

				case sxsdk::enums::mapping_grayscale_alpha_mode:
					if (imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_a && imageD.texTransform.isSame(texTransform)) {
						chkF = true;
					}
					break;

				default:
					if (imageD.texTransform.isSame(texTransform)) chkF = true;
			}
		} else {
			if (imageD.texTransform.isSame(texTransform)) chkF = true;
		}
		if (!chkF) continue;

		if (imageD.pMasterImageHandle == mHandle) {
			retI = (int)i;
			break;
		}
	}

	return retI;
}

/**
 * マッピングレイヤで、複数テクスチャをベイク.
 * @param[in]  surface           表面材質クラス.
 * @param[out] materialData  マテリアル情報が返る.
 */
bool CMaterialTextureBake::m_getMaterialMultiMappingFromSurface (sxsdk::surface_class* surface, CMaterialData& materialData)
{
	// 複数テクスチャの合成クラス.
	CImagesBlend imagesBlend(m_pScene, surface);
	imagesBlend.blendImages();

	CTextureTransform texTransform;
	std::string masterImageName = "";
	int imageIndex = -1;

	{
		const sxsdk::enums::mapping_type iType = sxsdk::enums::diffuse_mapping;
		if (imagesBlend.hasImage(iType)) {
			const sxsdk::rgb_class factor = (surface->get_diffuse_color()) * (surface->get_diffuse());
			sxsdk::master_image_class* masterImage = imagesBlend.getSingleMasterImage(iType);
			materialData.useDiffuseAlpha = imagesBlend.getDiffuseAlphaTrans();

			if (masterImage) {		// 単一テクスチャの場合.
				imageIndex = m_storeMasterImage(masterImage, materialData.diffuseTexture, masterImageName, materialData.useDiffuseAlpha);
			} else {				// 合成したテクスチャの場合.
				imageIndex = m_storeCustomImage(imagesBlend.getImage(iType), factor, materialData.diffuseTexture, masterImageName, materialData.useDiffuseAlpha);
				materialData.diffuseColor[0] = 1.0f;
				materialData.diffuseColor[1] = 1.0f;
				materialData.diffuseColor[2] = 1.0f;
			}

			const sx::vec<int,2> repeatV = imagesBlend.getImageRepeat(iType);
			materialData.diffuseTexture.textureParam.repeatU = repeatV.x;
			materialData.diffuseTexture.textureParam.repeatV = repeatV.y;

			if (materialData.useDiffuseAlpha) {
				materialData.opacityTexture.textureParam.imageIndex = imageIndex;
				materialData.opacityTexture.textureParam.repeatU = repeatV.x;
				materialData.opacityTexture.textureParam.repeatV = repeatV.y;
				materialData.opacityTexture.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_a;
			}
		}
	}

	{
		const sxsdk::enums::mapping_type iType = sxsdk::enums::reflection_mapping;
		if (imagesBlend.hasImage(iType)) {
			const sxsdk::rgb_class factor = (sxsdk::rgb_class(1, 1, 1)) * (surface->get_reflection());
			sxsdk::master_image_class* masterImage = imagesBlend.getSingleMasterImage(iType);
			if (masterImage) {		// 単一テクスチャの場合.
				imageIndex = m_storeMasterImage(masterImage, materialData.metallicTexture, masterImageName);
			} else {				// 合成したテクスチャの場合.
				imageIndex = m_storeCustomImage(imagesBlend.getImage(iType), factor, materialData.metallicTexture, masterImageName);
				materialData.metallic = 1.0f;
			}
			const sx::vec<int,2> repeatV = imagesBlend.getImageRepeat(iType);
			materialData.metallicTexture.textureParam.repeatU = repeatV.x;
			materialData.metallicTexture.textureParam.repeatV = repeatV.y;
		}
		materialData.metallic = surface->get_reflection();
	}

	{
		const sxsdk::enums::mapping_type iType = sxsdk::enums::glow_mapping;
		if (imagesBlend.hasImage(iType)) {
			const sxsdk::rgb_class factor = (surface->get_glow_color()) * (surface->get_glow());
			sxsdk::master_image_class* masterImage = imagesBlend.getSingleMasterImage(iType);
			if (masterImage) {		// 単一テクスチャの場合.
				imageIndex = m_storeMasterImage(masterImage, materialData.emissiveTexture, masterImageName);
			} else {				// 合成したテクスチャの場合.
				imageIndex = m_storeCustomImage(imagesBlend.getImage(iType), factor, materialData.emissiveTexture, masterImageName);

				materialData.emissiveColor[0] = 1.0f;
				materialData.emissiveColor[1] = 1.0f;
				materialData.emissiveColor[2] = 1.0f;
			}

			const sx::vec<int,2> repeatV = imagesBlend.getImageRepeat(iType);
			materialData.emissiveTexture.textureParam.repeatU = repeatV.x;
			materialData.emissiveTexture.textureParam.repeatV = repeatV.y;
		}
	}

	{
		const sxsdk::enums::mapping_type iType = sxsdk::enums::roughness_mapping;
		if (imagesBlend.hasImage(iType)) {
			const sxsdk::rgb_class factor = (sxsdk::rgb_class(1, 1, 1)) * (surface->get_roughness());
			sxsdk::master_image_class* masterImage = imagesBlend.getSingleMasterImage(iType);
			if (masterImage) {		// 単一テクスチャの場合.
				imageIndex = m_storeMasterImage(masterImage, materialData.roughnessTexture, masterImageName);
			} else {				// 合成したテクスチャの場合.
				imageIndex = m_storeCustomImage(imagesBlend.getImage(iType), factor, materialData.roughnessTexture, masterImageName);
				materialData.roughness = 1.0f;
			}

			const sx::vec<int,2> repeatV = imagesBlend.getImageRepeat(iType);
			materialData.roughnessTexture.textureParam.repeatU = repeatV.x;
			materialData.roughnessTexture.textureParam.repeatV = repeatV.y;
		}
		materialData.roughness = surface->get_roughness();
	}

	{
		const sxsdk::enums::mapping_type iType = MAPPING_TYPE_OPACITY;
		if (imagesBlend.hasImage(iType)) {
			//const sxsdk::rgb_class factor = (sxsdk::rgb_class(1, 1, 1)) * (surface->get_roughness());
			sxsdk::master_image_class* masterImage = imagesBlend.getSingleMasterImage(iType);
			if (!masterImage) {			// 合成したテクスチャの場合.
				const sxsdk::rgb_class factor(1.0f, 1.0f, 1.0f);
				imageIndex = m_storeCustomImage(imagesBlend.getImage(iType), factor, materialData.opacityTexture, masterImageName);
				materialData.opacity = 1.0f;
			}

			const sx::vec<int,2> repeatV = imagesBlend.getImageRepeat(iType);
			materialData.opacityTexture.textureParam.repeatU = repeatV.x;
			materialData.opacityTexture.textureParam.repeatV = repeatV.y;
		}
	}

	{
		const sxsdk::enums::mapping_type iType = sxsdk::enums::normal_mapping;
		if (imagesBlend.hasImage(iType)) {
			const sxsdk::rgb_class factor = (sxsdk::rgb_class(1, 1, 1));
			sxsdk::master_image_class* masterImage = imagesBlend.getSingleMasterImage(iType);
			if (masterImage) {		// 単一テクスチャの場合.
				imageIndex = m_storeMasterImage(masterImage, materialData.normalTexture, masterImageName);
			} else {				// 合成したテクスチャの場合.
				imageIndex = m_storeCustomImage(imagesBlend.getImage(iType), factor, materialData.normalTexture, masterImageName);
			}
			const sx::vec<int,2> repeatV = imagesBlend.getImageRepeat(iType);
			materialData.normalTexture.textureParam.repeatU = repeatV.x;
			materialData.normalTexture.textureParam.repeatV = repeatV.y;
		}
	}
/*
	try {
		// マッピングレイヤ情報を取得.
		const int mappingLayersCou = surface->get_number_of_mapping_layers();
		if (!(surface->get_has_mapping_layers()) || mappingLayersCou == 0) return true;

		// テクスチャを取得.
		for (int mLoop = 0; mLoop < mappingLayersCou; ++mLoop) {
			sxsdk::mapping_layer_class& mappingLayer = surface->mapping_layer(mLoop);
			const int mType = mappingLayer.get_type();
			CTextureTransform texTransform;
			texTransform.flipColor     = mappingLayer.get_flip_color();
			texTransform.textureWeight = mappingLayer.get_weight();
		}

	} catch (...) { }
*/
	return true;
}

/**
 * 指定のマスターイメージをエクスポート用に格納.
 * @param[in]  masterImage      マスターイメージクラス.
 * @param[out] texMappingData   マッピング情報の格納先.
 * @param[out] masterImageName  USDでのマスターイメージ名が返る.
 * @param[in]  diffuseAlpha     DiffuseのALphaを使用する場合.
 * @return イメージ番号.
 */
int CMaterialTextureBake::m_storeMasterImage (sxsdk::master_image_class* masterImage, CTextureMappingData& texMappingData, std::string& masterImageName, const bool diffuseAlpha)
{
	CTextureTransform texTransform;
	int channelMix = 0;

	int imageIndex = m_findMasterImageInImagesList(masterImage, texTransform, channelMix);
	masterImageName = "";

	if (imageIndex >= 0) {
		const CImageData& imageD = m_imagesList[imageIndex];
		masterImageName = imageD.fileName;
	} else {
		// イメージ名をASCIIのファイル名にする.
		masterImageName = masterImage->get_name();
		if (!StringUtil::checkASCII(masterImageName)) {
			masterImageName = "texture";
		}

		if (diffuseAlpha) {
			// 「アルファ透明」使用時でデフォルトのパラメータの時は、強制的にpng出力.
			masterImageName = StringUtil::SetFileImageExtension(masterImageName, "png", true);
		} else {
			// エクスポートオプションm_exportParam.optTextureTypeにより、pngにするかjpgにするか決める.
			if (m_exportParam.optTextureType == USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_use_image_name) {
				// 拡張子がある場合はそれを採用し、ない場合はpngにする.
				masterImageName = StringUtil::SetFileImageExtension(masterImageName, "png");
			} else {
				const bool usePng = (m_exportParam.optTextureType == USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_replace_png);
				masterImageName = StringUtil::SetFileImageExtension(masterImageName, usePng ? "png" : "jpg", true);
			}
		}

		// ユニークファイル名として追加.
		// 同一名がある場合は、連番付きで返す.
		masterImageName = m_findImageFileNames.appendName(masterImageName, USD_DATA::NODE_TYPE::texture_node, true);

		imageIndex = (int)m_imagesList.size();
		m_imagesList.push_back(CImageData());
	}

	if (imageIndex >= 0) {
		CImageData& imageD = m_imagesList[imageIndex];
		imageD.pMasterImageHandle = masterImage->get_handle();
		imageD.fileName = masterImageName;
		imageD.occlusionF = false;

		texMappingData.textureParam.imageIndex = imageIndex;

		imageD.texTransform = texTransform;
	}

	return imageIndex;
}

/**
 * 指定のカスタムイメージをエクスポート用に格納.
 * @param[in]  image            マスターイメージクラス.
 * @param[in]  factor           乗算値.
 * @param[out] texMappingData   マッピング情報の格納先.
 * @param[out] masterImageName  USDでのマスターイメージ名が返る.
 * @param[in]  diffuseAlpha     DiffuseのALphaを使用する場合.
 * @return イメージ番号.
 */
int CMaterialTextureBake::m_storeCustomImage (sxsdk::image_interface* image, const sxsdk::rgb_class factor, CTextureMappingData& texMappingData, std::string& masterImageName, const bool diffuseAlpha)
{
	int imageIndex = -1;
	if (image == NULL) return -1;

	// ユニークなテクスチャファイル名を取得.
	std::string imageName = "bake_texture";

	if (diffuseAlpha) {
		// 「アルファ透明」使用時でデフォルトのパラメータの時は、強制的にpng出力.
		imageName = StringUtil::SetFileImageExtension(imageName, "png", true);
	} else {
		if (m_exportParam.optTextureType == USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_use_image_name) {
			// 拡張子がある場合はそれを採用し、ない場合はpngにする.
			imageName = StringUtil::SetFileImageExtension(imageName, "png");
		} else {
			const bool usePng = (m_exportParam.optTextureType == USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_replace_png);
			imageName = StringUtil::SetFileImageExtension(imageName, usePng ? "png" : "jpg", true);
		}
	}

	// ユニークファイル名として追加.
	// 同一名がある場合は、連番付きで返す.
	imageName = m_findImageFileNames.appendName(imageName, USD_DATA::NODE_TYPE::texture_node, true);

	imageIndex = (int)m_imagesList.size();
	m_imagesList.push_back(CImageData());

	CImageData& imageD = m_imagesList[imageIndex];
	imageD.fileName   = imageName;

	// テクスチャのRGBAを保持.
	try {
		const int width  = image->get_size().x;
		const int height = image->get_size().y;
		imageD.imageWidth  = width;
		imageD.imageHeight = height;

		imageD.rgbaBuff.resize(width * height * 4);

		std::vector<sx::rgba8_class> lineBuff;
		lineBuff.resize(width);

		int iPos = 0;
		for (int y = 0; y < height; ++y) {
			image->get_pixels_rgba(0, y, width, 1, &(lineBuff[0]));
			for (int x = 0; x < width; ++x) {
				imageD.rgbaBuff[iPos + 0] = (unsigned char)((float)lineBuff[x].red * factor.red);
				imageD.rgbaBuff[iPos + 1] = (unsigned char)((float)lineBuff[x].green * factor.green);
				imageD.rgbaBuff[iPos + 2] = (unsigned char)((float)lineBuff[x].blue * factor.blue);
				imageD.rgbaBuff[iPos + 3] = lineBuff[x].alpha;
				iPos += 4;
			}
		}
	} catch (...) { }

	texMappingData.textureParam.imageIndex = imageIndex;

	return imageIndex;
}

