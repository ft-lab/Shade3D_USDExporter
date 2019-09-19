/**
 * GLTF用のメッシュデータ.
 */
#include "MeshData.h"
#include "MathUtil.h"

//---------------------------------------------------------------.
/**
 * 1つのメッシュ情報 (Shade3Dでの構成).
 */
CTempMeshData::CTempMeshData ()
{
	clear();
}

CTempMeshData::CTempMeshData (const CTempMeshData& v)
{
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
}

CTempMeshData::~CTempMeshData ()
{
}

void CTempMeshData::clear ()
{
	name = "";
	vertices.clear();
	skinWeights.clear();
	skinJoints.clear();
	skinJointsHandle.clear();
	faceVertexCounts.clear();
	faceIndices.clear();
	faceNormals.clear();
	faceUV0.clear();
	faceUV1.clear();
	faceColor0.clear();
	faceFaceGroupIndex.clear();
	faceGroupMasterSurfaces.clear();
	faceGroupFacesCount.clear();

	materialIndex = -1;
	flipFaces = false;
	subdivision = false;
}

/**
 * 最適化 (不要頂点の除去など).
 * @param[in]  removeUnusedVertices   未使用頂点を削除する場合はtrue.
 */
void CTempMeshData::optimize (const bool removeUnusedVertices)
{
	const float fMin = (float)(1e-5);
	const size_t facesCou = faceVertexCounts.size();
	if (facesCou == 0) return;

	// 面の頂点オフセット.
	std::vector<int> faceVOffset;
	faceVOffset.resize(facesCou);
	for (int i = 0, iPos = 0; i < facesCou; ++i) {
		const int faceVCou = faceVertexCounts[i];
		faceVOffset[i] = iPos;
		iPos += faceVCou;
	}

	// 面積が0の面を削除.
	{
		std::vector<int> removeFaceIndexList;
		const float scale = 1000.0f;
		{
			// TODO : 三角形のみチェックしている.
			for (size_t i = 0; i < facesCou; ++i) {
				const int iPos = faceVOffset[i];

				const int faceVCou = faceVertexCounts[i];
				if (faceVCou <= 2 || faceVCou >= 5) continue;

				const int i1 = faceIndices[iPos + 0];
				const int i2 = faceIndices[iPos + 1];
				const int i3 = faceIndices[iPos + 2];
				if (i1 == i2 || i1 == i3 || i2 == i3) {
					removeFaceIndexList.push_back(i);
					continue;
				}
				const sxsdk::vec3 v1 = vertices[i1] * scale;
				const sxsdk::vec3 v2 = vertices[i2] * scale;
				const sxsdk::vec3 v3 = vertices[i3] * scale;
				if (MathUtil::isZero(v1 - v2, fMin) || MathUtil::isZero(v2 - v3, fMin) || MathUtil::isZero(v1 - v3, fMin)) {
					removeFaceIndexList.push_back(i);
					continue;
				}
				if (MathUtil::calcTriangleArea(v1, v2, v3) < fMin) {
					removeFaceIndexList.push_back(i);
					continue;
				}
			}
		}

		if (!removeFaceIndexList.empty()) {
			const int rCou = (int)removeFaceIndexList.size();
			for (int i = rCou - 1; i >= 0; --i) {
				const int faceIndex = removeFaceIndexList[i];
				const int iPos = faceVOffset[faceIndex];
				const int fvCou = faceVertexCounts[faceIndex];
				for (int j = 0; j < fvCou; ++j) faceIndices.erase(faceIndices.begin() + iPos);
				if (!faceFaceGroupIndex.empty()) {
					faceFaceGroupIndex.erase(faceFaceGroupIndex.begin() + faceIndex);
				}
				if (!faceNormals.empty()) {
					for (int j = 0; j < fvCou; ++j) faceNormals.erase(faceNormals.begin() + iPos);
				}
				if (!faceUV0.empty()) {
					for (int j = 0; j < fvCou; ++j) faceUV0.erase(faceUV0.begin() + iPos);
				}
				if (!faceUV1.empty()) {
					for (int j = 0; j < fvCou; ++j) faceUV1.erase(faceUV1.begin() + iPos);
				}
				if (!faceColor0.empty()) {
					for (int j = 0; j < fvCou; ++j) faceColor0.erase(faceColor0.begin() + iPos);
				}
				faceVertexCounts.erase(faceVertexCounts.begin() + faceIndex);
			}
		}
	}

	// 不要頂点を削除.
	if (removeUnusedVertices) {
		const size_t versCou = vertices.size();

		std::vector<int> useVersList;
		useVersList.resize(versCou, -1);

		for (size_t i = 0; i < faceIndices.size(); ++i) {
			useVersList[ faceIndices[i] ] = 1;
		}
		{
			int iPos = 0;
			for (size_t i = 0; i < versCou; ++i) {
				if (useVersList[i] > 0) useVersList[i] = iPos++;
			}
		}

		for (size_t i = 0; i < faceIndices.size(); ++i) {
			faceIndices[i] = useVersList[ faceIndices[i] ];
		}

		for (int i = (int)versCou - 1; i >= 0; --i) {
			if (useVersList[i] < 0) {
				useVersList.erase(useVersList.begin() + i);
				vertices.erase(vertices.begin() + i);
				if (!skinWeights.empty()) skinWeights.erase(skinWeights.begin() + i);
				if (!skinJointsHandle.empty()) skinJointsHandle.erase(skinJointsHandle.begin() + i);
			}
		}
	}
}

