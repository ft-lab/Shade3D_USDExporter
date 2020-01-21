/**
 * Export用にShade3Dの表面材質のマッピングレイヤのテクスチャを合成.
 */
#include "ImagesBlend.h"
#include "MathUtil.h"
#include "Shade3DUtil.h"
#include "StreamCtrl.h"
#include "USDData.h"

#include <math.h>

/*
	Shade3Dの「透明」「不透明マスク」「チャンネル合成のアルファ透明」は、すべてOpacityのテクスチャに格納される.
*/

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
	m_opacityRepeat    = sx::vec<int,2>(1, 1);

	m_diffuseTexCoord     = 0;
	m_normalTexCoord      = 0;
	m_reflectionTexCoord  = 0;
	m_roughnessTexCoord   = 0;
	m_glowTexCoord        = 0;
	m_occlusionTexCoord   = 0;
	m_opacityTexCoord     = 0;

	m_diffuseAlphaTrans = false;
	m_occlusionWeight = 1.0f;

	m_hasDiffuseImage    = false;
	m_hasReflectionImage = false;
	m_hasRoughnessImage  = false;
	m_hasNormalImage     = false;
	m_hasGlowImage       = false;
	m_hasOcclusionImage  = false;
	m_hasOpacityImage    = false;

	m_diffuseColor  = sxsdk::rgb_class(1.0f, 1.0f, 1.0f);
	m_emissiveColor = sxsdk::rgb_class(0.0f, 0.0f, 0.0f);
	m_metallic  = 0.0f;
	m_roughness = 0.0f;

	m_diffuseTexturesCount = 0;
	m_useDiffuseAlpha = false;

	// Shade3Dでの表面材質のマッピングレイヤで、加工無しの画像が参照されているかチェック.
	m_checkSingleImage(sxsdk::enums::diffuse_mapping, &m_diffuseMasterImage, m_diffuseTexCoord, m_diffuseRepeat, m_hasDiffuseImage);
	m_checkSingleImage(sxsdk::enums::normal_mapping, &m_normalMasterImage, m_normalTexCoord, m_normalRepeat, m_hasNormalImage);
	m_checkSingleImage(sxsdk::enums::reflection_mapping, &m_reflectionMasterImage, m_reflectionTexCoord, m_reflectionRepeat, m_hasReflectionImage);
	m_checkSingleImage(sxsdk::enums::roughness_mapping, &m_roughnessMasterImage, m_roughnessTexCoord, m_roughnessRepeat, m_hasRoughnessImage);
	m_checkSingleImage(sxsdk::enums::glow_mapping, &m_glowMasterImage, m_glowTexCoord, m_glowRepeat, m_hasGlowImage);
	m_checkSingleImage(MAPPING_TYPE_OPACITY, &m_opacityMasterImage, m_opacityTexCoord, m_opacityRepeat, m_hasOpacityImage);

	// マッピングレイヤのOcclusion情報を取得.
	m_occlusionWeight = 1.0f;
	//m_checkOcclusionSingleImage(&m_occlusionMasterImage, m_occlusionTexCoord, m_occlusionRepeat, m_hasOcclusionImage);

	// Diffuseのアルファ透明を使用しているかチェック.
	//m_diffuseAlphaTrans = m_checkDiffuseAlphaTrans();

	// Shade3Dでの表面材質のマッピングレイヤごとに、各種イメージを合成.
	if (!m_diffuseMasterImage && m_hasDiffuseImage) m_blendImages(sxsdk::enums::diffuse_mapping, m_diffuseRepeat);
	if (!m_normalMasterImage && m_hasNormalImage) m_blendImages(sxsdk::enums::normal_mapping, m_normalRepeat);
	if (!m_reflectionMasterImage && m_hasReflectionImage) m_blendImages(sxsdk::enums::reflection_mapping, m_reflectionRepeat);
	if (!m_roughnessMasterImage && m_hasRoughnessImage) m_blendImages(sxsdk::enums::roughness_mapping, m_roughnessRepeat);
	if (!m_glowMasterImage && m_hasGlowImage) m_blendImages(sxsdk::enums::glow_mapping, m_glowRepeat);
	if (!m_opacityMasterImage && m_hasOpacityImage) m_blendImages(MAPPING_TYPE_OPACITY, m_opacityRepeat);

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

	if (mappingType == sxsdk::enums::diffuse_mapping) {
		m_diffuseTexturesCount = 0;
		m_useDiffuseAlpha = false;
	}

	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.
		if (Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) continue;

		const float weight = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;

		const int type = mappingLayer.get_type();
		if (mappingType == MAPPING_TYPE_OPACITY) {
			if (type == sxsdk::enums::diffuse_mapping) {
				if (mappingLayer.get_channel_mix() != sxsdk::enums::mapping_transparent_alpha_mode) continue;
			} else if (type != sxsdk::enums::transparency_mapping && type != MAPPING_TYPE_OPACITY) continue;
		} else {
			if (type != mappingType) continue;
		}

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

		if (mappingType == sxsdk::enums::diffuse_mapping) {
			if (mappingLayer.get_channel_mix() == sxsdk::enums::mapping_transparent_alpha_mode) {
				m_useDiffuseAlpha = true;
			}
			m_diffuseTexturesCount++;
		}

		counter++;
	}

	// 「不透明マスク」の場合は強制的にベイクとする.
	if (mappingType == MAPPING_TYPE_OPACITY) singleImage = false;

	// 「アルファ透明」を持つ単一のdiffuseテクスチャの場合は、強制的にベイクとする.
	//if (mappingType == sxsdk::enums::diffuse_mapping) {
	//	if (m_useDiffuseAlpha && m_diffuseTexturesCount == 1) {
	//		singleImage = false;
	//	}
	//}

	if (!texRepeatSingle) texRepeat = sx::vec<int,2>(1, 1);
	if (counter >= 1) hasImage = true;
	if (singleImage && counter == 1) {
		*ppMasterImage = pRetMasterImage;
		return true;
	}
	return false;
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
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.
		if (Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) continue;

		const float weight = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;

		const int type = mappingLayer.get_type();
		if (mappingType == MAPPING_TYPE_OPACITY) {
			if (type == sxsdk::enums::diffuse_mapping) {
				if (mappingLayer.get_channel_mix() != sxsdk::enums::mapping_transparent_alpha_mode) continue;
			} else if (type != sxsdk::enums::transparency_mapping && type != MAPPING_TYPE_OPACITY) continue;
		} else {
			if (type != mappingType) continue;
		}

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
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.
		if (Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) continue;

		const float weight = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;

		const int type = mappingLayer.get_type();
		if (mappingType == MAPPING_TYPE_OPACITY) {
			if (type == sxsdk::enums::diffuse_mapping) {
				if (mappingLayer.get_channel_mix() != sxsdk::enums::mapping_transparent_alpha_mode) continue;
			} else if (type != sxsdk::enums::transparency_mapping && type != MAPPING_TYPE_OPACITY) continue;
		} else {
			if (type != mappingType) continue;
		}

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

		try {
			compointer<sxsdk::graphic_context_interface> gc(dstImage->get_graphic_context_interface());
			gc->set_color(sxsdk::rgb_class(1, 1, 1));
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
			gc->restore_color();
		} catch (...) { }

	} else {
		dstImage = image->duplicate_image(&dstSize);
	}

	if (flipColor) {
		for (int y = 0; y < height; ++y) {
			dstImage->get_pixels_rgba(0, y, width, 1, &(lineCols[0]));
			for (int x = 0; x < width; ++x) {
				lineCols[x].red   = 255 - lineCols[x].red;
				lineCols[x].green = 255 - lineCols[x].green;
				lineCols[x].blue  = 255 - lineCols[x].blue;
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

	sxsdk::rgba_class baseCol(1, 1, 1, 1);
	if (mappingType == sxsdk::enums::diffuse_mapping) {
		baseCol = sxsdk::rgba_class(m_surface->get_diffuse_color());
	}

	// 法線の中立値.
	const sxsdk::rgb_class normalDefaultCol(0.5f, 0.5f, 1.0f);
	const sxsdk::vec3 normalDefault = MathUtil::convRGBToNormal(normalDefaultCol);
	if (mappingType == sxsdk::enums::normal_mapping) {
		whiteCol = sxsdk::rgba_class(normalDefaultCol.red, normalDefaultCol.green, normalDefaultCol.blue, 1.0f);
		baseCol = whiteCol;
	}

	// 合成するテクスチャサイズ.
	const sx::vec<int,2> dstTexSize = m_getMaxMappingImageSize(mappingType);
	if (dstTexSize.x == 0 || dstTexSize.y == 0) return false;

	// 「マット」を持つか.
	const bool hasWeightTex = m_hasWeightTexture(mappingType);

	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.
		if (Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) continue;

		const float weight  = std::min(std::max(mappingLayer.get_weight(), 0.0f), 1.0f);
		const float weight2 = 1.0f - weight;
		if (MathUtil::isZero(weight)) continue;

		bool alphaTrans = false;
		const int type = mappingLayer.get_type();
		if (mappingType == MAPPING_TYPE_OPACITY) {
			if (type == sxsdk::enums::diffuse_mapping) {
				if (mappingLayer.get_channel_mix() != sxsdk::enums::mapping_transparent_alpha_mode) continue;
				alphaTrans = true;
			} else if (type != sxsdk::enums::transparency_mapping && type != MAPPING_TYPE_OPACITY) continue;
		} else {
			if (type != mappingType) continue;
		}

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

			// Diffuseテクスチャの場合はAlpha要素をRGBに入れる。Transparancyテクスチャの場合はRGBの値を反転する.
			if (mappingType == MAPPING_TYPE_OPACITY) {
				compointer<sxsdk::image_interface> image2(image->duplicate_image());

				std::vector<sxsdk::rgba_class> colA;
				float fA;
				colA.resize(width);
				if (type == sxsdk::enums::diffuse_mapping) {
					for (int y = 0; y < height; ++y) {
						image2->get_pixels_rgba_float(0, y, width, 1, &(colA[0]));
						for (int x = 0; x < width; ++x) {
							fA = colA[x].alpha;
							colA[x] = sxsdk::rgba_class(fA, fA, fA, 1.0f);
						}
						image2->set_pixels_rgba_float(0, y, width, 1, &(colA[0]));
					}
				} else if (type == sxsdk::enums::transparency_mapping) {
					for (int y = 0; y < height; ++y) {
						image2->get_pixels_rgba_float(0, y, width, 1, &(colA[0]));
						for (int x = 0; x < width; ++x) {
							fA = 1.0f - colA[x].alpha;
							colA[x] = sxsdk::rgba_class(fA, fA, fA, 1.0f);
						}
						image2->set_pixels_rgba_float(0, y, width, 1, &(colA[0]));
					}
				}
				image->Release();
				image = image2;
			}

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

				if (counter == 0) {
					float aV;
					if (blendMode == 7) {							// 「乗算」合成.
						for (int x = 0; x < newWidth; ++x) {
							const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
							const float w2 = 1.0f - w;
							rgbaLine[x] = (baseCol * w2 + rgbaLine[x] * w) * baseCol;
						}

					} else {										// 「通常」合成.
						for (int x = 0; x < newWidth; ++x) {
							const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
							const float w2 = 1.0f - w;
							rgbaLine[x] = rgbaLine[x] * w + baseCol * w2;
						}
					}

				} else {
					newImage->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine0[0]));

					if (mappingType == sxsdk::enums::normal_mapping) {
						sxsdk::vec3 n, n2;
						sxsdk::rgb_class col;

						for (int x = 0; x < newWidth; ++x) {
							const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
							const float w2 = 1.0f - w;
							n  = MathUtil::convRGBToNormal(sxsdk::rgb_class(rgbaLine[x].red, rgbaLine[x].green, rgbaLine[x].blue));
							n2 = MathUtil::convRGBToNormal(sxsdk::rgb_class(rgbaLine0[x].red, rgbaLine0[x].green, rgbaLine0[x].blue));

							n = n * w + n2 * w2;
							col = MathUtil::convNormalToRGB(n);
							rgbaLine[x] = sxsdk::rgba_class(col.red, col.green, col.blue, 1.0f);
						}

					} else {
						if (blendMode == sxsdk::enums::mapping_blend_mode) {		// 「通常」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								const float w2 = 1.0f - w;
								rgbaLine[x] = rgbaLine[x] * w + rgbaLine0[x] * w2;
							}

						} else if (blendMode == sxsdk::enums::mapping_mul_mode) {	// 「乗算 (レガシー)」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								const float w2 = 1.0f - w;
								rgbaLine[x] = rgbaLine[x] * rgbaLine0[x] * w;
							}
						} else if (blendMode == 7) {							// 「乗算」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								const float w2 = 1.0f - w;

								rgbaLine[x] = (whiteCol * w2 + rgbaLine[x] * w) * rgbaLine0[x];
							}

						} else if (blendMode == sxsdk::enums::mapping_add_mode) {		// 「加算」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								rgbaLine[x] = rgbaLine0[x] + (rgbaLine[x] * w);
								rgbaLine[x].red   = std::min(std::max(0.0f, rgbaLine[x].red), 1.0f);
								rgbaLine[x].green = std::min(std::max(0.0f, rgbaLine[x].green), 1.0f);
								rgbaLine[x].blue  = std::min(std::max(0.0f, rgbaLine[x].blue), 1.0f);
								rgbaLine[x].alpha = std::min(std::max(0.0f, rgbaLine[x].alpha), 1.0f);
							}
						} else if (blendMode == sxsdk::enums::mapping_sub_mode) {		// 「減算」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								rgbaLine[x] = rgbaLine0[x] - (rgbaLine[x] * w);
								rgbaLine[x].red   = std::min(std::max(0.0f, rgbaLine[x].red), 1.0f);
								rgbaLine[x].green = std::min(std::max(0.0f, rgbaLine[x].green), 1.0f);
								rgbaLine[x].blue  = std::min(std::max(0.0f, rgbaLine[x].blue), 1.0f);
								rgbaLine[x].alpha = std::min(std::max(0.0f, rgbaLine[x].alpha), 1.0f);
							}
						} else if (blendMode == sxsdk::enums::mapping_min_mode) {		// 「比較(暗)」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								rgbaLine[x].red   = std::min(rgbaLine0[x].red,   rgbaLine[x].red   * w);
								rgbaLine[x].green = std::min(rgbaLine0[x].green, rgbaLine[x].green * w);
								rgbaLine[x].blue  = std::min(rgbaLine0[x].blue,  rgbaLine[x].blue  * w);
							}
						} else if (blendMode == sxsdk::enums::mapping_max_mode) {		// 「比較(明)」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								rgbaLine[x].red   = std::max(rgbaLine0[x].red,   rgbaLine[x].red   * w);
								rgbaLine[x].green = std::max(rgbaLine0[x].green, rgbaLine[x].green * w);
								rgbaLine[x].blue  = std::max(rgbaLine0[x].blue,  rgbaLine[x].blue  * w);
							}
						}
					}
				}
				newImage->set_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));
			}
			counter++;

		} catch (...) { }
	}
	if (!newImage) return false;

	// 複数のテクスチャをベイクした場合.
	if (counter >= 2) {
		if (singleSimpleMapping) singleSimpleMapping = false;
	}

