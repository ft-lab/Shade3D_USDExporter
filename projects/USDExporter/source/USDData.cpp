/**
 * USD格納時に使用する情報.
 */
#include "USDData.h"
#include <math.h>

//--------------------------------------------------.
USD_DATA::MeshData::MeshData ()
{
	clear();
}

void USD_DATA::MeshData::clear ()
{
	vertices.clear();
	normals.clear();
	color0.clear();
	faceVertexCounts.clear();
	faceIndices.clear();
	faceUV0.clear();
	faceUV1.clear();

	skinWeights.clear();
	skinJoints.clear();
	skinJointsHandle.clear();

	refMaterialName = "";
	materialIndex = -1;
	skinSkeletonIndex = -1;
	subdivision = false;
	faceGroupMesh = false;
}

//--------------------------------------------------.

USD_DATA::NodeMatrixData::NodeMatrixData ()
{
	clear();
}

USD_DATA::NodeMatrixData::~NodeMatrixData ()
{
}

void USD_DATA::NodeMatrixData::clear()
{
	translate[0] = translate[1] = translate[2] = 0.0f;
	rotate[0] = rotate[1] = rotate[2] = 0.0f;
	scale[0] = scale[1] = scale[2] = 1.0f;
}

//--------------------------------------------------.

/**
 * テクスチャの要素から文字列を取得.
 */
const std::string USD_DATA::getTextureSourceString (const USD_DATA::TEXTURE_SOURE& texSource) {
	switch (texSource) {
	case USD_DATA::TEXTURE_SOURE::texture_source_rgb:
		return "rgb";
	case USD_DATA::TEXTURE_SOURE::texture_source_r:
		return "r";
	case USD_DATA::TEXTURE_SOURE::texture_source_g:
		return "g";
	case USD_DATA::TEXTURE_SOURE::texture_source_b:
		return "b";
	case USD_DATA::TEXTURE_SOURE::texture_source_a:
		return "a";
	}
	return "";
}

/**
 * 指定の数値がゼロかチェック.
 */
bool USD_DATA::isZero (const float v, const float fMin)
{
	return (std::abs(v) < fMin && std::abs(v) < fMin);
}

bool USD_DATA::isZero (const float v1, const float v2, const float v3, const float fMin)
{
	return ((std::abs(v1) < fMin && std::abs(v1) < fMin) && (std::abs(v2) < fMin && std::abs(v2) < fMin) && (std::abs(v3) < fMin && std::abs(v3) < fMin));
}

/**
 * 4x4行列に単位行列を指定.
 * @param[out] matrix 4x4行列の格納用.
 */
void USD_DATA::setMatrix4x4_Identity (std::vector<float>& matrix)
{
	matrix.resize(16);

	int iPos = 0;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			matrix[iPos] = (i == j) ? 1.0f : 0.0f;
			iPos++;
		}
	}
}

/**
 * 4x4行列を指定.
 * @param[in]  pM     16個のfloat配列.
 * @param[out] matrix 4x4行列の格納用.
 */
void USD_DATA::setMatrix4x4 (const float* pM, std::vector<float>& matrix)
{
	matrix.resize(16);
	int iPos = 0;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			matrix[iPos] = pM[iPos];
			iPos++;
		}
	}
}

/**
 * 色情報を逆ガンマ2.2し、リニア化.
 * @param[in/out] vRed    Red値.
 * @param[in/out] vGreen  Green値.
 * @param[in/out] vBlue   Blue値.
 */
void USD_DATA::convColorLinear (float& vRed, float& vGreen, float& vBlue)
{
	const float gamma = 2.2f;
	if (vRed < 1.0f) {
		vRed   = powf(vRed, gamma);
	}
	if (vGreen < 1.0f) {
		vGreen = powf(vGreen, gamma);
	}
	if (vBlue < 1.0f) {
		vBlue  = powf(vBlue, gamma);
	}
}
