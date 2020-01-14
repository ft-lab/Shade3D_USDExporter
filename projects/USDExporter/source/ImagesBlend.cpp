/**
 * Export用にShade3Dの表面材質のマッピングレイヤのテクスチャを合成.
 */
#include "ImagesBlend.h"
#include "MathUtil.h"
#include "Shade3DUtil.h"
#include "StreamCtrl.h"

CImagesBlend::CImagesBlend (sxsdk::scene_interface* scene, sxsdk::surface_class* surface) : m_pScene(scene), m_surface(surface)
{
}

/**
 * 個々のイメージを合成.
 */
void CImagesBlend::blendImages ()
{
	m_diffuseRepeat    = sx::vec<int,2>(1, 1);
	m_normalRepeat     = sx::vec<int,2>(1, 1);
	m_reflectionRepeat = sx::vec<int,2>(1, 1);
	m_roughnessRepeat  = sx::vec<int,2>(1, 1);
	m_glowRepeat       = sx::vec<int,2>(1, 1);
	m_diffuseTexCoord     = 0;
	m_normalTexCoord      = 0;
	m_reflectionTexCoord  = 0;
	m_roughnessTexCoord   = 0;
	m_glowTexCoord        = 0;
	m_occlusionTexCoord   = 0;

	m_diffuseAlphaTrans = false;
	m_normalWeight    = 1.0f;
	m_occlusionWeight = 1.0f;

	m_hasDiffuseImage    = false;
	m_hasReflectionImage = false;
	m_hasRoughnessImage  = false;
	m_hasNormalImage     = false;
	m_hasGlowImage       = false;
	m_hasOcclusionImage  = false;

	// Shade3Dでの表面材質のマッピングレイヤで、加工無しの画像が参照されているかチェック.
	m_checkSingleImage(sxsdk::enums::diffuse_mapping, &m_diffuseMasterImage, m_diffuseTexCoord, m_diffuseRepeat, m_hasDiffuseImage);
	m_checkSingleImage(sxsdk::enums::normal_mapping, &m_normalMasterImage, m_normalTexCoord, m_normalRepeat, m_hasNormalImage);
	m_checkSingleImage(sxsdk::enums::reflection_mapping, &m_reflectionMasterImage, m_reflectionTexCoord, m_reflectionRepeat, m_hasReflectionImage);
	m_checkSingleImage(sxsdk::enums::roughness_mapping, &m_roughnessMasterImage, m_roughnessTexCoord, m_roughnessRepeat, m_hasRoughnessImage);
	m_checkSingleImage(sxsdk::enums::glow_mapping, &m_glowMasterImage, m_glowTexCoord, m_glowRepeat, m_hasGlowImage);

	// マッピングレイヤのOcclusion情報を取得.
	m_occlusionWeight = 1.0f;
	m_checkOcclusionSingleImage(&m_occlusionMasterImage, m_occlusionTexCoord, m_occlusionRepeat, m_hasOcclusionImage);

	// Diffuseのアルファ透明を使用しているかチェック.
	m_diffuseAlphaTrans = m_checkDiffuseAlphaTrans();

	// 法線マップの強さを取得.
	m_normalWeight = m_getNormalWeight();

	// Shade3Dでの表面材質のマッピングレイヤごとに、各種イメージを合成.
	if (!m_diffuseMasterImage && m_hasDiffuseImage) m_blendImages(sxsdk::enums::diffuse_mapping, m_diffuseRepeat);
	if (!m_normalMasterImage && m_hasNormalImage) m_blendImages(sxsdk::enums::normal_mapping, m_normalRepeat);
	if (!m_reflectionMasterImage && m_hasReflectionImage) m_blendImages(sxsdk::enums::reflection_mapping, m_reflectionRepeat);
	if (!m_roughnessMasterImage && m_hasRoughnessImage) m_blendImages(sxsdk::enums::roughness_mapping, m_roughnessRepeat);
	if (!m_glowMasterImage && m_hasGlowImage) m_blendImages(sxsdk::enums::glow_mapping, m_glowRepeat);

	// Shade3DのマテリアルからPBRマテリアルに置き換え.
	m_convShade3DToPBRMaterial();
}

/**
 * 指定のテクスチャの種類がベイク不要の1枚のテクスチャであるかチェック.
 * @param[in]  mappingType   マッピングの種類.
 * @param[out] ppMasterImage master imageの参照を返す.
 * @param[out] uvTexCoord    UV用の使用テクスチャ層番号を返す.
 * @param[out] texRepeat     繰り返し回数。複数テクスチャ使用で異なる数の場合は(1, 1)が入る.
 * @param[out] hasImage      イメージを持つか (単数または複数).
 */