#if 0
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
#endif

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
	if (mappingType == MAPPING_TYPE_OPACITY) {
		m_opacityImage    = newImage;
		m_opacityRepeat   = repeatTex;
		m_opacityTexCoord = newTexCoord;
	}

	return true;
}

/**
 * diffuse/roughness/metallicのテクスチャを、Shade3Dの状態からPBRマテリアルに変換.
 */
void CImagesBlend::m_convShade3DToPBRMaterial ()
{
	/*
	m_diffuseColor  = sxsdk::rgb_class(1.0f, 1.0f, 1.0f);
	m_emissiveColor = sxsdk::rgb_class(0.0f, 0.0f, 0.0f);
	m_metallic  = 0.0f;
	m_roughness = 0.0f;
	*/

	/*
 
	*/
	if (!m_diffuseImage && !m_reflectionImage) {

	}

	// 反射が大きい場合にbaseColorを黒にするとglTFとして見たときは黒くなるため、reflectionも考慮.
	// なお、ここでの「色」はテクスチャに乗算するものなので、リニア変換は行わない.
	// 「拡散反射色」については、テクスチャ作成時にすでに考慮しているのでここでは入れない.
	const float diffuseV = std::min(1.0f, std::max(0.0f, m_surface->get_diffuse()));
	const sxsdk::rgb_class col0 = m_diffuseImage ? sxsdk::rgb_class(1, 1, 1) : (m_surface->get_diffuse_color());
	sxsdk::rgb_class col = col0 * diffuseV;
	sxsdk::rgb_class reflectionCol = m_surface->get_reflection_color();
	const float reflectionV  = std::max(std::min(1.0f, m_surface->get_reflection()), 0.0f);
	const float reflectionV2 = 1.0f - reflectionV;
	col = col * reflectionV2 + reflectionCol * reflectionV;
	m_diffuseColor.red   = std::min(col0.red, col.red);
	m_diffuseColor.green = std::min(col0.green, col.green);
	m_diffuseColor.blue  = std::min(col0.blue, col.blue);

	m_metallic  = reflectionV;
	m_roughness = m_surface->get_roughness();

	// DiffuseとOpacityの両方が存在する場合、かつ、1つの「アルファ透明」を使用したテクスチャをベイクする場合、DiffuseのA要素としてOpacityを格納.
	if (m_useDiffuseAlpha && m_diffuseTexturesCount == 1) {
		if (m_diffuseImage && m_opacityImage) {
			if (m_diffuseTexCoord == m_opacityTexCoord && m_diffuseRepeat == m_opacityRepeat) {
				const int width  = m_diffuseImage->get_size().x;
				const int height = m_diffuseImage->get_size().y;
				compointer<sxsdk::image_interface> optImage(m_opacityImage->duplicate_image(&(sx::vec<int,2>(width, height))));
			
				std::vector<sxsdk::rgba_class> col1A;
				std::vector<sxsdk::rgba_class> col2A;
				col1A.resize(width);
				col2A.resize(height);
				for (int y = 0; y < height; ++y) {
					m_diffuseImage->get_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
					optImage->get_pixels_rgba_float(0, y, width, 1, &(col2A[0]));
					for (int x = 0; x < width; ++x) {
						col1A[x].alpha = col2A[x].red;
					}
					m_diffuseImage->set_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
				}
				m_diffuseAlphaTrans = true;

				m_hasOpacityImage = false;
				m_opacityImage->Release();
			}
		} else if ((!m_diffuseImage && m_diffuseMasterImage) && m_opacityImage) {
			// マスターイメージとしてdiffuse Textureを持ち、Opacity Textureを持つ場合はopacityは不要 (pngとしてRGBAで保持するため).
			m_diffuseAlphaTrans = true;
			m_hasOpacityImage = false;
			m_opacityImage->Release();
		}
	}

	if (m_diffuseImage && m_reflectionImage) {
		const int width  = m_diffuseImage->get_size().x;
		const int height = m_diffuseImage->get_size().y;
		const int widthM  = m_reflectionImage->get_size().x;
		const int heightM = m_reflectionImage->get_size().y;
		if (width == widthM && height == heightM) {
			std::vector<sxsdk::rgba_class> lineCols, lineCols2;
			lineCols.resize(width);
			lineCols2.resize(width);

			sxsdk::rgba_class col;
			sxsdk::rgb_class baseColorCol, metallicCol;
			float rV, d1, d2;
			sxsdk::rgb_class blackCol(0, 0, 0);
			sxsdk::vec3 hsv;

			for (int y = 0; y < height; ++y) {
				m_diffuseImage->get_pixels_rgba_float(0, y, width, 1, &(lineCols[0]));
				m_reflectionImage->get_pixels_rgba_float(0, y, width, 1, &(lineCols2[0]));
				for (int x = 0; x < width; ++x) {
					// Shade3Dのdiffuse/reflection値より、PBRマテリアルでのdiffuse/metallicに変換.
					col = lineCols[x];
					rV  = lineCols2[x].red;

					baseColorCol.red   = std::max(std::min(col.red   + rV, 1.0f), 0.0f);
					baseColorCol.green = std::max(std::min(col.green + rV, 1.0f), 0.0f);
					baseColorCol.blue  = std::max(std::min(col.blue  + rV, 1.0f), 0.0f);
					const float baseV = MathUtil::rgb_to_grayscale(baseColorCol);

					// Metallicの計算.
					float metallicV = 0.0f;
					if (baseV > 0.0f) {
						d1 = 0.7f;
						d2 = 1.0f - d1;
						metallicV = std::min(rV / (baseV * d1 + d2), 1.0f);
					}

					// RGB ==> HSVに変換.
					hsv = MathUtil::rgb_to_hsv(col);

					// Diffuse色の明度をMetallicで調整.
					const float dd = (1.0f - hsv.y) + diffuseV + 1.0f;
					hsv.z = std::max(0.0f, std::min(1.0f, hsv.z * dd));
					hsv.z = hsv.z * (1.0f - (1.0f - diffuseV) * 0.2f); 

					// Metallic成分が強い場合、Diffuse成分が弱い場合は、彩度をわずかに下げる.
					d1 = (1.0f - diffuseV) * 0.8f;
					d2 = 1.0f - d1;

					const float m1 = metallicV * 0.6f;
					float m2 = (1.0f - m1) * d2;
					hsv.y = hsv.y * m2;

					baseColorCol = MathUtil::hsv_to_rgb(hsv);

					lineCols[x].red   = baseColorCol.red;
					lineCols[x].green = baseColorCol.green;
					lineCols[x].blue  = baseColorCol.blue;

					lineCols2[x] = sxsdk::rgba_class(metallicV, metallicV, metallicV, 1.0f);
				}
				m_diffuseImage->set_pixels_rgba_float(0, y, width, 1, &(lineCols[0]));
				m_reflectionImage->set_pixels_rgba_float(0, y, width, 1, &(lineCols2[0]));
			}
		}
	}
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
	if (mappingType == MAPPING_TYPE_OPACITY) return m_hasOpacityImage;
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
	if (mappingType == MAPPING_TYPE_OPACITY) return m_opacityMasterImage;
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
	if (mappingType == MAPPING_TYPE_OPACITY) return m_opacityImage;
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
	if (mappingType == MAPPING_TYPE_OPACITY) return m_opacityTexCoord;
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
	if (mappingType == MAPPING_TYPE_OPACITY) return m_opacityRepeat;
	return sx::vec<int,2>(1, 1);
}

/**
 * イメージの強度を色として取得.
 */
sxsdk::rgb_class CImagesBlend::getImageFactor (const sxsdk::enums::mapping_type mappingType)
{
	sxsdk::rgb_class retCol(1, 1, 1);

	if (mappingType == sxsdk::enums::diffuse_mapping) {
		retCol = m_diffuseColor;
	}
	if (mappingType == sxsdk::enums::glow_mapping) {
		retCol = m_emissiveColor;
	}
	if (mappingType == sxsdk::enums::roughness_mapping) {
		retCol = sxsdk::rgb_class(m_roughness, m_roughness, m_roughness);
	}
	if (mappingType == sxsdk::enums::reflection_mapping) {
		retCol = sxsdk::rgb_class(m_metallic, m_metallic, m_metallic);
	}

	return retCol;
}

