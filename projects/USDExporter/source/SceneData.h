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
#include "MaterialTextureBake.h"

#include <string>
#include <vector>
#include <memory>

class CSceneData
{
private:
	CFindNames m_findNames;				// USDでの同一パス名検索クラス.

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
	std::string m_usdzFileName;						// 出力したUSDZファイル名.

	std::vector<CSkeletonData> m_skeletonList;		// アニメーション用のスケルトンリスト.

	std::unique_ptr<CMaterialTextureBake> m_materialTextureBake;		// マテリアルで使用するテクスチャベイク用.

public:
	std::string filePath;					// 保存ファイルパス.
	CTempMeshData tmpMeshData;				// メッシュ情報の一時格納用.

	std::vector< std::shared_ptr<CNodeBaseData> > nodesList;			// ノード情報を格納.
	std::vector<CMaterialData> materialsList;							// マテリアルを格納.

private:
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

	/**
	 * リンク情報を参照として取得.
	 */
	void m_getShapeRef (const int tIndex, const CNodeRefData& nodeRefData, std::vector<std::string>& orgNameList, std::vector<std::string>& orgMaterialNameList);

	/**
	 * マスターオブジェクト的な要素でパートとして参照される場合、スコープ内にマテリアルを移動させる.
	 */
	void m_setLinkMaterials (CUSDExporter& usdExport, const int tIndex, const CNodeRefData& nodeRefData);

	/**
	 * ジョイントの回転情報を、QuaternionからEulerに変換し格納.
	 */
	 void m_calcJointQuaternionToEuler (CJointMotionData& motionData);

	 /**
	  * テクスチャをエクスポートパラメータでリサイズしてファイル出力.
	  * @param[in] fileName  出力ファイル名.
	  * @param[in] image     imageクラス.
	  */
	 void m_saveTextureImage (const std::string fileName, sxsdk::image_interface* image);

	 /**
	  * スキンを持つ形状で、名前の重複がある場合は別名を付ける.
	  */
	 void m_makeUniqueName ();

	 /**
	  * 指定の表面材質を持つマテリアル番号を取得.
	  */
	 int m_findSameMaterial (sxsdk::surface_class* pSurface);
	 int m_findSameMaterial (sxsdk::master_surface_class* pMasterSurface);

public:
	CSceneData ();
	~CSceneData ();

	void clear ();

	/**
	 * エクスポート開始の情報を渡す.
	 * @param[in] scene        Shade3Dのシーンクラス.
	 * @param[in] exportParam  エクスポートパラメータ.
	 */
	void setupExport (sxsdk::scene_interface* scene, const CExportParam& exportParam);

	/**
	 * 指定の形状を格納する.
	 * @param[in] shape  形状の参照.
	 * @param[in] linkedParentShape  リンク時の親の参照.
	 * @return USDで格納する際の形状パス.
	 */
	std::string appendShape (sxsdk::shape_class* shape, sxsdk::shape_class* linkedParentShape);

	/**
	 * 指定のパスを格納する。このときにユニークな名前を取得.
	 * @param[in] nodeName    "/root/xxx/cylinder"のようなUSDでのパス.
	 * @return ユニークな形状パス.
	 */
	std::string appendUniquePath (sxsdk::shape_class* shape, const std::string& nodeName);

	/**
	 * 指定の形状に対応するUSDでのパスを取得.
	 * @param[in] shape  形状の参照.
	 * @param[in] linkedParentShape  リンク時の親の参照.
	 * @return USDで格納する際の形状パス.
	 */
	std::string getShapePath (sxsdk::shape_class* shape, sxsdk::shape_class* linkedParentShape);

	/**
	 * 同一の形状がすでに格納済みの場合は、既存のパスを取得.
	 */
	std::string searchSamePath (sxsdk::shape_class* shape);

	/**
	 * 情報として指定の形状がすでに格納済みかチェック（リンクやマスターオブジェクト格納時の最適化用）.
	 */
	bool existStoreShape (sxsdk::shape_class* shape);

	/**
	 * 指定の形状がメッシュに変換できるか調べる.
	 */
	bool checkConvertMesh (sxsdk::shape_class* shape);

	/**
	 * パートをNULLノードとして格納.
	 * @param[in] shape     対象の形状クラス.
	 * @param[in] namePath  形状パス (/root/objects/xxx のような形式).
	 * @param[in] matrix    変換行列.
	 */
	void appendNodeNull (sxsdk::shape_class* shape, const std::string& namePath, const sxsdk::mat4& matrix);

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
	 * リンクとしての参照を追加.
	 * @param[in] shape     対象形状.
	 * @param[in] namePath  形状パス (/root/objects/xxx のような形式).
	 */
	void appendNodeReference (sxsdk::shape_class* shape, const std::string& namePath);

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
	 * エクスポートしたファイル一覧を取得 (usdzも含む).
	 */
	std::vector<std::string> getExportFilesList () const;
};

#endif