bool CImagesBlend::m_checkSingleImage (const sxsdk::enums::mapping_type mappingType, sxsdk::master_image_class** ppMasterImage,
		int& uvTexCoord, sx::vec<int,2>& texRepeat, bool& hasImage)
{
	const int layersCou = m_surface->get_number_of_mapping_layers();
	bool singleImage = false;
	int counter = 0;
	sxsdk::master_image_class* pRetMasterImage = NULL;
	*ppMasterImage = NULL;
	texRepeat = sx::vec<int,2>(1, 1);
	hasImage = false;

	float baseV = 1.0f;
	if (mappingType == sxsdk::enums::diffuse_mapping) baseV = m_surface->get_diffuse();
	if (mappingType == sxsdk::enums::reflection_mapping) baseV = m_surface->get_reflection();
	if (mappingType == sxsdk::enums::roughness_mapping) baseV = m_surface->get_roughness();
	if (mappingType == sxsdk::enums::glow_mapping) baseV = m_surface->get_glow();

	bool texRepeatSingle = true;

	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
		const int type = mappingLayer.get_type();
		if (type != mappingType) continue;
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

		const float weight = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;

		compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
		if (!image || !(image->has_image()) || (image->get_size().x) <= 0 || (image->get_size().y) <= 0) continue;

		const bool flipColor = mappingLayer.get_flip_color();			// 色反転.
		const bool flipH     = mappingLayer.get_horizontal_flip();		// 左右反転.
		const bool flipV     = mappingLayer.get_vertical_flip();		// 上下反転.
		const bool rotate90  = mappingLayer.get_swap_axes();			// 90度回転.

		const int channelMix = mappingLayer.get_channel_mix();			// イメージのチャンネル.
		const bool useChannelMix = (channelMix == sxsdk::enums::mapping_grayscale_alpha_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_red_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_green_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_blue_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_average_mode);

		// マスターイメージを持つか調べる.
		pRetMasterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);
		if (pRetMasterImage) {
			if (counter == 0) {
				singleImage     = true;
				texRepeatSingle = true;
				uvTexCoord  = mappingLayer.get_uv_mapping();
				const int repeatX = mappingLayer.get_repetition_x();
				const int repeatY = mappingLayer.get_repetition_y();
				texRepeat = sx::vec<int,2>(repeatX, repeatY);
			}
		}

		// 1つ前が「マット」の場合は無条件でベイク対象にする.
		if (i > 0) {
			sxsdk::mapping_layer_class& prevMappingLayer = m_surface->mapping_layer(i - 1);
			if (prevMappingLayer.get_pattern() == sxsdk::enums::image_pattern) {
				if (prevMappingLayer.get_type() == sxsdk::enums::weight_mapping) {
					{
						const int repeatX = prevMappingLayer.get_repetition_x();
						const int repeatY = prevMappingLayer.get_repetition_y();
						if (texRepeat[0] != repeatX || texRepeat[1] != repeatY) texRepeatSingle = false;
					}
					counter++;
				}
			}
		}

		if (flipColor || flipH || flipV || rotate90 || useChannelMix || !MathUtil::isZero(weight - 1.0f) || !MathUtil::isZero(baseV - 1.0f)) {
			singleImage = false;
		}

		{
			const int repeatX = mappingLayer.get_repetition_x();
			const int repeatY = mappingLayer.get_repetition_y();
			if (texRepeat[0] != repeatX || texRepeat[1] != repeatY) texRepeatSingle = false;
		}

		counter++;
	}

	if (!texRepeatSingle) texRepeat = sx::vec<int,2>(1, 1);
	if (counter >= 1) hasImage = true;
	if (singleImage && counter == 1) {
		*ppMasterImage = pRetMasterImage;
		return true;
	}


	return false;
}

/**
 * 法線マップの強さを取得.
 */
float CImagesBlend::m_getNormalWeight ()
{
	float retWeight = 1.0f;

	const int layersCou = m_surface->get_number_of_mapping_layers();
	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
		if (mappingLayer.get_type() != sxsdk::enums::normal_mapping) continue;
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

		const float weight = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;
		retWeight = weight;
		break;
	}
	return retWeight;
}

/**
 * Occlusionのテクスチャの種類がベイク不要の1枚のテクスチャであるかチェック.
 * ※ OcclusionレイヤはShade3D ver.17/18段階では存在しないため COcclusionTextureShaderInterface クラスで与えている.
 * @param[out] ppMasterImage master imageの参照を返す.
 * @param[out] uvTexCoord    UV用の使用テクスチャ層番号を返す.
 * @param[out] texRepeat     繰り返し回数.
 * @param[out] hasImage      イメージを持つか (単数または複数).
 */
bool CImagesBlend::m_checkOcclusionSingleImage (sxsdk::master_image_class** ppMasterImage,
	int& uvTexCoord,
	sx::vec<int,2>& texRepeat,
	bool& hasImage)
{
	const int layersCou = m_surface->get_number_of_mapping_layers();
	bool singleImage = false;
	int counter = 0;
	sxsdk::master_image_class* pRetMasterImage = NULL;
	*ppMasterImage = NULL;
	texRepeat = sx::vec<int,2>(1, 1);
	hasImage = false;
	uvTexCoord = 0;

	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);

		// Occlusionレイヤで拡散反射かどうか.
		if (!Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) continue;
		if (mappingLayer.get_type() != sxsdk::enums::diffuse_mapping) continue;

		const float weight  = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;

		compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
		if (!image || !(image->has_image()) || (image->get_size().x) <= 0 || (image->get_size().y) <= 0) continue;

		const bool flipColor = mappingLayer.get_flip_color();			// 色反転.
		const bool flipH     = mappingLayer.get_horizontal_flip();		// 左右反転.
		const bool flipV     = mappingLayer.get_vertical_flip();		// 上下反転.
		const int channelMix = mappingLayer.get_channel_mix();			// イメージのチャンネル.
		const bool useChannelMix = (channelMix == sxsdk::enums::mapping_grayscale_alpha_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_red_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_green_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_blue_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_average_mode);

		if (flipColor || flipH || flipV || useChannelMix) {
			counter++;
			continue;
		}

		// マスターイメージを持つか調べる.
		pRetMasterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);
		if (pRetMasterImage) {
			if (counter == 0) {
				singleImage = true;
				m_occlusionWeight = weight;

				// UV層番号は、カスタムのshader_interfaceではmapping_layer_classから取得できないため、.
				// streamに保持しているのを取得.
				//uvTexCoord  = mappingLayer.get_uv_mapping();
				try {
					compointer<sxsdk::stream_interface> stream(mappingLayer.get_attribute_stream_interface_with_uuid(OCCLUSION_SHADER_INTERFACE_ID));
					COcclusionShaderData occlusionD;
					StreamCtrl::loadOcclusionParam(stream, occlusionD);
					uvTexCoord = occlusionD.uvIndex;
				} catch (...) { }

				const int repeatX = mappingLayer.get_repetition_x();
				const int repeatY = mappingLayer.get_repetition_y();
				texRepeat = sx::vec<int,2>(repeatX, repeatY);
			}
		}

		counter++;
		if (counter >= 2) break;
	}
	if (counter >= 1) hasImage = true;
	if (singleImage && counter == 1) {
		*ppMasterImage = pRetMasterImage;
		return true;
	}

	return false;
}

