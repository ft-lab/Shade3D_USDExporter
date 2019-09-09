/**
 * 一時的なシーン情報を保持.
 */
#ifndef _SCENEDATA_H
#define _SCENEDATA_H

#include "GlobalHeader.h"
#include "FindNames.h"
#include "MeshData.h"
#include "MaterialData.h"
#include "ImageData.h"
#include "USDData.h"
#include "ExportParam.h"
#include "AnimationData.h"
#include "USDExporter.h"
#include "TextureTransform.h"

#include <string>
#include <vector>
#include <memory>

class CSceneData
{
private:
	CFindNames m_findNames;		// USDでの同一パス名検索クラス.
	CFindNames m_findImageFileNames;	// 画像ファイル名が同じにならないようにするクラス.

	/**
	 * 格納情報 (形状、マテリアル、画像).
	 */
	class CElementData {
	public:
		std::string orgName;		// オリジナルの形状名.
		std::string storeName;		// USDとして格納する際の形状名 (/root/objects/xxxx の絶対パス形式).

		sxsdk::shape_class* shape;	// 形状の参照.

	public:
		CElementData () {
			orgName   = "";
			storeName = "";
			shape     = NULL;
		}
		~CElementData () {
		}

		CElementData (const CElementData& v) {
			this->orgName   = v.orgName;
			this->storeName = v.storeName;
			this->shape     = v.shape;
		}

	    CElementData& operator = (const CElementData &v) {
			this->orgName   = v.orgName;
			this->storeName = v.storeName;
			this->shape     = v.shape;

			return (*this);
		}
	};

private:
	sxsdk::scene_interface* m_pScene;			// カレントのシーンクラス.
	CExportParam m_exportParam;					// エクスポート時のパラメータ.

	std::vector<CElementData> m_elementsList;	// 要素情報を格納する.

	std::vector<std::string> m_exportFilesList;		// 出力したファイルのフルパスを保持。usdz出力時に使用する.

	std::vector<CSkeletonData> m_skeletonList;		// アニメーション用のスケルトンリスト.

public:
	std::string filePath;					// 保存ファイルパス.
	CTempMeshData tmpMeshData;				// メッシュ情報の一時格納用.

	std::vector< std::shared_ptr<CNodeBaseData> > nodesList;			// ノード情報を格納.
	std::vector<CMaterialData> materialsList;							// マテリアルを格納.
	std::vector<CImageData> imagesList;									// テクスチャイメージを格納.

private:
	/**
	 * 同一のマスターイメージがすでにimagesListに格納済みか.
	 * @param[in]  masterImage     追加するMasterImage.
	 * @param[in]  texTransform    マッピングの変換情報.
	 * @param[in]  channelMix      mapping_layerのchanelMixの指定.
	 */
	int m_findMasterImageInImagesList (sxsdk::master_image_class* masterImage, const CTextureTransform& texTransform, const int channelMix);

	/**
	 * テクスチャイメージの出力.
	 * filePathのフォルダにテクスチャを出力する.
	 * @param[in]  filePath  出力ファイルパス（xxxx.usd などのファイル名も付加される）.
	 */
	void m_exportTextures (const std::string& filePath);

	/**
	 * テクスチャマッピング情報を追加.
	 * @param[in]  mappingLayer    マッピングレイヤ情報.
	 * @param[in]  texTransform    マッピングの変換情報.
	 * @param[out] texMappingData  マッピング情報の格納先.
	 */
	void m_setTextureMappingData (sxsdk::mapping_layer_class& mappingLayer, const CTextureTransform& texTransform, CTextureMappingData& texMappingData, const bool occlusionF = false);

	/**
	 * Shade3Dの変換行列から USD_DATA::NodeMatrixData に変換.
	 */
	USD_DATA::NodeMatrixData m_convMatrix (const sxsdk::mat4& m);

