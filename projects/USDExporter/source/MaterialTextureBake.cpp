/**
 * Shade3Dの表面材質より、テクスチャをPBR向けにベイク.
 * このクラス内に他とかぶらないようにしたテクスチャイメージのリストを持つ.
 */
#include "MaterialTextureBake.h"
#include "Shade3DUtil.h"
#include "StringUtil.h"
#include "StreamCtrl.h"
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

CMaterialTextureBake::CMaterialTextureBake (sxsdk::scene_interface* scene, const CExportParam& exportParam)
{
	m_pScene = scene;
	m_exportParam = exportParam;
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
		//if (!m_exportParam.texOptBakeMultiTextures) {
			// 単純なベイクを行う.
			return m_getSimpleMaterialMappingFromSurface(surface, materialData);
		//} else {
			// 複数マッピングを合成.
		//	return m_getMaterialMultiMappingFromSurface(surface, materialData);
		//}

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
				m_setTextureMappingData(mappingLayer, texTransform, materialData.occlusionTexture, true);
			}

			if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;

			if (mType == sxsdk::enums::diffuse_mapping) {
				// アルファ透明を使用.
				if (mappingLayer.get_channel_mix() == sxsdk::enums::mapping_transparent_alpha_mode) {
					materialData.useDiffuseAlpha = true;
				}
				texTransform.multiR = materialData.diffuseColor[0];
				texTransform.multiG = materialData.diffuseColor[1];
				texTransform.multiB = materialData.diffuseColor[2];
				m_setTextureMappingData(mappingLayer, texTransform, materialData.diffuseTexture, materialData.useDiffuseAlpha);
			}

			if (mType == sxsdk::enums::glow_mapping) {
				texTransform.multiR = materialData.emissiveColor[0];
				texTransform.multiG = materialData.emissiveColor[1];
				texTransform.multiB = materialData.emissiveColor[2];
				m_setTextureMappingData(mappingLayer, texTransform, materialData.emissiveTexture);
			}
			if (mType == sxsdk::enums::normal_mapping) {
				const float weight = mappingLayer.get_weight();
				texTransform.textureNormal = true;
				m_setTextureMappingData(mappingLayer, texTransform, materialData.normalTexture);
			}
			if (mType == sxsdk::enums::roughness_mapping) {
				texTransform.multiR = texTransform.multiG = texTransform.multiB = materialData.roughness;
				m_setTextureMappingData(mappingLayer, texTransform, materialData.roughnessTexture);
			}
			if (mType == sxsdk::enums::reflection_mapping) {
				texTransform.multiR = texTransform.multiG = texTransform.multiB = materialData.metallic;
				m_setTextureMappingData(mappingLayer, texTransform, materialData.metallicTexture);
			}
			if (mType == sxsdk::enums::transparency_mapping) {
				texTransform.multiR = texTransform.multiG = texTransform.multiB = 1.0f;
				texTransform.flipColor = !texTransform.flipColor;		// 透明度とOpacityは逆になる.
				texTransform.factor[0] = texTransform.factor[1] = texTransform.factor[2] = texTransform.factor[3] = materialData.opacity;
				m_setTextureMappingData(mappingLayer, texTransform, materialData.opacityTexture);
			}
		}

		return true;

	} catch (...) { }

	return false;
}

/**
 * テクスチャマッピング情報を追加.
 * @param[in]  mappingLayer    マッピングレイヤ情報.
 * @param[in]  texTransform    マッピングの変換情報.
 * @param[out] texMappingData  マッピング情報の格納先.
 */
void CMaterialTextureBake::m_setTextureMappingData (sxsdk::mapping_layer_class& mappingLayer, const CTextureTransform& texTransform, CTextureMappingData& texMappingData, const bool occlusionF)
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
			if (useAlpha) {
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

	return true;
}