//---------------------------------------------------------------.
/**
 * 1つのメッシュ情報 .
 */
CNodeMeshData::CNodeMeshData ()
{
	clear();
}

CNodeMeshData::CNodeMeshData (const CNodeMeshData& v)
{
	this->name     = v.name;
	this->matrix   = v.matrix;
	this->nodeType = v.nodeType;

	this->vertices         = v.vertices;
	this->normals          = v.normals;
	this->faceUV0          = v.faceUV0;
	this->faceUV1          = v.faceUV1;
	this->color0           = v.color0;
	this->faceVertexCounts = v.faceVertexCounts;
	this->faceIndices      = v.faceIndices;
	this->materialIndex    = v.materialIndex;
	this->skinWeights      = v.skinWeights;
	this->skinJoints       = v.skinJoints;
	this->skinJointsHandle = v.skinJointsHandle;

	this->refMaterialName = v.refMaterialName;
	this->masterSurfaceHangle = v.masterSurfaceHangle;
	this->subdivision = v.subdivision;
}

CNodeMeshData::~CNodeMeshData ()
{
}

void CNodeMeshData::clear ()
{
	name = "";
	matrix = sxsdk::mat4::identity;
	nodeType = USD_DATA::NODE_TYPE::mesh_node;

	vertices.clear();
	normals.clear();
	faceUV0.clear();
	faceUV1.clear();
	color0.clear();
	faceVertexCounts.clear();
	faceIndices.clear();
	materialIndex = -1;
	skinWeights.clear();
	skinJoints.clear();
	skinJointsHandle.clear();

	refMaterialName = "";
	masterSurfaceHangle = NULL;
	subdivision = false;
}

