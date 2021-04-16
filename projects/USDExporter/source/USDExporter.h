/**
 * USD出力機能.
 */
#ifndef _USDEXPORTER_H
#define _USDEXPORTER_H

#include "USDData.h"
#include "MaterialData.h"
#include "SkeletonData.h"
#include "JointMotionData.h"
#include "ImageData.h"

#include <string>
#include <vector>

class CUSDExporter {
private:
	std::vector<std::string> m_pathStack;			// パスの格納スタック.
	std::string m_currentPath;						// カレントの形状パス.
	std::vector<CSkeletonData> m_skeletonsList;		// アニメーション時のスケルトン情報.
	std::vector<CImageData> m_imagesList;			// 出力するイメージ情報のリスト.

	std::string m_versionString;					// プラグインバージョンの文字列.

public:
	CUSDExporter ();

	void clear ();

public:
	//-----------------------------------------------------------.
	// 以下、テスト用.
	//-----------------------------------------------------------.
	/**
	 * 球を作成するだけのサンプル.
	 */
	void testCreateSphere (const std::string& fileName);

	/**
	 * NURBSを配置するサンプル.
	 */
	void testCreateNURBSCurves (const std::string& fileName);

	/**
	 * ノードや形状の位置、回転、スケール指定のサンプル.
	 */
	void testCreateSphere_xformOp (const std::string& fileName);

	/**
	 * メッシュを作成するサンプル.
	 */
	void testCreateMesh (const std::string& fileName);

	/**
	 * 球を作成し、回転アニメーションさせるサンプル.
	 */
	void testCreateSphereWithAnimation (const std::string& fileName);

private:
	/**
	 * ヘッダ情報を出力.
	 */
	void m_outputHeaders ();

	/**
	 * テクスチャ情報を出力.
	 * @param[in] pathStr        USD上のパス (/root/xxx/red).
	 * @param[in] materialData  マテリアル情報クラス.
	 * @param[in] patternType   テクスチャの種類.
	 * @param[in] textureSource テクスチャの参照要素.
	 */
	void m_outputTextureData (const std::string& pathStr, const CMaterialData& materialData, const USD_DATA::TEXTURE_PATTERN_TYPE& patternType, const USD_DATA::TEXTURE_SOURE& textureSource);

	/**
	 * 指定のメッシュがスキンを持つか.
	 */
	bool m_hasSkinMesh (const USD_DATA::MeshData& meshData);

	/**
	 * ノードに対してモーション情報(transform animation)を格納.
	 */
	void m_setTransformAnimation (const std::string& nodeName, const CJointMotionData& jointMotion);

	/**
	 * 指定のUSDのパスにマテリアル情報を格納.
	 * @param[in] pathStr        USD上のパス (/root/xxx/red).
	 * @param[in] materialData   マテリアルデータ.
	 */
	void m_appendNodeMaterial (const std::string& pathStr, const CMaterialData& materialData);

	/**
	 * 指定のUSDのパスにマテリアル情報を格納 (OmniverseのMDL用).
	 * @param[in] pathStr        USD上のパス (/root/xxx/red).
	 * @param[in] materialData   マテリアルデータ.
	 */
	void m_appendNodeMaterial_OmniverseMDL (const std::string& pathStr, const CMaterialData& materialData);

public:
	//-----------------------------------------------------------.
	// USDエクスポート用.
	//-----------------------------------------------------------.
	/**
	 * Export開始.
	 */
	void beginExport (const std::string& fileName);

	/**
	 * Export終了.
	 */
	void endExport ();

	/**
	 * バージョン文字列を渡す.
	 * @param[in] verStr  バージョン文字列。 "0.0.1.2" など.
	 */
	void setVersionString (const std::string& verStr);

	/**
	 * スケルトン情報を渡す.
	 */
	void setSkeletonsData (const std::vector<CSkeletonData>& skelData);