/**
 * Diffuseのアルファ透明を使用しているかチェック.
 */
bool CImagesBlend::m_checkDiffuseAlphaTrans ()
{
	bool alphaTrans = false;
	const int layersCou = m_surface->get_number_of_mapping_layers();
	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
		if (mappingLayer.get_type() != sxsdk::enums::diffuse_mapping) continue;
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

		if (mappingLayer.get_channel_mix() == sxsdk::enums::mapping_transparent_alpha_mode) {
			alphaTrans = true;
			break;
		}
	}
	return alphaTrans;
}

/**
 * 指定のマッピングの種類でのテクスチャサイズの最大を取得.
 * 異なるサイズのテクスチャが混在する場合、一番大きいサイズのテクスチャに合わせる.
 * @param[in]  mappingType   マッピングの種類.
 */
sx::vec<int,2> CImagesBlend::m_getMaxMappingImageSize (const sxsdk::enums::mapping_type mappingType)
{
	sx::vec<int,2> texSize(0, 0);

	const int layersCou = m_surface->get_number_of_mapping_layers();
	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
		if (mappingLayer.get_type() != mappingType) continue;
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

		const float weight = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;

		compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
		if (!image || !(image->has_image()) || (image->get_size().x) <= 0 || (image->get_size().y) <= 0) continue;
		sx::vec<int,2> size = image->get_size();

		// 1つ前が「マット」の場合.
		if (i > 0) {
			sxsdk::mapping_layer_class& prevMappingLayer = m_surface->mapping_layer(i - 1);
			if (prevMappingLayer.get_pattern() == sxsdk::enums::image_pattern) {
				if (prevMappingLayer.get_type() == sxsdk::enums::weight_mapping) {
					compointer<sxsdk::image_interface> image2(prevMappingLayer.get_image_interface());
					if (image2 && (image2->has_image()) && (image2->get_size().x) > 0 && (image2->get_size().y) > 0) {
						const sx::vec<int,2> size2 = image2->get_size();
						size.x = std::max(size.x, size2.x);
						size.y = std::max(size.y, size2.y);
					}
				}
			}
		}

		texSize.x = std::max(texSize.x, size.x);
		texSize.y = std::max(texSize.y, size.y);
	}
	return texSize;
}

/**
 * マッピングレイヤで「マット」を持つか.
 * @param[in]  mappingType   マッピングの種類.
 */
 bool CImagesBlend::m_hasWeightTexture (const sxsdk::enums::mapping_type mappingType)
 {
	bool hasF = false;

	const int layersCou = m_surface->get_number_of_mapping_layers();
	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
		if (mappingLayer.get_type() != mappingType) continue;
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

		const float weight = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;

		compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
		if (!image || !(image->has_image()) || (image->get_size().x) <= 0 || (image->get_size().y) <= 0) continue;

		if (i > 0) {
			sxsdk::mapping_layer_class& prevMappingLayer = m_surface->mapping_layer(i - 1);
			if (prevMappingLayer.get_pattern() == sxsdk::enums::image_pattern) {
				if (prevMappingLayer.get_type() == sxsdk::enums::weight_mapping) {
					compointer<sxsdk::image_interface> image2(prevMappingLayer.get_image_interface());
					if (image2 && (image2->has_image()) && (image2->get_size().x) > 0 && (image2->get_size().y) > 0) {
						hasF = true;
						break;
					}
				}
			}
		}
	}
	return hasF;
 }

 /**
  * テクスチャを複製する場合に、反転や繰り返し回数を考慮.
  * @param[in] image       イメージクラス.
  * @param[in] dstSize     複製後のサイズ.
  * @param[in] flipColor   色反転.
  * @param[in] flipH       左右反転.
  * @param[in] flipV       上下反転.
  * @param[in] rotate90    90度反転.
  * @param[in] repeatU     繰り返し回数U.
  * @param[in] repeatV     繰り返し回数V.
  */