namespace {
	/**
	 * CTempMeshDataからコンバート.
	 * フェイスグループ別にメッシュ化する場合に使用する.
	 * @param[in]  tempMeshData   オリジナルのメッシュデータ.
	 * @param[out] retMeshData    変換後のメッシュデータ.
	 */
	void m_convert (const CTempMeshData& tempMeshData, CNodeMeshData& retMeshData)
	{
		retMeshData.clear();

		retMeshData.name             = tempMeshData.name;
		retMeshData.materialIndex    = tempMeshData.materialIndex;
		retMeshData.vertices         = tempMeshData.vertices;
		retMeshData.faceVertexCounts = tempMeshData.faceVertexCounts;
		retMeshData.faceIndices      = tempMeshData.faceIndices;
		retMeshData.skinWeights      = tempMeshData.skinWeights;
		retMeshData.skinJointsHandle = tempMeshData.skinJointsHandle;
		retMeshData.subdivision      = tempMeshData.subdivision;
		retMeshData.faceUV0          = tempMeshData.faceUV0;
		retMeshData.faceUV1          = tempMeshData.faceUV1;

		//---------------------------------------------------------.
		// 法線やUVは、面ごとの頂点から頂点数分の配列に格納するようにコンバート.
		//---------------------------------------------------------.
		const size_t versCou  = retMeshData.vertices.size();
		const size_t facesCou = retMeshData.faceVertexCounts.size();
		if (versCou == 0 || facesCou <= 0) return;

		// 各頂点で共有する面をリスト.
		std::vector< std::vector< sx::vec<int,2> > > faceIndexInVertexList;
		faceIndexInVertexList.resize(versCou);
		for (size_t i = 0; i < versCou; ++i) faceIndexInVertexList[i].clear();

		for (size_t i = 0, iPos = 0; i < facesCou; ++i) {
			const int fCou = retMeshData.faceVertexCounts[i];
			for (int j = 0; j < fCou; ++j) {
				const int i0 = retMeshData.faceIndices[iPos + j];
				faceIndexInVertexList[i0].push_back(sx::vec<int,2>(i, j));
			}
			iPos += fCou;
		}

		// 面ごとの頂点オフセット用.
		std::vector<int> faceVOffsetList;
		faceVOffsetList.resize(facesCou);
		for (size_t i = 0, iPos = 0; i < facesCou; ++i) {
			const int fCou = retMeshData.faceVertexCounts[i];
			faceVOffsetList[i] = iPos;
			iPos += fCou;
		}

		// 追加した同一頂点番号を格納するバッファ.
		std::vector< std::vector<int> > sameVersList;
		sameVersList.resize(versCou);
		for (size_t i = 0; i < versCou; ++i) sameVersList[i].clear();

		// 共有する頂点のうち、法線が異なる場合は頂点として分離.
		for (size_t vLoop = 0; vLoop < versCou; ++vLoop) {
			const std::vector< sx::vec<int,2> >& facesList = faceIndexInVertexList[vLoop];
			const size_t vtCou = facesList.size();
			if (vtCou <= 1) continue;

			// 法線/UVをチェック.
			for (size_t i = 1; i < vtCou; ++i) {
				const int fIndex  = facesList[i][0];
				const int fOffset = faceVOffsetList[fIndex];

				const int iP1 = fOffset + facesList[i][1];
				const sxsdk::vec3& n_1   = tempMeshData.faceNormals[iP1];

				int sIndex = tempMeshData.subdivision ? 0 : -1;
				for (size_t j = 0; j < i; ++j) {
					const int fIndex2  = facesList[j][0];
					const int fOffset2 = faceVOffsetList[fIndex2];

					const int iP2 = fOffset2 + facesList[j][1];
					const sxsdk::vec3& n_2   = tempMeshData.faceNormals[iP2];
					if (MathUtil::isZero(n_1 - n_2)) {
						sIndex = (int)j;
						break;
					}
				}

				// 新しく頂点を追加して対応.
				if (sIndex < 0) {
					const size_t vIndex = retMeshData.vertices.size();
					{
						const sxsdk::vec3 v = retMeshData.vertices[vLoop];
						retMeshData.vertices.push_back(v);
					}
					if (!retMeshData.skinWeights.empty()) {
						const sxsdk::vec4 v = retMeshData.skinWeights[vLoop];
						retMeshData.skinWeights.push_back(v);
					}
					if (!retMeshData.skinJointsHandle.empty()) {
						const sx::vec<void*,4> v = retMeshData.skinJointsHandle[vLoop];
						retMeshData.skinJointsHandle.push_back(v);
					}
					sameVersList[vLoop].push_back(vIndex);

					retMeshData.faceIndices[iP1] = vIndex;
				} else {
					const int fIndex2  = facesList[sIndex][0];
					const int fOffset2 = faceVOffsetList[fIndex2];

					const int iP2 = fOffset2 + facesList[sIndex][1];
					retMeshData.faceIndices[iP1] = retMeshData.faceIndices[iP2];
				}
			}
		}

		// 法線とUVを格納.
		const size_t newVersCou = retMeshData.vertices.size();
		retMeshData.normals.resize(newVersCou);

		for (size_t i = 0, iPos = 0; i < facesCou; ++i) {
			const int fCou = retMeshData.faceVertexCounts[i];

			for (int j = 0; j < fCou; ++j) {
				const int iV = retMeshData.faceIndices[iPos + j];
				retMeshData.normals[iV] = tempMeshData.faceNormals[iPos + j];
			}
			iPos += fCou;
		}

		// 頂点カラーを格納.
		if (!tempMeshData.faceColor0.empty()) {
			retMeshData.color0.resize(newVersCou);
			for (size_t i = 0, iPos = 0; i < facesCou; ++i) {
				const int fCou = retMeshData.faceVertexCounts[i];

				for (int j = 0; j < fCou; ++j) {
					const int iV = retMeshData.faceIndices[iPos + j];
					retMeshData.color0[iV] = tempMeshData.faceColor0[iPos + j];
				}
				iPos += fCou;
			}
		}
	}
}