	/**
	 * イメージ情報を渡す.
	 */
	void SetImagesList (const std::vector<CImageData>& imagesList);

	/**
	 * usdzファイルを出力.
	 * @param[in] filePath  出力ファイルパス.
	 * @param[in] files     出力で追加するファイル（すべて絶対パスで指定）.
	 */
	bool exportUSDZ (const std::string& filePath, std::vector<std::string>& files);

	/**
	 * Materialノードを出力.
	 * @param[in] materialData   マテリアルデータ.
	 */
	void appendNodeMaterial (const CMaterialData& materialData);

	/**
	 * OmniverseのMDLとしてMaterialノードを出力.
	 * @param[in] materialData   マテリアルデータ.
	 */
	void appendNodeMaterial_OmniverseMDL (const CMaterialData& materialData);

	/**
	 * マテリアルを複製.
	 * これはShade3Dのリンク使用時に、マスターオブジェクトのスコープ内でマテリアルを参照できるようにする.
	 * @param[in]  nodeName       対象のノードパス.
	 * @param[in]  materialsList  マテリアル情報のリスト.
	 */
	void setMaterialsInScope (const std::string& nodeName, const std::vector<CMaterialData>& materialsList);

	/**
	 * NULLノードを出力.
	 * @param[in] nodeName  ノード名 (/root/xxx/mesh1 などのパス形式).
	 * @param[in] matrix    変換要素.
	 * @param[in] jointMotion  ノードでモーション情報を持つ場合のモーション情報.
	 */
	void appendNodeNull (const std::string& nodeName, const USD_DATA::NodeMatrixData& matrix, const CJointMotionData& jointMotion);

	/**
	 * Meshノードを出力.
	 * @param[in] nodeName    ノード名 (/root/xxx/mesh1 などのパス形式).
	 * @param[in] matrix      変換要素.
	 * @param[in] meshData    メッシュ情報.
	 * @param[in] doubleSided 両面表示するか.
	 */
	void appendNodeMesh (const std::string& nodeName, const USD_DATA::NodeMatrixData& matrix, const USD_DATA::MeshData& meshData, const bool doubleSided);

	/**
	 * 主にSubdivisionの指定 (Omniverseにパラメータがある).
	 * @param[in] nodeName        ノード名 (/root/xxx/mesh1 などのパス形式).
	 * @param[in] enableOverride   refinementEnableOverrideの指定.
	 * @param[in] refinementLevel  refinementLevelの指定.
	 */
	void setRefinement (const std::string& nodeName, const bool enableOverride = false, const int refinementLevel = 0);

	/**
	 * 参照を指定.
	 * @param[in] nodeName           ノード名 (/root/xxx/mesh1 などのパス形式).
	 * @param[in] refNodeName        参照するノード名 (/root/xxx/mesh1 などのパス形式).
	 * @param[in] refMaterialName    参照するマテリアル名 (/root/materials/xxx1 などのパス形式).
	 */
	void setShapeReference (const std::string& nodeName, const std::string& refNodeName, const std::string& refMaterialName);

	/**
	 * アニメーション情報を出力.
	 * @param[in] startFrame  開始フレーム.
	 * @param[in] endFrame    終了フレーム.
	 * @param[in] framerate   フレームレート.
	 */
	void setAnimationData (const float startFrame, const float endFrame, const float framerate = 30.0f);

	/**
	 * Skeleton情報を出力.
	 */
	void appendSkeletonData (const CSkeletonData& skelData);

	/**
	 * 非表示化.
	 * @param[in] nodeName    ノード名 (/root/xxx/mesh1 などのパス形式).
	 */
	void setActive (const std::string& nodeName, const bool activeV = true);

	/**
	 * 非表示化。これはiOS 14.4で非表示となるわけではない.
	 * @param[in] nodeName    ノード名 (/root/xxx/mesh1 などのパス形式).
	 */
	void setVisible (const std::string& nodeName, const bool visible = true);

};

#endif