compointer<sxsdk::image_interface> CImagesBlend::m_duplicateImage (sxsdk::image_interface* image, const sx::vec<int,2>& dstSize, const bool flipColor, const bool flipH, const bool flipV, const bool rotate90, const int repeatU, const int repeatV)
{
	compointer<sxsdk::image_interface> image2, dstImage;
	const int width  = dstSize.x;
	const int height = dstSize.y;
	if (width == 0 || height == 0) return dstImage;

	std::vector<sx::rgba8_class> lineCols, lineCols2;
	lineCols.resize(width);
	lineCols2.resize(width);

	if (repeatU > 1 || repeatV > 1) {
		dstImage = m_pScene->create_image_interface(dstSize);

		compointer<sxsdk::graphic_context_interface> gc(dstImage->get_graphic_context_interface());
		const float dx = (float)width / (float)repeatU;
		const float dy = (float)height / (float)repeatV;

		sx::rectangle_class rect;
		for (float y = 0.0f; y < (float)(height - 1); y += dy) {
			for (float x = 0.0f; x < (float)(width - 1); x += dx) {
				rect.min = sx::vec<int,2>((int)x, (int)y);
				rect.max = sx::vec<int,2>((int)(x + dx), (int)(y + dy));
				gc->draw_image(image, rect);
			}
		}
	} else {
		dstImage = image->duplicate_image(&dstSize);
	}

	if (flipColor) {
		for (int y = 0; y < height; ++y) {
			dstImage->get_pixels_rgba(0, y, width, 1, &(lineCols[0]));
			for (int x = 0; x < width; ++x) {
				lineCols[x].red   = 1.0f - lineCols[x].red;
				lineCols[x].green = 1.0f - lineCols[x].green;
				lineCols[x].blue  = 1.0f - lineCols[x].blue;
			}
			dstImage->set_pixels_rgba(0, y, width, 1, &(lineCols[0]));
		}
	}
	if (flipH) {
		for (int y = 0; y < height; ++y) {
			dstImage->get_pixels_rgba(0, y, width, 1, &(lineCols2[0]));
			for (int x = 0; x < width; ++x) {
				lineCols[x] = lineCols2[width - x - 1];
			}
			dstImage->set_pixels_rgba(0, y, width, 1, &(lineCols[0]));
		}
	}

	if (flipV) {
		for (int y = 0; y < height / 2; ++y) {
			dstImage->get_pixels_rgba(0, y, width, 1, &(lineCols[0]));
			dstImage->get_pixels_rgba(0, height - y - 1, width, 1, &(lineCols2[0]));
			dstImage->set_pixels_rgba(0, y, width, 1, &(lineCols2[0]));
			dstImage->set_pixels_rgba(0, height - y - 1, width, 1, &(lineCols[0]));
		}
	}

	if (rotate90) {
		try {
			compointer<sxsdk::image_interface> image2(dstImage->duplicate_image(&sx::vec<int,2>(height, width)));
			for (int y = 0; y < height; ++y) {
				image2->get_pixels_rgba(y, 0, 1, width, &(lineCols2[0]));
				for (int x = 0; x < width; ++x) {
					lineCols[x] = lineCols2[width - x - 1];

				}
				dstImage->set_pixels_rgba(0, y, width, 1, &(lineCols[0]));
			}
		} catch (...) { }
	}

	return dstImage;
 }


/**
 * 指定のテクスチャの合成処理.
 * @param[in] mappingType  マッピングの種類.
 * @param[in] repeatTex    繰り返し回数.
 */
