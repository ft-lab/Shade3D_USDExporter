﻿/**
 * Export用にShade3Dの表面材質のマッピングレイヤのテクスチャを合成.
 */
#ifndef _IMAGESBLEND_H
#define _IMAGESBLEND_H

#include "GlobalHeader.h"

class CImagesBlend
{
private:
	sxsdk::scene_interface* m_pScene;
	sxsdk::surface_class* m_surface;

	compointer<sxsdk::image_interface> m_diffuseImage;			// Diffuseの画像.
	compointer<sxsdk::image_interface> m_normalImage;			// Normalの画像.
	compointer<sxsdk::image_interface> m_reflectionImage;		// Reflectionの画像.
	compointer<sxsdk::image_interface> m_roughnessImage;		// Roughnessの画像.
	compointer<sxsdk::image_interface> m_glowImage;				// Glowの画像.
	compointer<sxsdk::image_interface> m_occlusionImage;		// Occlusionの画像.
	compointer<sxsdk::image_interface> m_opacityImage;			// Opacit(不透明マスク相当)の画像.

	bool m_hasDiffuseImage;										// diffuseのイメージを持つか.
	bool m_hasReflectionImage;									// reflectionのイメージを持つか.
	bool m_hasRoughnessImage;									// roughnessのイメージを持つか.
	bool m_hasNormalImage;										// normalのイメージを持つか.
	bool m_hasGlowImage;										// glowのイメージを持つか.
	bool m_hasOcclusionImage;									// Occlusionのイメージを持つか.
	bool m_hasOpacityImage;										// Opacityのイメージを持つか.

	// 以下、マッピングレイヤで1枚のみの加工不要のテクスチャである場合のマスターサーフェスクラスの参照.
	sxsdk::master_image_class* m_diffuseMasterImage;			// Diffuseのマスターイメージクラス.
	sxsdk::master_image_class* m_reflectionMasterImage;			// Reflectionのマスターイメージクラス.
	sxsdk::master_image_class* m_roughnessMasterImage;			// Roughnessのマスターイメージクラス.
	sxsdk::master_image_class* m_normalMasterImage;				// Normalのマスターイメージクラス.
	sxsdk::master_image_class* m_glowMasterImage;				// Glowのマスターイメージクラス.
	sxsdk::master_image_class* m_occlusionMasterImage;			// Occlusionのマスターイメージクラス.
	sxsdk::master_image_class* m_opacityMasterImage;			// Opacityのマスターイメージクラス.

	sx::vec<int,2> m_diffuseRepeat;								// Diffuseの反復回数.
	sx::vec<int,2> m_normalRepeat;								// Normalの反復回数.
	sx::vec<int,2> m_reflectionRepeat;							// Reflectionの反復回数.
	sx::vec<int,2> m_roughnessRepeat;							// Roughnessの反復回数.
	sx::vec<int,2> m_glowRepeat;								// Glowの反復回数.
	sx::vec<int,2> m_occlusionRepeat;							// Occlusionの反復回数.
	sx::vec<int,2> m_opacityRepeat;								// Opacityの反復回数.

	int m_diffuseTexCoord;										// DiffuseのUVレイヤ番号.
	int m_normalTexCoord;										// NormalのUVレイヤ番号.
	int m_reflectionTexCoord;									// ReflectionのUVレイヤ番号.
	int m_roughnessTexCoord;									// RoughnessのUVレイヤ番号.
	int m_glowTexCoord;											// GlowのUVレイヤ番号.
	int m_occlusionTexCoord;									// OcclusionのUVレイヤ番号.
	int m_opacityTexCoord;										// OpacityのUVレイヤ番号.

	bool m_diffuseAlphaTrans;									// Diffuseのアルファ透明を使用しているか.
	float m_occlusionWeight;									// Occlusionのウエイト値.

	// テクスチャを使用しない場合のパラメータ.
	sxsdk::rgb_class m_diffuseColor;							// Diffuse色.
	sxsdk::rgb_class m_emissiveColor;							// Emmisive色.
	float m_metallic;											// Metallic値.
	float m_roughness;											// Roughness値.

private:
	/**
	 * 指定のテクスチャの合成処理.
	 * @param[in] mappingType  マッピングの種類.
	 * @param[in] repeatTex    繰り返し回数.
	 */
	bool m_blendImages (const sxsdk::enums::mapping_type mappingType, const sx::vec<int,2>& repeatTex);

	/**
	 * Diffuseのアルファ透明を使用しているかチェック.
	 */
	bool m_checkDiffuseAlphaTrans ();