/**
 * 頂点座標のバウンディングボックスを計算.
 */
void CNodeMeshData::calcBoundingBox (sxsdk::vec3& bbMin, sxsdk::vec3& bbMax) const
{
	const size_t versCou = vertices.size();
	bbMin = bbMax = sxsdk::vec3(0, 0, 0);
	if (versCou == 0) return;

	bbMin = bbMax = vertices[0];
	for (size_t i = 1; i < versCou; ++i) {
		const sxsdk::vec3& v = vertices[i];
		bbMin.x = std::min(bbMin.x, v.x);
		bbMin.y = std::min(bbMin.y, v.y);
		bbMin.z = std::min(bbMin.z, v.z);
		bbMax.x = std::max(bbMax.x, v.x);
		bbMax.y = std::max(bbMax.y, v.y);
		bbMax.z = std::max(bbMax.z, v.z);
	}
}

/**
 * CTempMeshDataから複数のフェイスグループを考慮して、コンバート.
 */
int CNodeMeshData::convert (const CTempMeshData& tempMeshData, std::vector<CNodeMeshData>& meshes)
{
	meshes.clear();

	CTempMeshData srcMeshD = tempMeshData;
	srcMeshD.optimize();		// 不要頂点の除去など.
	const size_t facesCou = srcMeshD.faceVertexCounts.size();
	if (facesCou == 0) return 0;

	// フェイスグループに属さない面数を取得.
	const size_t faceGroupsCount = srcMeshD.faceGroupFacesCount.size();
	int noneFaceGroupFacesCount = 0;
	if (faceGroupsCount > 0) {
		for (size_t i = 0; i < facesCou; ++i) {
			const int faceGroupIndex = srcMeshD.faceFaceGroupIndex[i];
			if (faceGroupIndex < 0) {
				noneFaceGroupFacesCount++;
			}
		}
	} else {
		noneFaceGroupFacesCount = facesCou;
	}

	// 面の頂点オフセット.
	std::vector<int> faceVOffset;
	faceVOffset.resize(facesCou);
	for (int i = 0, iPos = 0; i < facesCou; ++i) {
		const int faceVCou = srcMeshD.faceVertexCounts[i];
		faceVOffset[i] = iPos;
		iPos += faceVCou;
	}

	// フェイスグループに属さない面だけを取り出し、1Meshにする.
	if (noneFaceGroupFacesCount > 0) {
		CTempMeshData newMeshD;
		newMeshD.clear();
		newMeshD.name             = srcMeshD.name;
		newMeshD.vertices         = srcMeshD.vertices;
		newMeshD.skinWeights      = srcMeshD.skinWeights;
		newMeshD.skinJoints       = srcMeshD.skinJoints;
		newMeshD.skinJointsHandle = srcMeshD.skinJointsHandle;
		newMeshD.subdivision      = srcMeshD.subdivision;

		for (size_t loop = 0; loop < facesCou; ++loop) {
			const int faceGroupIndex = srcMeshD.faceFaceGroupIndex[loop];
			if (faceGroupIndex >= 0) continue;

			const int fOffset = faceVOffset[loop];
			const int faceVCou = srcMeshD.faceVertexCounts[loop];	// 面の頂点数.
			int fOffset2 = fOffset;
			int fInc = 1;
			if (srcMeshD.flipFaces) {		// 面反転.
				fOffset2 += faceVCou - 1;
				fInc = -1;
			}
			for (int i = 0; i < faceVCou; ++i, fOffset2 += fInc) {
				newMeshD.faceIndices.push_back(srcMeshD.faceIndices[fOffset2]);
				if (!srcMeshD.faceNormals.empty()) newMeshD.faceNormals.push_back(srcMeshD.faceNormals[fOffset2] * (srcMeshD.flipFaces ? -1.0f : 1.0f));
				if (!srcMeshD.faceUV0.empty()) newMeshD.faceUV0.push_back(srcMeshD.faceUV0[fOffset2]);
				if (!srcMeshD.faceUV1.empty()) newMeshD.faceUV1.push_back(srcMeshD.faceUV1[fOffset2]);
				if (!srcMeshD.faceColor0.empty()) newMeshD.faceColor0.push_back(srcMeshD.faceColor0[fOffset2]);
			}
			newMeshD.faceVertexCounts.push_back(faceVCou);
		}

		if (!newMeshD.faceVertexCounts.empty()) {
			newMeshD.optimize();	// 参照されていない不要頂点の削除.

			// メッシュデータをコンバートして格納.
			CNodeMeshData nMeshD;
			m_convert(newMeshD, nMeshD);
			meshes.push_back(nMeshD);
		}
	}

	// フェイスグループごとに1Meshに分けて格納.
	for (size_t fLoop = 0; fLoop < faceGroupsCount; ++fLoop) {
		CTempMeshData newMeshD;
		newMeshD.name             = srcMeshD.name;
		newMeshD.vertices         = srcMeshD.vertices;
		newMeshD.skinWeights      = srcMeshD.skinWeights;
		newMeshD.skinJoints       = srcMeshD.skinJoints;
		newMeshD.skinJointsHandle = srcMeshD.skinJointsHandle;
		newMeshD.subdivision      = srcMeshD.subdivision;
		sxsdk::master_surface_class* masterSurface = srcMeshD.faceGroupMasterSurfaces[fLoop];
		if (masterSurface == NULL) continue;

		for (size_t loop = 0; loop < facesCou; ++loop) {
			const int faceGroupIndex = srcMeshD.faceFaceGroupIndex[loop];
			if (faceGroupIndex != fLoop) continue;

			const int fOffset = faceVOffset[loop];
			const int faceVCou = srcMeshD.faceVertexCounts[loop];	// 面の頂点数.
			int fOffset2 = fOffset;
			int fInc = 1;
			if (srcMeshD.flipFaces) {		// 面反転.
				fOffset2 += faceVCou - 1;
				fInc = -1;
			}
			for (int i = 0; i < faceVCou; ++i, fOffset2 += fInc) {
				newMeshD.faceIndices.push_back(srcMeshD.faceIndices[fOffset2]);
				if (!srcMeshD.faceNormals.empty()) newMeshD.faceNormals.push_back(srcMeshD.faceNormals[fOffset2] * (srcMeshD.flipFaces ? -1.0f : 1.0f));
				if (!srcMeshD.faceUV0.empty()) newMeshD.faceUV0.push_back(srcMeshD.faceUV0[fOffset2]);
				if (!srcMeshD.faceUV1.empty()) newMeshD.faceUV1.push_back(srcMeshD.faceUV1[fOffset2]);
				if (!srcMeshD.faceColor0.empty()) newMeshD.faceColor0.push_back(srcMeshD.faceColor0[fOffset2]);
			}
			newMeshD.faceVertexCounts.push_back(faceVCou);
		}

		if (!newMeshD.faceVertexCounts.empty()) {
			newMeshD.optimize();	// 参照されていない不要頂点の削除.

			// メッシュデータをコンバートして格納.
			CNodeMeshData nMeshD;
			m_convert(newMeshD, nMeshD);
			nMeshD.masterSurfaceHangle = masterSurface->get_handle();
			meshes.push_back(nMeshD);
		}
	}

	return (int)meshes.size();
}