bool CImagesBlend::m_blendImages (const sxsdk::enums::mapping_type mappingType, const sx::vec<int,2>& repeatTex)
{
	const int layersCou = m_surface->get_number_of_mapping_layers();
	compointer<sxsdk::image_interface> newImage;
	int newWidth, newHeight;
	int newRepeatX, newRepeatY;
	int newTexCoord;
	sxsdk::master_image_class* pNewMasterImage = NULL;
	std::string newTexName;
	int counter = 0;
	std::vector<sxsdk::rgba_class> rgbaLine0, rgbaLine, rgbaWeightLine;
	sxsdk::rgba_class col, whiteCol;
	whiteCol = sxsdk::rgba_class(1, 1, 1, 1);
	bool singleSimpleMapping = true;				// 1枚のテクスチャのみの参照で、色反転や左右反転/上下反転などがない場合は.
													// そのときのマスターイメージを採用 (ベイクする必要なしの場合、true).

	// Diffuse時にアルファ透明を使用する場合はそのレイヤ番号を取得.
	int diffuseAlphaLayerIndex = -1;
	if (mappingType == sxsdk::enums::diffuse_mapping) {
		for (int i = 0; i < layersCou; ++i) {
			sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
			if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
			if (mappingLayer.get_type() != sxsdk::enums::diffuse_mapping) continue;
			if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

			if (mappingLayer.get_channel_mix() == sxsdk::enums::mapping_transparent_alpha_mode) {
				diffuseAlphaLayerIndex = i;
				m_diffuseAlphaTrans = true;
				break;
			}
		}
	}
	std::vector<unsigned char> alphaBuff;

	// 法線の中立値.
	sxsdk::rgba_class normalDefault(0.5f, 0.5f, 1.0f, 1.0f);
	if (mappingType == sxsdk::enums::normal_mapping) whiteCol = normalDefault;

	// 合成するテクスチャサイズ.
	const sx::vec<int,2> dstTexSize = m_getMaxMappingImageSize(mappingType);
	if (dstTexSize.x == 0 || dstTexSize.y == 0) return false;

	// 「マット」を持つか.
	const bool hasWeightTex = m_hasWeightTexture(mappingType);

	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
		if (mappingLayer.get_type() != mappingType) continue;
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

		const float weight  = std::min(std::max(mappingLayer.get_weight(), 0.0f), 1.0f);
		const float weight2 = 1.0f - weight;
		if (MathUtil::isZero(weight)) continue;

		float repeatU = (float)mappingLayer.get_repetition_X();
		float repeatV = (float)mappingLayer.get_repetition_Y();
		if (repeatTex[0] != 1 || repeatTex[1] != 1) {
			repeatU = 1;
			repeatV = 1;
		}

		// 1つ前が「マット」の場合.
		compointer<sxsdk::image_interface> weightImage;
		int weightWidth  = 0;
		int weightHeight = 0;
		bool weightFlipColor = false;
		bool weightFlipH     = false;
		bool weightFlipV     = false;
		bool weightRotate90  = false;
		int weightRepeatU = 1;
		int weightRepeatV = 1;
		if (i > 0) {
			sxsdk::mapping_layer_class& prevMappingLayer = m_surface->mapping_layer(i - 1);
			if (prevMappingLayer.get_pattern() == sxsdk::enums::image_pattern) {
				if (prevMappingLayer.get_type() == sxsdk::enums::weight_mapping) {
					weightFlipColor = prevMappingLayer.get_flip_color();
					weightFlipH     = prevMappingLayer.get_horizontal_flip();
					weightFlipV     = prevMappingLayer.get_vertical_flip();
					weightRotate90  = prevMappingLayer.get_swap_axes();
					weightRepeatU   = prevMappingLayer.get_repetition_X();
					weightRepeatV   = prevMappingLayer.get_repetition_Y();
					if (repeatTex[0] != 1 || repeatTex[1] != 1) {
						weightRepeatU = 1;
						weightRepeatV = 1;
					}

					try {
						weightImage = prevMappingLayer.get_image_interface();
						if (weightImage && (weightImage->has_image())) {
							weightWidth  = weightImage->get_size().x;
							weightHeight = weightImage->get_size().y;
							if (weightWidth <= 1 || weightHeight <= 1) {
								weightWidth = weightHeight = 0;
								weightImage->Release();
							}
						}
					} catch (...) {
						weightWidth = weightHeight = 0;
					}
				}
			}
		}

		const int blendMode = mappingLayer.get_blend_mode();
		try {
			compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
			if (!image || !(image->has_image())) continue;
			const int width  = image->get_size().x;
			const int height = image->get_size().y;
			if (width <= 1 || height <= 1) continue;

			const bool flipColor = mappingLayer.get_flip_color();			// 色反転.
			const bool flipH     = mappingLayer.get_horizontal_flip();		// 左右反転.
			const bool flipV     = mappingLayer.get_vertical_flip();		// 上下反転.
			const bool rotate90  = mappingLayer.get_swap_axes();			// 90度回転.
			const int channelMix = mappingLayer.get_channel_mix();			// イメージのチャンネル.
			const bool useChannelMix = (channelMix == sxsdk::enums::mapping_grayscale_alpha_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_red_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_green_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_blue_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_average_mode);

			if (!newImage) {
				newImage = m_pScene->create_image_interface(dstTexSize);
				newWidth    = dstTexSize.x;
				newHeight   = dstTexSize.y;
				newRepeatX  = repeatU;
				newRepeatY  = repeatV;
				newTexCoord = mappingLayer.get_uv_mapping();
				rgbaLine.resize(newWidth);
				rgbaLine0.resize(newWidth);
				if (hasWeightTex) rgbaWeightLine.resize(newWidth);

				// マスターイメージを持つか調べる.
				pNewMasterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);

				if (pNewMasterImage) {
					// そのまま画像を採用する可能性があるかどうか.
					if (singleSimpleMapping) {
						singleSimpleMapping = (!flipColor && !flipH && !flipV && !rotate90 && !useChannelMix);
					}
					newTexName = pNewMasterImage->get_name();
				}
			}
			if (newTexCoord != mappingLayer.get_uv_mapping()) continue;
			sx::vec<int,2> newImgSize = newImage->get_size();
			compointer<sxsdk::image_interface> image2 = m_duplicateImage(image, newImgSize, flipColor, flipH, flipV, rotate90, repeatU, repeatV);

			compointer<sxsdk::image_interface> weightImage2;
			if (weightWidth > 0) {
				weightImage2 = m_duplicateImage(weightImage, newImgSize, weightFlipColor, weightFlipH, weightFlipV, weightRotate90, weightRepeatU, weightRepeatV);
			}

			// アルファ値を保持するバッファを作成.
			if (i == diffuseAlphaLayerIndex) {
				alphaBuff.resize(newWidth * newHeight, 255);
			}

			for (int y = 0; y < newHeight; ++y) {
				image2->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));

				if (weightWidth > 0) {
					weightImage2->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaWeightLine[0]));
				}

				// チャンネルの合成モード により、色を埋める.
				if (useChannelMix) {
					float fVal;
					if (channelMix == sxsdk::enums::mapping_grayscale_alpha_mode) {
						for (int x = 0; x < newWidth; ++x) {
							fVal = rgbaLine[x].alpha;
							rgbaLine[x] = sxsdk::rgba_class(fVal, fVal, fVal, 1.0f);
						}
					} else if (channelMix == sxsdk::enums::mapping_grayscale_red_mode) {
						for (int x = 0; x < newWidth; ++x) {
							fVal = rgbaLine[x].red;
							rgbaLine[x] = sxsdk::rgba_class(fVal, fVal, fVal, 1.0f);
						}
					} else if (channelMix == sxsdk::enums::mapping_grayscale_green_mode) {
						for (int x = 0; x < newWidth; ++x) {
							fVal = rgbaLine[x].green;
							rgbaLine[x] = sxsdk::rgba_class(fVal, fVal, fVal, 1.0f);
						}
					} else if (channelMix == sxsdk::enums::mapping_grayscale_blue_mode) {
						for (int x = 0; x < newWidth; ++x) {
							fVal = rgbaLine[x].blue;
							rgbaLine[x] = sxsdk::rgba_class(fVal, fVal, fVal, 1.0f);
						}
					} else if (channelMix == sxsdk::enums::mapping_grayscale_average_mode) {
						for (int x = 0; x < newWidth; ++x) {
							fVal = (rgbaLine[x].red + rgbaLine[x].green + rgbaLine[x].blue) * 0.3333f;
							rgbaLine[x] = sxsdk::rgba_class(fVal, fVal, fVal, 1.0f);
						}
					}
				}

				// Roughnessの場合、Shade3Dはテクスチャの濃淡は逆転している.
				if (mappingType == sxsdk::enums::roughness_mapping) {
					for (int x = 0; x < newWidth; ++x) {
						rgbaLine[x].red   = 1.0f - rgbaLine[x].red;
						rgbaLine[x].green = 1.0f - rgbaLine[x].green;
						rgbaLine[x].blue  = 1.0f - rgbaLine[x].blue;
					}
				}

				// アルファ値を保持.
				if (i == diffuseAlphaLayerIndex) {
					const int iPos = y * newWidth;
					for (int x = 0; x < newWidth; ++x) {
						alphaBuff[x + iPos] = (unsigned char)std::min((int)(rgbaLine[x].alpha * 255.0f), 255);
					}
				}

				if (counter == 0) {
					for (int x = 0; x < newWidth; ++x) {
						rgbaLine[x] = rgbaLine[x] * weight + whiteCol * weight2;
					}

				} else {
					newImage->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine0[0]));

					if (blendMode == sxsdk::enums::mapping_blend_mode) {		// 「通常」合成.
						if (weightWidth > 0) {		// 「マット」を考慮.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = rgbaWeightLine[x].red * weight;
								const float w2 = 1.0f - w;
								rgbaLine[x] = rgbaLine[x] * w + rgbaLine0[x] * w2;
							}

						} else {
							for (int x = 0; x < newWidth; ++x) {
								rgbaLine[x] = rgbaLine[x] * weight + rgbaLine0[x] * weight2;
							}
						}

					} else if (blendMode == sxsdk::enums::mapping_mul_mode) {	// 「乗算 (レガシー)」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x] = rgbaLine[x] * rgbaLine0[x] * weight;
						}
					} else if (blendMode == 7) {							// 「乗算」合成.
						if (weightWidth > 0) {		// 「マット」を考慮.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = rgbaWeightLine[x].red * weight;
								const float w2 = 1.0f - w;
								rgbaLine[x] = (whiteCol * w2 + rgbaLine[x] * w) * rgbaLine0[x];
							}

						} else {
							// TODO : 法線マップ時は正しくない.
							for (int x = 0; x < newWidth; ++x) {
								rgbaLine[x] = (whiteCol * weight2 + rgbaLine[x] * weight) * rgbaLine0[x];
							}
						}

					} else if (blendMode == sxsdk::enums::mapping_add_mode) {		// 「加算」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x] = rgbaLine0[x] + (rgbaLine[x] * weight);
							rgbaLine[x].red   = std::min(std::max(0.0f, rgbaLine[x].red), 1.0f);
							rgbaLine[x].green = std::min(std::max(0.0f, rgbaLine[x].green), 1.0f);
							rgbaLine[x].blue  = std::min(std::max(0.0f, rgbaLine[x].blue), 1.0f);
							rgbaLine[x].alpha = std::min(std::max(0.0f, rgbaLine[x].alpha), 1.0f);
						}
					} else if (blendMode == sxsdk::enums::mapping_sub_mode) {		// 「減算」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x] = rgbaLine0[x] - (rgbaLine[x] * weight);
							rgbaLine[x].red   = std::min(std::max(0.0f, rgbaLine[x].red), 1.0f);
							rgbaLine[x].green = std::min(std::max(0.0f, rgbaLine[x].green), 1.0f);
							rgbaLine[x].blue  = std::min(std::max(0.0f, rgbaLine[x].blue), 1.0f);
							rgbaLine[x].alpha = std::min(std::max(0.0f, rgbaLine[x].alpha), 1.0f);
						}
					} else if (blendMode == sxsdk::enums::mapping_min_mode) {		// 「比較(暗)」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x].red   = std::min(rgbaLine0[x].red,   rgbaLine[x].red   * weight);
							rgbaLine[x].green = std::min(rgbaLine0[x].green, rgbaLine[x].green * weight);
							rgbaLine[x].blue  = std::min(rgbaLine0[x].blue,  rgbaLine[x].blue  * weight);
						}
					} else if (blendMode == sxsdk::enums::mapping_max_mode) {		// 「比較(明)」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x].red   = std::max(rgbaLine0[x].red,   rgbaLine[x].red   * weight);
							rgbaLine[x].green = std::max(rgbaLine0[x].green, rgbaLine[x].green * weight);
							rgbaLine[x].blue  = std::max(rgbaLine0[x].blue,  rgbaLine[x].blue  * weight);
						}
					}
				}
				newImage->set_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));
			}
			counter++;

			// 法線マップは1枚のみを採用.
			//if (mappingType == sxsdk::enums::normal_mapping) break;

		} catch (...) { }
	}
	if (!newImage) return false;

	// 複数のテクスチャをベイクした場合.
	if (counter >= 2) {
		if (singleSimpleMapping) singleSimpleMapping = false;
	}

	// アルファ透明のアルファ値で、アルファ値を上書き.
	if (!alphaBuff.empty()) {
		for (int y = 0, iPos = 0; y < newHeight; ++y, iPos += newWidth) {
			newImage->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));
			for (int x = 0; x < newWidth; ++x) {
				rgbaLine[x].alpha = (float)alphaBuff[x + iPos] / 255.0f;
			}
			newImage->set_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));
		}
	}

	if (mappingType == sxsdk::enums::diffuse_mapping) {
		m_diffuseImage    = newImage;
		m_diffuseRepeat   = repeatTex;
		m_diffuseTexCoord = newTexCoord;
	}
	if (mappingType == sxsdk::enums::normal_mapping) {
		m_normalImage    = newImage;
		m_normalRepeat   = repeatTex;
		m_normalTexCoord = newTexCoord;
	}
	if (mappingType == sxsdk::enums::reflection_mapping) {
		m_reflectionImage    = newImage;
		m_reflectionRepeat   = repeatTex;
		m_reflectionTexCoord = newTexCoord;
	}
	if (mappingType == sxsdk::enums::roughness_mapping) {
		m_roughnessImage    = newImage;
		m_roughnessRepeat   = repeatTex;
		m_roughnessTexCoord = newTexCoord;
	}
	if (mappingType == sxsdk::enums::glow_mapping) {
		m_glowImage    = newImage;
		m_glowRepeat   = repeatTex;
		m_glowTexCoord = newTexCoord;
	}

	return true;
}