	/**
	 * 指定のテクスチャの種類がベイク不要の1枚のテクスチャであるかチェック.
	 * @param[in]  mappingType   マッピングの種類.
	 * @param[out] ppMasterImage master imageの参照を返す.
	 * @param[out] uvTexCoord    UV用の使用テクスチャ層番号を返す.
	 * @param[out] texRepeat     繰り返し回数.
	 * @param[out] hasImage      イメージを持つか (単数または複数).
	 */
	bool m_checkSingleImage (const sxsdk::enums::mapping_type mappingType,
		sxsdk::master_image_class** ppMasterImage,
		int& uvTexCoord,
		sx::vec<int,2>& texRepeat,
		bool& hasImage);

	/**
	 * Occlusionのテクスチャの種類がベイク不要の1枚のテクスチャであるかチェック.
	 * ※ OcclusionレイヤはShade3D ver.17/18段階では存在しないため COcclusionTextureShaderInterface クラスで与えている.
	 * @param[out] ppMasterImage master imageの参照を返す.
	 * @param[out] uvTexCoord    UV用の使用テクスチャ層番号を返す.
	 * @param[out] texRepeat     繰り返し回数.
	 * @param[out] hasImage      イメージを持つか (単数または複数).
	 */
	bool m_checkOcclusionSingleImage (sxsdk::master_image_class** ppMasterImage,
		int& uvTexCoord,
		sx::vec<int,2>& texRepeat,
		bool& hasImage);

	/**
	 * 法線マップの強さを取得.
	 */
	float m_getNormalWeight ();

	/**
	 * 指定のマッピングの種類でのテクスチャサイズの最大を取得.
	 * 異なるサイズのテクスチャが混在する場合、一番大きいサイズのテクスチャに合わせる.
	 * @param[in]  mappingType   マッピングの種類.
	 */
	sx::vec<int,2> m_getMaxMappingImageSize (const sxsdk::enums::mapping_type mappingType);

	/**
	 * マッピングレイヤで「マット」を持つか.
	 * @param[in]  mappingType   マッピングの種類.
	 */
	 bool m_hasWeightTexture (const sxsdk::enums::mapping_type mappingType);

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
	 compointer<sxsdk::image_interface> m_duplicateImage (sxsdk::image_interface* image, const sx::vec<int,2>& dstSize,
		 const bool flipColor = false, const bool flipH = false, const bool flipV = false, const bool rotate90 = false,
		 const int repeatU = 1, const int repeatV = 1);

	/**
	 * diffuse/roughness/metallicのテクスチャを、Shade3Dの状態からPBRマテリアルに変換.
	 */
	void m_convShade3DToPBRMaterial ();

public:
	CImagesBlend (sxsdk::scene_interface* scene, sxsdk::surface_class* surface);

	/**
	 * 個々のイメージを合成.
	 */
	void blendImages ();

	/**
	 * 各種イメージを持つか (単一または複数).
	 */
	bool hasImage (const sxsdk::enums::mapping_type mappingType) const;

	/**
	 * 単一テクスチャを参照する場合のマスターイメージクラスを取得.
	 */
	sxsdk::master_image_class* getSingleMasterImage (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージを取得.
	 */
	compointer<sxsdk::image_interface> getImage (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージのUV層番号を取得.
	 */
	int getTexCoord (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージの反復回数を取得.
	 */
	sx::vec<int,2> getImageRepeat (const sxsdk::enums::mapping_type mappingType);

	/**
	 * アルファ透明を使用しているか.
	 */
	bool getDiffuseAlphaTrans () { return m_diffuseAlphaTrans; }

	/**
	 * Occlusionのウエイト値を取得.
	 */
	float getOcclusionWeight () { return m_occlusionWeight; }

	/**
	 * Occlusionのマスターイメージを取得.
	 */
	sxsdk::master_image_class* getOcclusionMasterImage () { return m_occlusionMasterImage; }

	/**
	 * OcclusionのUV層番号を取得.
	 */
	int getOcclusionTexCoord () { return m_occlusionTexCoord; }

	/**
	 * Diffuse色を取得.
	 */
	sxsdk::rgb_class getDiffuseColor () const { return m_diffuseColor; }

	/**
	 * Emissive色を取得.
	 */
	sxsdk::rgb_class getEmissiveColor () const { return m_emissiveColor; }

	/**
	 * Metallic値を取得.
	 */
	float getMetallic () const { return m_metallic; }

	/**
	 * Roughness値を取得.
	 */
	float getRoughness () const { return m_roughness; }

};

#endif
