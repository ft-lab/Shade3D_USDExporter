/**
 * Export用にShade3Dの表面材質のマッピングレイヤのテクスチャを合成.
 */
#ifndef _IMAGESBLEND_H
#define _IMAGESBLEND_H

#include "GlobalHeader.h"
#include "ExportParam.h"
#include "usddata.h"

class CImagesBlend
{
public:
	// ベイク時のエラー.
	enum IMAGE_BAKE_RESULT {
		bake_success = 0,				// ベイク成功.
		bake_error_mixed_uv_layer,		// UVレイヤが混在.
		bake_mixed_repeat,				// 繰り返し回数が混在しているのでまとめた.
	};

private:
	sxsdk::scene_interface* m_pScene;
	sxsdk::surface_class* m_surface;
	CExportParam m_exportParam;							// Export時のパラメータ.

	sxsdk::image_interface* m_diffuseImage;				// Diffuseの画像.
	sxsdk::image_interface* m_normalImage;				// Normalの画像.
	sxsdk::image_interface* m_reflectionImage;			// Reflectionの画像.
	sxsdk::image_interface* m_roughnessImage;			// Roughnessの画像.
	sxsdk::image_interface* m_glowImage;				// Glowの画像.
	sxsdk::image_interface* m_occlusionImage;			// Occlusionの画像.
	sxsdk::image_interface* m_transparencyImage;		// Transparencyの画像.
	sxsdk::image_interface* m_opacityMaskImage;			// 不透明マスク相当の画像.

	bool m_hasDiffuseImage;										// diffuseのイメージを持つか.
	bool m_hasReflectionImage;									// reflectionのイメージを持つか.
	bool m_hasRoughnessImage;									// roughnessのイメージを持つか.
	bool m_hasNormalImage;										// normalのイメージを持つか.
	bool m_hasGlowImage;										// glowのイメージを持つか.
	bool m_hasOcclusionImage;									// Occlusionのイメージを持つか.
	bool m_hasTransparencyImage;								// transparencyのイメージを持つか.
	bool m_hasOpacityMaskImage;									// 不透明マスクのイメージを持つか.

	sx::vec<int,2> m_diffuseRepeat;								// Diffuseの反復回数.
	sx::vec<int,2> m_normalRepeat;								// Normalの反復回数.
	sx::vec<int,2> m_reflectionRepeat;							// Reflectionの反復回数.
	sx::vec<int,2> m_roughnessRepeat;							// Roughnessの反復回数.
	sx::vec<int,2> m_glowRepeat;								// Glowの反復回数.
	sx::vec<int,2> m_occlusionRepeat;							// Occlusionの反復回数.
	sx::vec<int,2> m_transparencyRepeat;						// Transparencyの反復回数.
	sx::vec<int,2> m_opacityMaskRepeat;							// OpacityMaskの反復回数.

	int m_diffuseTexCoord;										// DiffuseのUVレイヤ番号.
	int m_normalTexCoord;										// NormalのUVレイヤ番号.
	int m_reflectionTexCoord;									// ReflectionのUVレイヤ番号.
	int m_roughnessTexCoord;									// RoughnessのUVレイヤ番号.
	int m_glowTexCoord;											// GlowのUVレイヤ番号.
	int m_occlusionTexCoord;									// OcclusionのUVレイヤ番号.
	int m_transparencyTexCoord;									// TransparencyのUVレイヤ番号.
	int m_opacityMaskTexCoord;									// OpacityMaskのUVレイヤ番号.

	bool m_diffuseAlphaTrans;									// Diffuseのアルファ透明を使用しているか.

	int m_diffuseTexturesCount;									// Diffuseテクスチャ数.
	bool m_useDiffuseAlpha;										// Diffuseの「アルファ透明」を使用しているか.
	int m_normalTexturesCount;									// Normalテクスチャ数.

	// テクスチャを使用しない場合のパラメータ.
	sxsdk::rgb_class m_diffuseColor;							// Diffuse色.
	sxsdk::rgb_class m_emissiveColor;							// Emmisive色.
	sxsdk::rgb_class m_transparencyColor;						// 透明度の色.

	float m_metallic;											// Metallic値.
	float m_roughness;											// Roughness値.
	float m_transparency;										// 透明度.

	float m_normalStrength;										// 法線マップの強さ。MDLに出力する場合は、法線マップと強さを分離して扱える.

private:
	/**
	 * 指定のテクスチャの合成処理.
	 * @param[in] mappingType  マッピングの種類.
	 * @param[in] repeatTex    繰り返し回数.
	 */
	bool m_blendImages (const sxsdk::enums::mapping_type mappingType, const sx::vec<int,2>& repeatTex);

	/**
	 * 指定のテクスチャの種類がベイク不要の1枚のテクスチャであるかチェック.
	 * @param[in]  mappingType   マッピングの種類.
	 * @param[out] uvTexCoord    UV用の使用テクスチャ層番号を返す.
	 * @param[out] texRepeat     繰り返し回数.
	 * @param[out] hasImage      イメージを持つか (単数または複数).
	 */
	bool m_checkImage (const sxsdk::enums::mapping_type mappingType,
		int& uvTexCoord,
		sx::vec<int,2>& texRepeat,
		bool& hasImage);

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
	 * 「不透明」と「透明」のテクスチャを分離もしくは合成して再格納.
	 * @return 不透明度のテクスチャを格納.
	 */
	sxsdk::image_interface* m_storeOpasicyTransparencyTexture ();

	/**
	 * diffuse/roughness/metallicのテクスチャを、Shade3Dの状態からPBRマテリアルに変換.
	 */
	void m_convShade3DToPBRMaterial ();

	/**
	 * 加工せずにそのままPBRマテリアルとして格納.
	 */
	void m_noBakeShade3DToPBRMaterial ();

	/**
	 * 1つのテクスチャイメージを持ち、かつ乗算合成かチェック.
	 * @param[in]  mappingType    マッピングの種類.
	 * @param[put] mWeight        乗算合成の場合の適用率.
	 * @return 単一テクスチャで乗算合成の場合はtrueを返す.
	 */
	bool m_singleTextureAndMulti (const sxsdk::enums::mapping_type mappingType, float& mWeight);

public:
	CImagesBlend (sxsdk::scene_interface* scene, sxsdk::surface_class* surface);

	void clear ();

	/**
	 * 個々のイメージを合成.
	 * @return ベイクの結果.
	 */
	IMAGE_BAKE_RESULT blendImages (const CExportParam& exportParam);

	/**
	 * 各種イメージを持つか (単一または複数).
	 */
	bool hasImage (const sxsdk::enums::mapping_type mappingType) const;

	/**
	 * イメージを取得.
	 */
	sxsdk::image_interface* getImage (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージのUV層番号を取得.
	 */
	int getTexCoord (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージの反復回数を取得.
	 */
	sx::vec<int,2> getImageRepeat (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージの強度を色として取得.
	 */
	sxsdk::rgb_class getImageFactor (const sxsdk::enums::mapping_type mappingType);

	/**
	 * アルファ透明を使用しているか.
	 */
	bool getDiffuseAlphaTrans () { return m_diffuseAlphaTrans; }

	/**
	 * 指定のマッピングの種類でのイメージフォーマットの種類を取得.
	 */
	USD_DATA::IMAGE_FORMAT_TYPE getImageFormatType (const sxsdk::enums::mapping_type mappingType);

	/**
	 * 透明度の強さを取得.
	 */
	float getTransparency () const { return m_transparency; }

	/**
	 * 法線マップの強さを取得.
	 */
	float getNormalStrength () const { return m_normalStrength; } 
};

#endif