namespace {
	sxsdk::vec3 rgb_to_hsv (const sxsdk::rgb_class& col) {
		float h_val, s_val, v_val;
		float min_val;
		sxsdk::vec3 rgb;

		rgb = sxsdk::vec3(col.red, col.green, col.blue);
		if (rgb.x < rgb.y) {
			if (rgb.y < rgb.z) v_val = rgb.z;
			else v_val = rgb.y;
		} else {
			if (rgb.x < rgb.z) v_val = rgb.z;
			else v_val = rgb.x;
		}
		if (rgb.x < rgb.y) {
			if (rgb.x < rgb.z) min_val = rgb.x;
			else min_val = rgb.z;
		} else {
			if (rgb.y < rgb.z) min_val = rgb.y;
			else min_val = rgb.z;
		}
		if (sx::zero(v_val)) s_val = 0.0f;
		else s_val = (v_val - min_val) / v_val;

		if (sx::zero(v_val - min_val)) {
			rgb = sxsdk::vec3(0.0f, 0.0f, 0.0f);
		} else {
			rgb.x = (v_val - col.red)   / (v_val - min_val);
			rgb.y = (v_val - col.green) / (v_val - min_val);
			rgb.z = (v_val - col.blue)  / (v_val - min_val);
		}
		if (sx::zero(v_val - col.red)) h_val = 60.0f * (rgb.z - rgb.y);
		else if (sx::zero(v_val - col.green)) h_val = 60.0f * (2.0f + rgb.x - rgb.z);
		else h_val = 60.0f * (4.0f + rgb.y - rgb.x);
		if (h_val < 0.0f) h_val = h_val + 360.0f;

		return sxsdk::vec3(h_val, s_val, v_val);
	}

