/**
 * USD格納時に使用する情報.
 * これは、Shade3DやUSDに依存しない情報.
 */

#ifndef _USDDATA_H
#define _USDDATA_H

#include <vector>
#include <string>

namespace USD_DATA {
	/**
	 * ノードの種類.
	 */
	enum NODE_TYPE {
		null_node = 0,		// NULLノード (パート).
		mesh_node,			// Mesh.
		bone_node,			// Bone (joint).
		material_node,		// Material.
		texture_node,		// Texture.
	};

	/**
	 * テクスチャでの採用要素.
	 * roughness/metallicなどでは、R/G/Bのいずれかを採用することになる.
	 */
	enum TEXTURE_SOURE {
		texture_source_rgb = 0,		// RGB.
		texture_source_r,			// R.
		texture_source_g,			// G.
		texture_source_b,			// B.
		texture_source_a,			// A.
	};

	/**
	 * テクスチャの種類.
	 */
	enum TEXTURE_PATTERN_TYPE {
		texture_pattern_type_difuseColor = 0,	// diffuse color.
		texture_pattern_type_emissiveColor,		// emissive color.
		texture_pattern_type_normal,			// normal.
		texture_pattern_type_roughness,			// roughness.
		texture_pattern_type_metallic,			// metallic.
		texture_pattern_type_opacity,			// opacity.
		texture_pattern_type_occlusion,			// occlusion.
	};

	/**
	 * メッシュの情報.
	 * これは、USDにエクスポートする際に使用する作業情報.
	 */
	class MeshData
	{
	public:
		std::vector<float> vertices;		// 頂点座標を格納 (XYZで1要素).
		std::vector<float> normals;			// 頂点ごとの法線 (XYZで1要素).
		std::vector<float> color0;			// 頂点ごとのColor0 (XYZで1要素).

		std::vector<int> faceVertexCounts;	// 面ごとの頂点数.
		std::vector<int> faceIndices;		// 面の頂点インデックス.
		std::vector<float> faceUV0;			// 面ごとのUV0 (XYで1要素).
		std::vector<float> faceUV1;			// 面ごとのUV1 (XYで1要素).

		std::vector< std::vector<float> > skinWeights;				// 頂点ごとのスキン時のウエイト (最大4つ分).
		std::vector< std::vector<int> > skinJoints;					// 頂点ごとのスキン時に参照するジョイントインデックスリスト (最大4つ分).
		std::vector< std::vector<void *> > skinJointsHandle;		// 頂点ごとのShade3Dでのshape_classのジョイントのハンドル (最大4つ分).
		int skinSkeletonIndex;										// スキンのスケルトン番号 (作業用).

		std::string refMaterialName;		// 参照するマテリアル名 (パス).
		int materialIndex;					// マテリアル番号.
		bool subdivision;					// Subdivision処理を行う.

	public:
		MeshData ();

		void clear ();
	};

	/**
	 * ノードの変換行列要素.
	 */
	class NodeMatrixData {
	public:
		float translate[3];		// 移動.
		float rotate[3];		// 回転 (角度指定).
		float scale[3];			// スケール.

	public:
		NodeMatrixData ();
		~NodeMatrixData ();

		void clear();
	};

	/**
	 * テクスチャの要素から文字列を取得.
	 */
	const std::string getTextureSourceString (const USD_DATA::TEXTURE_SOURE& texSource);

	/**
	 * 指定の数値がゼロかチェック.
	 */
	bool isZero (const float v, const float fMin = (float)(1e-3));
	bool isZero (const float v1, const float v2, const float v3, const float fMin = (float)(1e-3));

	/**
	 * 4x4行列に単位行列を指定.
	 * @param[out] matrix 4x4行列の格納用.
	 */
	void setMatrix4x4_Identity (std::vector<float>& matrix);

	/**
	 * 4x4行列を指定.
	 * @param[in]  pM     16個のfloat配列.
	 * @param[out] matrix 4x4行列の格納用.
	 */
	void setMatrix4x4 (const float* pM, std::vector<float>& matrix);

}

#endif
