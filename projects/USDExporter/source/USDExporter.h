﻿/**
 * USD出力機能.
 */
#ifndef _USDEXPORTER_H
#define _USDEXPORTER_H

#include "USDData.h"
#include "MaterialData.h"
#include "SkeletonData.h"

#include <string>
#include <vector>

class CUSDExporter {
private:
	std::vector<std::string> m_pathStack;			// パスの格納スタック.
	std::string m_currentPath;						// カレントの形状パス.
	std::vector<CSkeletonData> m_skeletonsList;		// アニメーション時のスケルトン情報.

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
	 * テクスチャ情報を出力.
	 * @param[in] materialData  マテリアル情報クラス.
	 * @param[in] patternType   テクスチャの種類.
	 * @param[in] textureSource テクスチャの参照要素.
	 */
	void m_outputTextureData (const CMaterialData& materialData, const USD_DATA::TEXTURE_PATTERN_TYPE& patternType, const USD_DATA::TEXTURE_SOURE& textureSource);

	/**
	 * 指定のメッシュが、スケルトンのどのルートに属するか.
	 */
	int m_findSkeletonRootIndex (const USD_DATA::MeshData& meshData);

	/**
	 * 指定のメッシュがスキンを持つか.
	 */
	bool m_hasSkinMesh (const USD_DATA::MeshData& meshData);

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
	 * スケルトン情報を渡す.
	 */
	void setSkeletonsData (const std::vector<CSkeletonData>& skelData);

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
	 * NULLノードを出力.
	 * @param[in] nodeName  ノード名 (/root/xxx/mesh1 などのパス形式).
	 * @param[in] matrix    変換要素.
	 */
	void appendNodeNull (const std::string& nodeName, const USD_DATA::NodeMatrixData& matrix);

	/**
	 * Meshノードを出力.
	 * @param[in] nodeName    ノード名 (/root/xxx/mesh1 などのパス形式).
	 * @param[in] matrix      変換要素.
	 * @param[in] meshData    メッシュ情報.
	 * @param[in] doubleSided 両面表示するか.
	 */
	void appendNodeMesh (const std::string& nodeName, const USD_DATA::NodeMatrixData& matrix, const USD_DATA::MeshData& meshData, const bool doubleSided);

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
};

#endif