	sxsdk::rgb_class hsv_to_rgb (const sxsdk::vec3& hsv) {
		const int i_val = (int)floor(hsv.x / 60.0f);
		float fl = (hsv.x / 60.0f) - (float)i_val;
		if(!(i_val & 1)) fl = 1.0 - fl;

		const float m = hsv.z * (1.0f - hsv.y);
		const float n = hsv.z * (1.0f - hsv.y * fl);

		sxsdk::rgb_class col;
		switch (i_val) {
		case 0:
			col = sxsdk::rgb_class(hsv.z, n, m);
			break;
		case 1:
			col = sxsdk::rgb_class(n, hsv.z, m);
			break;
		case 2:
			col = sxsdk::rgb_class(m, hsv.z, n);
			break;
		case 3:
			col = sxsdk::rgb_class(m, (float)n, hsv.z);
			break;
		case 4:
			col = sxsdk::rgb_class(n, (float)m, hsv.z);
			break;
		case 5:
			col = sxsdk::rgb_class(hsv.z, m, n);
			break;
		}
		return col;
	}
}

/**
 * diffuse/roughness/metallicのテクスチャを、Shade3Dの状態からPBRマテリアルに変換.
 */
void CImagesBlend::m_convShade3DToPBRMaterial ()
{
	/*
const sxsdk::rgb_class col0 = surface->get_diffuse_color();    
sxsdk::rgb_class col = col0 * (surface->get_diffuse());    
sxsdk::rgb_class reflectionCol = surface->get_reflection_color();    
const float reflectionV  = std::max(std::min(1.0f, surface->get_reflection()), 0.0f);    
const float reflectionV2 = 1.0f - reflectionV;    
col = col * reflectionV2 + reflectionCol * reflectionV;    
col.red   = std::min(col0.red, col.red);    
col.green = std::min(col0.green, col.green);    
col.blue  = std::min(col0.blue, col.blue);   
	*/

	if (m_diffuseImage && m_reflectionImage) {
		const float diffuseV = m_surface->get_diffuse();

		const int width  = m_diffuseImage->get_size().x;
		const int height = m_diffuseImage->get_size().y;
		const int widthM  = m_reflectionImage->get_size().x;
		const int heightM = m_reflectionImage->get_size().y;
		if (width == widthM && height == heightM) {
			std::vector<sxsdk::rgba_class> lineCols, lineCols2;
			lineCols.resize(width);
			lineCols2.resize(width);

			sxsdk::rgba_class col;
			sxsdk::rgb_class reflectionCol = m_surface->get_reflection_color();
			sxsdk::rgb_class c0;
			float reflectionV, reflectionV2;
			float scaleV;
			sxsdk::vec3 hsv;

			for (int y = 0; y < height; ++y) {
				m_diffuseImage->get_pixels_rgba_float(0, y, width, 1, &(lineCols[0]));
				m_reflectionImage->get_pixels_rgba_float(0, y, width, 1, &(lineCols2[0]));
				for (int x = 0; x < width; ++x) {
					// Diffuse色を取得し、HSVに変換.
					col = lineCols[x];
					hsv = ::rgb_to_hsv(sxsdk::rgb_class(col.red, col.green, col.blue));

					reflectionV  = lineCols2[x].red;		// Metallic値.
					reflectionV2 = std::min(1.0f, reflectionV * 2.0f);

					// 明度(V)をMetallic値で調整.
					hsv.z = std::max(reflectionV2, hsv.z);

					// 彩度(S)をMetallic値で調整.
					hsv.y = hsv.y * (1.0f - reflectionV * 0.8f);	//std::min(hsv.y, (1.0f - reflectionV * 0.8f));

					// HSVからRGBに変換して格納.
					c0 = ::hsv_to_rgb(hsv);
					col.red   = c0.red;
					col.green = c0.green;
					col.blue  = c0.blue;

/*
					reflectionV  = lineCols2[x].red;
					reflectionV2 = 1.0f - reflectionV;
					scaleV = reflectionV + 1.0f;

					col.red   = std::min(col.red * scaleV, 1.0f);
					col.green = std::min(col.green * scaleV, 1.0f);
					col.blue  = std::min(col.blue * scaleV, 1.0f);
*/
					lineCols[x] = col;
				}
				m_diffuseImage->set_pixels_rgba_float(0, y, width, 1, &(lineCols[0]));
			}

		}
	}
}

