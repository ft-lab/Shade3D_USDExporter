/**
 * Shade3Dの表面材質より、テクスチャをPBR向けにベイク.
 * このクラス内に他とかぶらないようにしたテクスチャイメージのリストを持つ.
 */

#ifndef _MATERIALTEXTUREBAKE_H
#define _MATERIALTEXTUREBAKE_H

#include "GlobalHeader.h"
#include "USDData.h"
#include "MaterialData.h"
#include "TextureTransform.h"
#include "FindNames.h"
#include "ImageData.h"
#include "ExportParam.h"

#include <string>
#include <vector>

//------------------------------------------------------------------.
/**
 * テクスチャパラメータ.
 */
class CMaterialTextureBake
{
private:
	sxsdk::scene_interface* m_pScene;			// Shade3Dのシーンクラス.
	CExportParam m_exportParam;					// エクスポート時のパラメータ.
	CFindNames m_findImageFileNames;			// 画像ファイル名が同じにならないようにするクラス.
	std::vector<CImageData> m_imagesList;		// テクスチャイメージを格納.

private:
	/**
	 * 指定の形状でマテリアルを取得.
	 * @param[in]  masterSurface     マスターサーフェスクラス.
	 * @param[in]  surface           表面材質クラス.
	 * @param[out] materialData  マテリアル情報が返る.
	 */
	bool m_getMaterialDataFromShape (sxsdk::master_surface_class* masterSurface, sxsdk::surface_class* surface, CMaterialData& materialData);

	/**
	 * マッピングレイヤで、単純に1テクスチャで構成.
	 * @param[in]  surface           表面材質クラス.
	 * @param[out] materialData  マテリアル情報が返る.
	 */
	bool m_getSimpleMaterialMappingFromSurface (sxsdk::surface_class* surface, CMaterialData& materialData);

	/**
	 * マッピングレイヤで、複数テクスチャをベイク.
	 * @param[in]  surface           表面材質クラス.
	 * @param[out] materialData  マテリアル情報が返る.
	 */
	bool m_getMaterialMultiMappingFromSurface (sxsdk::surface_class* surface, CMaterialData& materialData);

	/**
	 * テクスチャマッピング情報を追加.
	 * @param[in]  mappingLayer    マッピングレイヤ情報.
	 * @param[in]  texTransform    マッピングの変換情報.
	 * @param[out] texMappingData  マッピング情報の格納先.
	 * @param[in]  occlusionF           Occlusionのテクスチャか.
	 * @param[in]  useTransparentAlpha  アルファ透明を使用するか.
	 */
	void m_setTextureMappingData (sxsdk::mapping_layer_class& mappingLayer, const CTextureTransform& texTransform, CTextureMappingData& texMappingData, const bool occlusionF = false, const bool useTransparentAlpha = false);

	/**
	 * アルファ透明使用時の、テクスチャマッピング情報を追加.
	 * @param[in]  mappingLayer    マッピングレイヤ情報.
	 * @param[in]  texTransform    マッピングの変換情報.
	 * @param[out] texMappingData  マッピング情報の格納先.
	 */
	void m_setTextureMappingOpacityData (sxsdk::mapping_layer_class& diffuseMappingLayer, const CTextureTransform& texTransform, CTextureMappingData& texMappingData);

	/**
	 * 同一のマスターイメージがすでにimagesListに格納済みか.
	 * @param[in]  masterImage     追加するMasterImage.
	 * @param[in]  texTransform    マッピングの変換情報.
	 * @param[in]  channelMix      mapping_layerのchanelMixの指定.
	 */
	int m_findMasterImageInImagesList (sxsdk::master_image_class* masterImage, const CTextureTransform& texTransform, const int channelMix);

public:
	CMaterialTextureBake (sxsdk::scene_interface* scene, const CExportParam& exportParam);
	~CMaterialTextureBake ();

	void clear ();

	/**
	 * テクスチャイメージリストを取得.
	 */
	const std::vector<CImageData>& getImagesList () const { return m_imagesList; }

	/**
	 * 指定の形状でマテリアルを取得.
	 * @param[in]  shape         対象の形状.
	 * @param[out] materialData  マテリアル情報が返る.
	 */
	bool getMaterialDataFromShape (sxsdk::shape_class* shape, CMaterialData& materialData);

	/**
	 * 指定のマスターサーフェスから、表面材質情報を取得.
	 * @param[in]  masterSurface  対象のマスターサーフェス.
	 * @param[in]  surface        masterSurfaceがnullの場合はこのsurfaceを参照する.
	 * @param[out] materialDat   マテリアル情報が返る.
	 * @return マテリアル情報を取得した場合はtrue.
	 */
	bool getMaterialFromMasterSurface (sxsdk::master_surface_class* masterSurface, sxsdk::surface_class* surface, CMaterialData& materialData);

};

#endif
