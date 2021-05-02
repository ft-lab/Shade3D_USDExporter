/**
 * Meshの表面積を計算.
 */
#include "CalcSurfaceArea.h"
#include "MathUtil.h"

namespace
{
	/**
	 * 多角形の三角形分割クラス.
	 */
	std::vector<int> m_triangleIndex; 

	class CDivideTrianglesOutput : public sxsdk::shade_interface::output_function_class {
	private:
	public:
		virtual void output (int i0 , int i1 , int i2 , int i3) {
			const int offset = m_triangleIndex.size();
			m_triangleIndex.resize(offset + 3);
			m_triangleIndex[offset + 0] = i0;
			m_triangleIndex[offset + 1] = i1;
			m_triangleIndex[offset + 2] = i2;
		}
	};
}

double MathUtil::calcSurfaceArea (sxsdk::scene_interface* scene, const CNodeMeshData& meshD)
{
	double areaV = 0.0;
	
	::m_triangleIndex.clear();
	::CDivideTrianglesOutput divC;

	int iPos = 0;
	std::vector<sxsdk::vec3> vertices;
	vertices.resize(50);
	for (size_t loop = 0; loop < meshD.faceVertexCounts.size(); ++loop) {
		// 面の頂点座標を取得.
		const int vCou = meshD.faceVertexCounts[loop];
		if (vCou < 3) continue;
		if (vCou >= (int)vertices.size()) {
			vertices.resize(vCou + 16);
		}
		for (int i = 0; i < vCou; ++i) {
			vertices[i] = meshD.vertices[ meshD.faceIndices[i + iPos] ];
		}
		if (vCou == 3) {
			areaV += MathUtil::calcTriangleArea(vertices[0], vertices[1], vertices[2]);
			continue;
		}

		// 多角形を三角形分割.
		scene->divide_polygon(divC, vCou, &(vertices[0]), true);
		const int triCou = ::m_triangleIndex.size() / 3;

		for (int i = 0; i < ::m_triangleIndex.size(); i += 3) {
			areaV += MathUtil::calcTriangleArea(vertices[ ::m_triangleIndex[i + 0] ], vertices[ ::m_triangleIndex[i + 1] ], vertices[ ::m_triangleIndex[i + 2] ]);
		}

		iPos += vCou;
	}

	return areaV;
}
