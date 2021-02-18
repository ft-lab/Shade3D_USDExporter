/**
 * GLTF用のメッシュデータ.
 */
#ifndef _MESHDATA_H
#define _MESHDATA_H

#include "GlobalHeader.h"
#include "NodeData.h"
#include "USDData.h"

#include <vector>
#include <string>

//---------------------------------------------------------------.
/**
 * 1つのメッシュ情報 (Shade3Dでの構成).
 */
class CTempMeshData
{
public:
	std::string name;						// 形状名.

	std::vector<sxsdk::vec3> vertices;					// 頂点座標.
	std::vector<sxsdk::vec4> skinWeights;				// 頂点ごとのスキンのウエイト値.
	std::vector< sx::vec<int,4> > skinJoints;			// 頂点ごとのスキンのジョイントインデックス値 (Import時に使用).
	std::vector< sx::vec<void *,4> > skinJointsHandle;	// 頂点ごとのスキンのジョイントのハンドル (Export時に使用).

	std::vector<int> faceVertexCounts;					// 面ごとの頂点数.

	std::vector<int> faceIndices;						// 面の頂点インデックス.
	std::vector<int> faceFaceGroupIndex;				// 面ごとのフェイスグループ番号.
	std::vector<sxsdk::vec3> faceNormals;				// 面ごとの法線.
	std::vector<sxsdk::vec2> faceUV0;					// 面ごとのUV0.
	std::vector<sxsdk::vec2> faceUV1;					// 面ごとのUV1.
	std::vector<sxsdk::vec4> faceColor0;				// 面ごとの頂点カラー0.

	int materialIndex;							// 対応するマテリアル番号.
	std::vector<sxsdk::master_surface_class *> faceGroupMasterSurfaces;	// フェイスグループごとのマスターサーフェスの参照.
	std::vector<int> faceGroupFacesCount;								// フェイスグループごとの保有面数.

	bool flipFaces;								// 面反転フラグ.
	bool subdivision;							// Subdivision処理を行う.

public:
	CTempMeshData ();
	CTempMeshData (const CTempMeshData& v);
	~CTempMeshData ();

    CTempMeshData& operator = (const CTempMeshData &v) {
		this->name            = v.name;
		this->vertices        = v.vertices;
		this->skinWeights     = v.skinWeights;
		this->skinJoints      = v.skinJoints;
		this->skinJointsHandle = v.skinJointsHandle;
		this->faceVertexCounts = v.faceVertexCounts;
		this->faceIndices         = v.faceIndices;
		this->faceFaceGroupIndex  = v.faceFaceGroupIndex;
		this->faceGroupMasterSurfaces = v.faceGroupMasterSurfaces;
		this->faceGroupFacesCount = v.faceGroupFacesCount;
		this->faceNormals         = v.faceNormals;
		this->faceUV0             = v.faceUV0;
		this->faceUV1             = v.faceUV1;
		this->faceColor0          = v.faceColor0;

		this->materialIndex   = v.materialIndex;
		this->flipFaces = v.flipFaces;
		this->subdivision = v.subdivision;

		return (*this);
    }

	void clear ();

	/**
	 * 最適化 (不要頂点の除去など).
	 * @param[in]  removeUnusedVertices   未使用頂点を削除する場合はtrue.
	 */
	void optimize (const bool removeUnusedVertices = true);
};

//---------------------------------------------------------------.
/**
 * 1つのメッシュ情報 .
 */
class CNodeMeshData : public CNodeBaseData
{
public:
	void *shapeHandle;			// sxsdk::shape_classのハンドル.

	// vertices/normals/uv0/uv1/skinWeights/skinJoints の要素数は同じ。.
	std::vector<sxsdk::vec3> vertices;		// 頂点座標.
	std::vector<sxsdk::vec3> normals;		// 頂点ごとの法線.
	std::vector<sxsdk::vec2> faceUV0;		// 面ごとのUV0.
	std::vector<sxsdk::vec2> faceUV1;		// 面ごとのUV1.
	std::vector<sxsdk::vec4> color0;		// 頂点ごとのColor0.
	std::vector<sxsdk::vec4> skinWeights;		// 頂点ごとのスキン時のウエイト (最大4つ分).
	std::vector< sx::vec<int,4> > skinJoints;	// 頂点ごとのスキン時に参照するジョイントインデックスリスト (最大4つ分).
	std::vector< sx::vec<void *,4> > skinJointsHandle;	// 頂点ごとのスキンのジョイントのハンドル (Export時に使用).

	std::vector<int> faceVertexCounts;			// 面ごとの頂点数.
	std::vector<int> faceIndices;				// 面の頂点インデックス.

	int materialIndex;						// 対応するマテリアル番号.
	void *masterSurfaceHangle;				// マテリアルのマスターサーフェスのハンドル (フェイスグループ時に使用).

	std::string refMaterialName;			// 参照するマテリアル名 (パス).
	bool subdivision;							// Subdivision処理を行う.
	bool faceGroupMesh;							// face groupのMeshの場合.

public:
	CNodeMeshData ();
	CNodeMeshData (const CNodeMeshData& v);
	virtual ~CNodeMeshData ();

    CNodeMeshData& operator = (const CNodeMeshData &v) {
		this->name     = v.name;
		this->matrix   = v.matrix;
		this->nodeType = v.nodeType;

		this->shapeHandle     = v.shapeHandle;
		this->vertices        = v.vertices;
		this->normals         = v.normals;
		this->faceUV0         = v.faceUV0;
		this->faceUV1         = v.faceUV1;
		this->color0          = v.color0;
		this->faceVertexCounts = v.faceVertexCounts;
		this->faceIndices      = v.faceIndices;
		this->materialIndex    = v.materialIndex;
		this->skinWeights      = v.skinWeights;
		this->skinJoints       = v.skinJoints;
		this->skinJointsHandle = v.skinJointsHandle;

		this->refMaterialName = v.refMaterialName;
		this->masterSurfaceHangle = v.masterSurfaceHangle;
		this->subdivision = v.subdivision;
		this->faceGroupMesh = v.faceGroupMesh;

		return (*this);
    }

	void clear ();

	/**
	 * CTempMeshDataから複数のフェイスグループを考慮して、コンバート.
	 */
	static int convert (const CTempMeshData& tempMeshData, std::vector<CNodeMeshData>& meshes);

	/**
	 * 頂点座標のバウンディングボックスを計算.
	 */
	void calcBoundingBox (sxsdk::vec3& bbMin, sxsdk::vec3& bbMax) const;

	/**
	 * 頂点カラー情報で、Alphaを出力する必要があるか.
	 */
	bool hasNeedVertexColorAlpha () const;

	/**
	 * USD_DATA::MeshDataにコンバート.
	 * @param[out] usdMeshData  USDのメッシュデータとしての格納先.
	 */
	void convertTo (USD_DATA::MeshData& usdMeshData);
};

#endif