	/**
	 * 指定のマスターサーフェスから、表面材質情報を取得.
	 * @param[in]  masterSurface  対象のマスターサーフェス.
	 * @param[in]  surface        masterSurfaceがnullの場合はこのsurfaceを参照する.
	 * @param[out] materialDat   マテリアル情報が返る.
	 * @return マテリアル情報を取得した場合はtrue.
	 */
	bool m_getMaterialFromMasterSurface (sxsdk::master_surface_class* masterSurface, sxsdk::surface_class* surface, CMaterialData& materialDat);

	/**
	 * Skeletonとjoint構造を出力.
	 */
	void m_exportSkeletonAndJoints (CUSDExporter& usdExport);

	/**
	 * アニメーション情報を取得.
	 */
	CAnimationData m_getAnimationData (sxsdk::scene_interface* scene);

	/**
	 * ボーンのジョイントモーション情報を取得.
	 */
	void m_getJointMotionData (sxsdk::shape_class* shape, CNodeNullData& nodeD);

	/**
	 * スケルトン情報(m_skeletonList)から、メッシュ情報での参照(ジョイントインデックス)を取得.
	 */
	void m_setMeshSkeletonRef (const CNodeMeshData& nodeMeshData, USD_DATA::MeshData& meshData);

public:
	CSceneData ();
	~CSceneData ();

	void clear ();

	/**
	 * エクスポートパラメータを指定.
	 */
	void setExportParam (const CExportParam& exportParam);

	/**
	 * シーンクラスを受け取る.
	 */
	void setSceneInterface (sxsdk::scene_interface* scene);

	/**
	 * 指定の形状を格納する.
	 * @param[in] shape  形状の参照.
	 * @return USDで格納する際の形状パス.
	 */
	std::string appendShape (sxsdk::shape_class* shape);

	/**
	 * 指定の形状に対応するUSDでのパスを取得.
	 * @param[in] shape  形状の参照.
	 * @return USDで格納する際の形状パス.
	 */
	std::string getShapePath (sxsdk::shape_class* shape);

	/**
	 * 指定の形状がメッシュに変換できるか調べる.
	 */
	bool checkConvertMesh (sxsdk::shape_class* shape);

	/**
	 * パートをNULLノードとして格納.
	 * @param[in] shape     対象の形状クラス.
	 * @param[in] namePath  形状パス (/root/objects/xxx のような形式).
	 * @param[in] matrix    変換行列.
	 * @param[in] isBone    ボーンの場合はtrue.
	 */
	void appendNodeNull (sxsdk::shape_class* shape, const std::string& namePath, const sxsdk::mat4& matrix, const bool isBone = false);

	/**
	 * Meshを追加.
	 * Mesh追加時に、マテリアルがある場合はそれもmaterialsListに追加する.
	 * @param[in] shape     対象形状.
	 * @param[in] namePath  形状パス (/root/objects/xxx のような形式).
	 * @param[in] matrix    変換行列.
	 * @param[in] _tempMeshData  メッシュ情報.
	 */
	void appendNodeMesh (sxsdk::shape_class* shape, const std::string& namePath, const sxsdk::mat4& matrix, const CTempMeshData& _tempMeshData);

	/**
	 * USDファイルを出力.
	 * @param[in] shade        shade_interface
	 * @param[in] filePath     出力ファイル名（絶対パス）.
	 */
	void exportUSD (sxsdk::shade_interface& shade, const std::string& filePath);

	/**
	 * usdzファイルを出力。exportUSDのあとに実行すること.
	 */
	void exportUSDZ (const std::string& filePath);

	/**
	 * 指定の形状から、表面材質情報を取得.
	 * なお、その形状が表面材質を持たない場合は親をたどる.
	 * @param[in]  shape         対象形状.
	 * @param[out] materialDat   マテリアル情報が返る.
	 * @return マテリアル情報を取得した場合はtrue.
	 */
	bool getMaterialFromShape (sxsdk::shape_class* shape, CMaterialData& materialDat);
};

#endif