/**
 * 頂点カラー情報で、Alphaを出力する必要があるか.
 */
bool CNodeMeshData::hasNeedVertexColorAlpha () const
{
	if (color0.empty()) return false;

	bool hasAlpha = false;

	const size_t vCou = color0.size();
	for (size_t i = 0; i < vCou; ++i) {
		const sxsdk::vec4& col = color0[i];
		if (!MathUtil::isZero(col.w - 1.0)) {
			hasAlpha = true;
			break;
		}
	}
	return hasAlpha;
}

/**
 * USD_DATA::MeshDataにコンバート.
 * @param[out] usdMeshData  USDのメッシュデータとしての格納先.
 */
void CNodeMeshData::convertTo (USD_DATA::MeshData& usdMeshData)
{
	usdMeshData.clear();
	usdMeshData.subdivision = this->subdivision;

	const size_t versCou = vertices.size();
	const size_t facesCou = faceVertexCounts.size();

	if (!vertices.empty()) {
		usdMeshData.vertices.resize(versCou * 3);
		for (size_t i = 0, iPos = 0; i < vertices.size(); ++i, iPos += 3) {
			usdMeshData.vertices[iPos + 0] = vertices[i].x;
			usdMeshData.vertices[iPos + 1] = vertices[i].y;
			usdMeshData.vertices[iPos + 2] = vertices[i].z;
		}
	}

	if (!normals.empty() && normals.size() == versCou) {
		usdMeshData.normals.resize(versCou * 3);
		for (size_t i = 0, iPos = 0; i < normals.size(); ++i, iPos += 3) {
			usdMeshData.normals[iPos + 0] = normals[i].x;
			usdMeshData.normals[iPos + 1] = normals[i].y;
			usdMeshData.normals[iPos + 2] = normals[i].z;
		}
	}

	if (!faceUV0.empty() && faceUV0.size() == faceIndices.size()) {
		for (size_t i = 0, iPos = 0; i < facesCou; ++i) {
			const int fvCou = faceVertexCounts[i];
			for (int j = 0; j < fvCou; ++j) {
				const sxsdk::vec2& v2 = faceUV0[iPos + j];
				usdMeshData.faceUV0.push_back(v2.x);
				usdMeshData.faceUV0.push_back(v2.y);
			}
			iPos += fvCou;
		}
	}
	if (!faceUV1.empty() && faceUV1.size() == faceIndices.size()) {
		for (size_t i = 0, iPos = 0; i < facesCou; ++i) {
			const int fvCou = faceVertexCounts[i];
			for (int j = 0; j < fvCou; ++j) {
				const sxsdk::vec2& v2 = faceUV1[iPos + j];
				usdMeshData.faceUV1.push_back(v2.x);
				usdMeshData.faceUV1.push_back(v2.y);
			}
			iPos += fvCou;
		}
	}

	if (!color0.empty() && color0.size() == versCou) {
		usdMeshData.color0.resize(versCou * 3);
		for (size_t i = 0, iPos = 0; i < color0.size(); ++i, iPos += 3) {
			usdMeshData.color0[iPos + 0] = color0[i].x;
			usdMeshData.color0[iPos + 1] = color0[i].y;
			usdMeshData.color0[iPos + 2] = color0[i].z;
		}
	}

	usdMeshData.faceVertexCounts = faceVertexCounts;
	usdMeshData.faceIndices      = faceIndices;

	usdMeshData.refMaterialName = refMaterialName;
	usdMeshData.materialIndex   = materialIndex;

	if (!skinWeights.empty()) {
		const size_t vCou = skinWeights.size();
		usdMeshData.skinWeights.resize(vCou);
		for (size_t i = 0; i < vCou; ++i) {
			usdMeshData.skinWeights[i].resize(4);
			for (size_t j = 0; j < 4; ++j) {
				usdMeshData.skinWeights[i][j] = skinWeights[i][j];
			}
		}
	}
	if (!skinJoints.empty()) {
		const size_t vCou = skinJoints.size();
		usdMeshData.skinJoints.resize(vCou);
		for (size_t i = 0; i < vCou; ++i) {
			usdMeshData.skinJoints[i].resize(4);
			for (size_t j = 0; j < 4; ++j) {
				usdMeshData.skinJoints[i][j] = skinJoints[i][j];
			}
		}
	}
	if (!skinJointsHandle.empty()) {
		const size_t vCou = skinJointsHandle.size();
		usdMeshData.skinJointsHandle.resize(vCou);
		for (size_t i = 0; i < vCou; ++i) {
			usdMeshData.skinJointsHandle[i].resize(4);
			for (size_t j = 0; j < 4; ++j) {
				usdMeshData.skinJointsHandle[i][j] = skinJointsHandle[i][j];
			}
		}
	}
}

//---------------------