/**
 * 各種イメージより、エクスポートするテクスチャを作成.
 */
bool CImagesBlend::calcExportImages ()
{
	const sxsdk::rgba_class whiteCol(1, 1, 1, 1);
	const sxsdk::rgba_class blackCol(0, 0, 0, 1);

	if (m_exportDiffuseImage) m_exportDiffuseImage->Release();
	if (m_exportMetallicImage) m_exportMetallicImage->Release();
	if (m_exportRoughnessImage) m_exportRoughnessImage->Release();
	if (m_exportOpacityImage) m_exportOpacityImage->Release();

	if (!m_diffuseImage && m_diffuseMasterImage) {
		m_exportDiffuseImage = m_diffuseMasterImage->get_image()->duplicate_image();
	}
	if (!m_reflectionImage && m_reflectionMasterImage) {
		m_exportMetallicImage = m_reflectionMasterImage->get_image()->duplicate_image();
	}
	if (!m_roughnessImage && m_roughnessMasterImage) {
		m_exportRoughnessImage = m_roughnessMasterImage->get_image()->duplicate_image();
	}

	return true;
}

/**
 * 各種イメージを持つか (単一または複数).
 */
bool CImagesBlend::hasImage (const sxsdk::enums::mapping_type mappingType) const
{
	if (mappingType == sxsdk::enums::diffuse_mapping) return m_hasDiffuseImage;
	if (mappingType == sxsdk::enums::reflection_mapping) return m_hasReflectionImage;
	if (mappingType == sxsdk::enums::roughness_mapping) return m_hasRoughnessImage;
	if (mappingType == sxsdk::enums::normal_mapping) return m_hasNormalImage;
	if (mappingType == sxsdk::enums::glow_mapping) return m_hasGlowImage;
	return false;
}

/**
 * 単一テクスチャを参照する場合のマスターイメージクラスを取得.
 */
sxsdk::master_image_class* CImagesBlend::getSingleMasterImage (const sxsdk::enums::mapping_type mappingType)
{
	if (mappingType == sxsdk::enums::diffuse_mapping) return m_diffuseMasterImage;
	if (mappingType == sxsdk::enums::reflection_mapping) return m_reflectionMasterImage;
	if (mappingType == sxsdk::enums::roughness_mapping) return m_roughnessMasterImage;
	if (mappingType == sxsdk::enums::normal_mapping) return m_normalMasterImage;
	if (mappingType == sxsdk::enums::glow_mapping) return m_glowMasterImage;
	return NULL;
}

/**
 * イメージを取得.
 */
compointer<sxsdk::image_interface> CImagesBlend::getImage (const sxsdk::enums::mapping_type mappingType)
{
	if (mappingType == sxsdk::enums::diffuse_mapping) return m_diffuseImage;
	if (mappingType == sxsdk::enums::reflection_mapping) return m_reflectionImage;
	if (mappingType == sxsdk::enums::roughness_mapping) return m_roughnessImage;
	if (mappingType == sxsdk::enums::normal_mapping) return m_normalImage;
	if (mappingType == sxsdk::enums::glow_mapping) return m_glowImage;
	return compointer<sxsdk::image_interface>();
}

/**
 * イメージのUV層番号を取得.
 */
int CImagesBlend::getTexCoord (const sxsdk::enums::mapping_type mappingType)
{
	if (mappingType == sxsdk::enums::diffuse_mapping) return m_diffuseTexCoord;
	if (mappingType == sxsdk::enums::reflection_mapping) return m_reflectionTexCoord;
	if (mappingType == sxsdk::enums::roughness_mapping) return m_roughnessTexCoord;
	if (mappingType == sxsdk::enums::normal_mapping) return m_normalTexCoord;
	if (mappingType == sxsdk::enums::glow_mapping) return m_glowTexCoord;
	return 0;
}

/**
 * イメージの反復回数を取得.
 */
sx::vec<int,2> CImagesBlend::getImageRepeat (const sxsdk::enums::mapping_type mappingType)
{
	if (mappingType == sxsdk::enums::diffuse_mapping) return m_diffuseRepeat;
	if (mappingType == sxsdk::enums::reflection_mapping) return m_reflectionRepeat;
	if (mappingType == sxsdk::enums::roughness_mapping) return m_roughnessRepeat;
	if (mappingType == sxsdk::enums::normal_mapping) return m_normalRepeat;
	if (mappingType == sxsdk::enums::glow_mapping) return m_glowRepeat;
	return sx::vec<int,2>(1, 1);
}
