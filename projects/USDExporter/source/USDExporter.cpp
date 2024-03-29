﻿/**
 * USD出力機能.
 */

/**
 参考:
  https://graphics.pixar.com/usd/docs/Generating-New-Schema-Classes.html
*/

#include "USDExporter.h"
#include "MathUtil.h"
#include "StringUtil.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/nurbsCurves.h"
#include "pxr/usd/usd/modelAPI.h"							// SetKind.

#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"		// USD v.21.02

#include "pxr/usd/usd/zipfile.h"

#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/bindingAPI.h"

#include "pxr/base/gf/rotation.h"		// GfRotation で使用.
#include "pxr/base/gf/matrix4f.h"		// GfMatrix4f で使用.

#include <vector>
#include <cstdlib>
#include <memory>
#include <iostream>
#include <map>

// 以下のnamespace内に、UsdXXXXのクラスがある.
using namespace PXR_INTERNAL_NS;

UsdStageRefPtr g_stage = NULL;		// USDエクスポート時のクラス.

#define ROOT_PATH  "/root"
#define MATERIAL_ROOT_PATH  "/root/Materials"
#define SKELETONS_ROOT_PATH  "/root/Skeletons"

// min-max範囲指定を使用するかどうか.
#define USE_DICTIONARY_RANGE  0

 namespace {
	 /**
	  * UsdPrimに行列 (移動/回転/スケール)を指定.
	  */
	void m_setMatrix (UsdPrim node, const USD_DATA::NodeMatrixData& matrix)
	{
		// 位置を指定.
		if (!USD_DATA::isZero(matrix.translate[0], matrix.translate[1], matrix.translate[2])) {
			UsdGeomXformOp transOp = UsdGeomXform(node).AddTranslateOp(UsdGeomXformOp::PrecisionFloat);
			transOp.Set(GfVec3f(matrix.translate[0], matrix.translate[1], matrix.translate[2]));
		}

		// 回転を指定.
		if (!USD_DATA::isZero(matrix.rotate[0], matrix.rotate[1], matrix.rotate[2])) {
			UsdGeomXformOp transOp = UsdGeomXform(node).AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat);
			transOp.Set(GfVec3f(matrix.rotate[0], matrix.rotate[1], matrix.rotate[2]));
		}

		// スケールを指定.
		if (!USD_DATA::isZero(1.0f - matrix.scale[0], 1.0f - matrix.scale[1], 1.0f - matrix.scale[2])) {
			UsdGeomXformOp transOp = UsdGeomXform(node).AddScaleOp(UsdGeomXformOp::PrecisionFloat);
			transOp.Set(GfVec3f(matrix.scale[0], matrix.scale[1], matrix.scale[2]));
		}
	}
 }

CUSDExporter::CUSDExporter ()
{
	clear();
	g_stage = NULL;
	m_versionString = "";
}

/**
 * 球を作成するだけのサンプル.
 */
void CUSDExporter::testCreateSphere (const std::string& fileName)
{
	UsdStageRefPtr stage = UsdStage::CreateNew(fileName);

	stage->DefinePrim(SdfPath("/hello"), TfToken("Xform"));
	UsdPrim sphere = stage->DefinePrim(SdfPath("/hello/sphere"), TfToken("Sphere"));

	// ジオメトリのTokenとして使用できるのは、
	// https://graphics.pixar.com/usd/docs/api/usd_geom_2tokens_8h_source.html

	// 半径を指定.
	sphere.GetAttribute(TfToken("radius")).Set(1.5);

	// extent (バウンディングボックス)を指定.
	UsdAttribute attr = sphere.GetAttribute(TfToken("extent"));

	std::vector<GfVec3f> vList;
	vList.push_back(GfVec3f(-1.5f, -1.5f, -1.5f));
	vList.push_back(GfVec3f(1.5f, 1.5f, 1.5f));
	VtVec3fArray ar = VtVec3fArray(vList.begin(), vList.end());
	attr.Set(ar);

	// displayColorを指定.
	std::vector<GfVec3f> colorList;
	colorList.push_back(GfVec3f(1.0f, 0.2f, 0.0f));
	UsdGeomGprim(sphere).GetDisplayColorAttr().Set(VtVec3fArray(colorList.begin(), colorList.end()));

	stage->Save();
}

/**
 * NURBSを配置するサンプル (TODO).
 */
void CUSDExporter::testCreateNURBSCurves (const std::string& fileName)
{
	UsdStageRefPtr stage = UsdStage::CreateNew(fileName);

	stage->DefinePrim(SdfPath("/hello"), TfToken("Xform"));
	UsdPrim prim = stage->DefinePrim(SdfPath("/hello/nurbs"), TfToken("NurbsCurve"));
	UsdGeomNurbsCurves nurbs(prim);

	UsdAttribute knots = nurbs.CreateKnotsAttr();

	stage->Save();
}

/**
 * ノードや形状の位置、回転、スケール指定のサンプル.
 */
void CUSDExporter::testCreateSphere_xformOp (const std::string& fileName)
{
	UsdStageRefPtr stage = UsdStage::CreateNew(fileName);

	UsdPrim node1 = stage->DefinePrim(SdfPath("/hello"), TfToken("Xform"));
	UsdPrim sphere = stage->DefinePrim(SdfPath("/hello/sphere"), TfToken("Sphere"));

	// 位置を指定.
	{
		UsdGeomXformOp transOp = UsdGeomXform(node1).AddTranslateOp(UsdGeomXformOp::PrecisionDouble);
		transOp.Set(GfVec3d(0.1, 0.5, 0.2));
	}

	// 回転を指定.
	{
		UsdGeomXformOp transOp = UsdGeomXform(node1).AddRotateXYZOp(UsdGeomXformOp::PrecisionDouble);
		transOp.Set(GfVec3d(0, 0, 45));
	}

	// スケールを指定.
	{
		UsdGeomXformOp transOp = UsdGeomXform(node1).AddScaleOp(UsdGeomXformOp::PrecisionDouble);
		transOp.Set(GfVec3d(1.5, 1, 1));
	}

	stage->Save();
}

/**
 * メッシュを作成するサンプル.
 */
void CUSDExporter::testCreateMesh (const std::string& fileName)
{
	UsdStageRefPtr stage = UsdStage::CreateNew(fileName);

	UsdPrim node1 = stage->DefinePrim(SdfPath("/hello"), TfToken("Xform"));

	// Meshを作成.
	UsdPrim prim = stage->DefinePrim(SdfPath("/hello/mesh"), TfToken("Mesh"));
	UsdGeomMesh geomMesh(prim);

	// Meshの頂点を格納.
	{
		std::vector<GfVec3f> vList;
		vList.push_back(GfVec3f(-1, 0, -1));
		vList.push_back(GfVec3f(-1, 0, 1));
		vList.push_back(GfVec3f(1, 0, 1));
		vList.push_back(GfVec3f(1, 0, -1));

		UsdAttribute attr = geomMesh.CreatePointsAttr();
		VtVec3fArray ar = VtVec3fArray(vList.begin(), vList.end());
		attr.Set(ar);
	}

	// 頂点の法線を格納.
	{
		std::vector<GfVec3f> vList;
		vList.push_back(GfVec3f(0, 1, 0));
		vList.push_back(GfVec3f(0, 1, 0));
		vList.push_back(GfVec3f(0, 1, 0));
		vList.push_back(GfVec3f(0, 1, 0));

		UsdAttribute attr = geomMesh.CreateNormalsAttr();
		VtVec3fArray ar = VtVec3fArray(vList.begin(), vList.end());
		attr.Set(ar);
	}

	// Meshの面情報を格納.
	{
		std::vector<int> iList;
		iList.push_back(4);

		UsdAttribute attr = geomMesh.CreateFaceVertexCountsAttr();
		VtIntArray ar = VtIntArray(iList.begin(), iList.end());
		attr.Set(ar);
	}
	{
		std::vector<int> iList;
		iList.push_back(0);
		iList.push_back(1);
		iList.push_back(2);
		iList.push_back(3);

		UsdAttribute attr = geomMesh.CreateFaceVertexIndicesAttr();
		VtIntArray ar = VtIntArray(iList.begin(), iList.end());
		attr.Set(ar);
	}

	// UVを格納.
	{
		std::vector<GfVec2f> uvList;
		uvList.push_back(GfVec2f(0, 0));
		uvList.push_back(GfVec2f(1, 0));
		uvList.push_back(GfVec2f(1, 1));
		uvList.push_back(GfVec2f(0, 1));

		UsdGeomPrimvar primV = geomMesh.CreatePrimvar(TfToken("st"), SdfValueTypeNames->TexCoord2fArray, UsdGeomTokens->varying);
		UsdAttribute attr = primV.GetAttr();
		VtVec2fArray ar = VtVec2fArray(uvList.begin(), uvList.end());
		attr.Set(ar);
	}

	//doubleSided指定.
	//prim.GetAttribute(TfToken("doubleSided")).Set(false);
	geomMesh.CreateDoubleSidedAttr(VtValue(false));

	// マテリアルを指定.
	{
		UsdPrim primMat = stage->DefinePrim(SdfPath("/hello/material_0"), TfToken("Material"));
		UsdShadeMaterial mat(primMat);

		// Shaderの作成.
		UsdPrim primShader = stage->DefinePrim(SdfPath("/hello/material_0/PBRShader"), TfToken("Shader"));
		UsdShadeShader shader(primShader);

		shader.CreateIdAttr().Set(TfToken("UsdPreviewSurface"));

		// 以下のdiffuseColorは、diffuseTextureを使用しない場合に色指定.
		//shader.CreateInput(TfToken("diffuseColor"), SdfValueTypeNames->Color3f).Set(GfVec3f(0.5f, 0.9f, 0.2f));

		shader.CreateInput(TfToken("roughness"), SdfValueTypeNames->Float).Set(0.3f);
		shader.CreateInput(TfToken("metallic"), SdfValueTypeNames->Float).Set(0.3f);

		// Texture Samplerの指定。UV読み込みのReaderとセットで指定している.
		UsdPrim primReader = stage->DefinePrim(SdfPath("/hello/material_0/stReader"), TfToken("Shader"));
		UsdShadeShader shaderReader(primReader);
		shaderReader.CreateIdAttr().Set(TfToken("UsdPrimvarReader_float2"));

		UsdPrim primDiffuseTexture = stage->DefinePrim(SdfPath("/hello/material_0/diffuseTexture"), TfToken("Shader"));
		UsdShadeShader shaderDiffuseTexture(primDiffuseTexture);

		shaderDiffuseTexture.CreateIdAttr().Set(TfToken("UsdUVTexture"));

		// diffuseTextureのファイル名を指定.
		shaderDiffuseTexture.CreateInput(TfToken("file"), SdfValueTypeNames->Asset).Set(SdfAssetPath("tile_image.png"));

		// UVのためのReaderと接続.
		shaderDiffuseTexture.CreateInput(TfToken("st"), SdfValueTypeNames->Float2).ConnectToSource(shaderReader, TfToken("result"));

		// diffuseTextureの出力指定.
		shaderDiffuseTexture.CreateOutput(TfToken("rgb"), SdfValueTypeNames->Float3);

		// DiffuseTextureをshaderのdiffuseColorにマッピング.
		shader.CreateInput(TfToken("diffuseColor"), SdfValueTypeNames->Color3f).ConnectToSource(shaderDiffuseTexture, TfToken("rgb"));

		// UVの接続.
		UsdShadeInput stInput = mat.CreateInput(TfToken("frame:stPrimvarName"), SdfValueTypeNames->Token);
		stInput.Set(TfToken("st"));

		// DiffuseTextureに割り当てるUVを接続.
		shaderReader.CreateInput(TfToken("varname"), SdfValueTypeNames->Token).ConnectToSource(stInput);

		// MaterialからShaderをつなぐ.
		mat.CreateSurfaceOutput().ConnectToSource(shader, TfToken("surface"));

		// マテリアルをprimの形状にバインド.
		// USD v.21.02ではUsdShadeMaterialBindingAPIを使用する.
		//mat.Bind(prim);
		UsdShadeMaterialBindingAPI(prim).Bind(mat);
	}

	stage->Save();
}

/**
 * メッシュを作成し、回転アニメーションさせるサンプル.
 */
void CUSDExporter::testCreateSphereWithAnimation (const std::string& fileName)
{
	UsdStageRefPtr stage = UsdStage::CreateNew(fileName);

	UsdPrim node1 = stage->DefinePrim(SdfPath("/root"), TfToken("Xform"));
	UsdPrim sphere = stage->DefinePrim(SdfPath("/root/sphere"), TfToken("Sphere"));

	// アニメーションを指定.
	{
		stage->SetFramesPerSecond(30.0);
		stage->SetStartTimeCode(0.0);
		stage->SetEndTimeCode(300.0);
	}

	// 回転アニメーションを指定.
	{
		UsdGeomXformOp transOp = UsdGeomXform(sphere).AddRotateYOp(UsdGeomXformOp::PrecisionFloat);
		transOp.Set(VtValue(0.0f), 0.0f);
		transOp.Set(VtValue(1440.0f), 300.0f);
	}

	// スケールを指定.
	{
		UsdGeomXformOp transOp = UsdGeomXform(sphere).AddScaleOp(UsdGeomXformOp::PrecisionDouble);
		transOp.Set(GfVec3d(1.5, 1, 1));
	}

	stage->Save();
}

//-----------------------------------------------------------.
// USDエクスポート用.
//-----------------------------------------------------------.
void CUSDExporter::clear ()
{
	m_pathStack.clear();
	m_currentPath.clear();
	m_skeletonsList.clear();
	m_imagesList.clear();
}

/**
 * バージョン文字列を渡す.
 * @param[in] verStr  バージョン文字列。 "0.0.1.2" など.
 */
void CUSDExporter::setVersionString (const std::string& verStr)
{
	m_versionString = verStr;

	// ヘッダ情報を出力.
	m_outputHeaders();
}

/**
 * Export開始.
 */
void CUSDExporter::beginExport (const std::string& fileName, const CExportParam& exportParam)
{
	g_stage = UsdStage::CreateNew(fileName);
	m_exportParam = exportParam;

	if (g_stage) {
		// rootノードを出力.
		UsdPrim prim = g_stage->DefinePrim(SdfPath(ROOT_PATH), TfToken("Xform"));

		// rootノードのkindを指定.
		// "", "subcomponent", "component", "assembly", "group"
		if (exportParam.optKind != USD_DATA::EXPORT::KIND_TYPE::kind_none) {
			std::string kindStr = "";
			switch (exportParam.optKind) {
			case USD_DATA::EXPORT::KIND_TYPE::kind_subcomponent:
				kindStr = "subcomponent";
				break;

			case USD_DATA::EXPORT::KIND_TYPE::kind_component:
				kindStr = "component";
				break;

			case USD_DATA::EXPORT::KIND_TYPE::kind_assembly:
				kindStr = "assembly";
				break;

			case USD_DATA::EXPORT::KIND_TYPE::kind_group:
				kindStr = "group";
				break;
			}
			UsdModelAPI(prim).SetKind(TfToken(kindStr.c_str()));
		}

		// 外部参照される場合のデフォルトPrimの指定.
		g_stage->SetDefaultPrim(prim);

		// ヘッダ情報を出力.
		m_outputHeaders();
	}
}

/**
 * ヘッダ情報を出力.
 */
void CUSDExporter::m_outputHeaders ()
{
	if (!g_stage) return;
	if (g_stage->GetRootLayer()) {
		std::string dStr = std::string("USD Exporter for Shade3D");
		if (m_versionString != "") {
			dStr += std::string(" (") + m_versionString + std::string(")");
		}
		g_stage->GetRootLayer()->SetDocumentation(dStr);
	}
}

/**
 * Export終了.
 */
void CUSDExporter::endExport ()
{
	if (!g_stage) return;

	// ファイル保存.
	g_stage->Save();

	g_stage.Reset();
	g_stage = NULL;
}

/**
 * スケルトン情報を渡す.
 */
void CUSDExporter::setSkeletonsData (const std::vector<CSkeletonData>& skelData)
{
	m_skeletonsList = skelData;
}

/**
 * イメージ情報を渡す.
 */
void CUSDExporter::SetImagesList (const std::vector<CImageData>& imagesList)
{
	m_imagesList = imagesList;
}

/**
 * Materialノードを出力.
 * @param[in] materialData   マテリアルデータ.
 */
void CUSDExporter::appendNodeMaterial (const CMaterialData& materialData)
{
	if (!g_stage) return;

	// "/Materials"が存在しない場合は追加.
	const std::string materialsName = MATERIAL_ROOT_PATH;
	UsdPrim prim = g_stage->GetPrimAtPath(SdfPath(materialsName));
	if (!prim.IsValid()) {
		g_stage->DefinePrim(SdfPath(materialsName), TfToken("Scope"));
	}

	//-----------------------------------.
	// マテリアルを追加.
	//-----------------------------------.
	UsdPrim primMat = g_stage->DefinePrim(SdfPath(materialData.name), TfToken("Material"));
	m_appendNodeMaterial(materialData.name, materialData);
}

/**
 * OmniverseのMDLとしてMaterialノードを出力.
 * @param[in] materialData   マテリアルデータ.
 */
void CUSDExporter::appendNodeMaterial_OmniverseMDL (const CMaterialData& materialData)
{
	if (!g_stage) return;

	// "/Materials"が存在しない場合は追加.
	const std::string materialsName = MATERIAL_ROOT_PATH;
	UsdPrim prim = g_stage->GetPrimAtPath(SdfPath(materialsName));
	if (!prim.IsValid()) {
		g_stage->DefinePrim(SdfPath(materialsName), TfToken("Scope"));
	}

	//-----------------------------------.
	// マテリアルを追加.
	//-----------------------------------.
	UsdPrim primMat = g_stage->DefinePrim(SdfPath(materialData.name), TfToken("Material"));
	m_appendNodeMaterial_OmniverseMDL(materialData.name, materialData);
}

/**
 * 指定のUSDのパスにマテリアル(UsdPreviewSurface)情報を格納.
 * @param[in] pathStr        USD上のパス (/root/xxx/red).
 * @param[in] materialData   マテリアルデータ.
 */
void CUSDExporter::m_appendNodeMaterial (const std::string& pathStr, const CMaterialData& materialData)
{
	UsdPrim primMat = g_stage->GetPrimAtPath(SdfPath(pathStr));
	if (!primMat.IsValid()) return;

	UsdShadeMaterial mat(primMat);

	// PBR Shaderの作成.
	UsdPrim primShader = g_stage->DefinePrim(SdfPath(pathStr + std::string("/PBRShader")), TfToken("Shader"));
	UsdShadeShader shader(primShader);

	shader.CreateIdAttr().Set(TfToken("UsdPreviewSurface"));

	if (materialData.diffuseTexture.textureParam.imageIndex < 0) {
		// 色をリニアにする.
		float vR, vG, vB;
		vR = materialData.diffuseColor[0];
		vG = materialData.diffuseColor[1];
		vB = materialData.diffuseColor[2];
		USD_DATA::convColorLinear(vR, vG, vB);

		shader.CreateInput(TfToken("diffuseColor"), SdfValueTypeNames->Color3f).Set(GfVec3f(vR, vG, vB));
	}

	if (materialData.roughnessTexture.textureParam.imageIndex < 0) {
		shader.CreateInput(TfToken("roughness"), SdfValueTypeNames->Float).Set(materialData.roughness);
	}

	if (materialData.metallicTexture.textureParam.imageIndex < 0) {
		shader.CreateInput(TfToken("metallic"), SdfValueTypeNames->Float).Set(materialData.metallic);
	}

	if (materialData.opacityTexture.textureParam.imageIndex < 0) {
		shader.CreateInput(TfToken("opacity"), SdfValueTypeNames->Float).Set(materialData.opacity);
	}

	if (materialData.alphaModeParam.alphaModeType == CommonParam::alpha_mode_mask) {
		// AlphaModeでのMask指定.
		shader.CreateInput(TfToken("opacityThreshold"), SdfValueTypeNames->Float).Set(materialData.alphaModeParam.alphaCutoff);

	} else if (materialData.alphaModeParam.alphaModeType == CommonParam::alpha_mode_opaque) {
		// 不透明マスクもしくはBaseColorにAlpha要素を持つ場合.
		if (materialData.useDiffuseAlpha) {
			shader.CreateInput(TfToken("opacityThreshold"), SdfValueTypeNames->Float).Set(0.5f);
		}
	}

	// 透過ピクセルがある場合、iorが影響するためior=1.0も出力する必要がある.
	shader.CreateInput(TfToken("ior"), SdfValueTypeNames->Float).Set(materialData.ior);

	if (materialData.emissiveTexture.textureParam.imageIndex < 0) {
		if (!MathUtil::isZero(materialData.emissiveColor[0]) || !MathUtil::isZero(materialData.emissiveColor[1]) || !MathUtil::isZero(materialData.emissiveColor[2])) {
			// 色をリニアにする.
			float vR, vG, vB;
			vR = materialData.emissiveColor[0];
			vG = materialData.emissiveColor[1];
			vB = materialData.emissiveColor[2];
			USD_DATA::convColorLinear(vR, vG, vB);

			shader.CreateInput(TfToken("emissiveColor"), SdfValueTypeNames->Color3f).Set(GfVec3f(vR, vG, vB));
		}
	}

	// DiffuseTexture.
	if (materialData.diffuseTexture.textureParam.imageIndex >= 0) {
		m_outputTextureData(pathStr, materialData, USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_difuseColor, USD_DATA::TEXTURE_SOURE::texture_source_rgb);
	}

	// EmissiveTexture.
	if (materialData.emissiveTexture.textureParam.imageIndex >= 0) {
		m_outputTextureData(pathStr, materialData, USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_emissiveColor, USD_DATA::TEXTURE_SOURE::texture_source_rgb);
	}

	// NormalTexture.
	if (materialData.normalTexture.textureParam.imageIndex >= 0) {
		m_outputTextureData(pathStr, materialData, USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_normal, USD_DATA::TEXTURE_SOURE::texture_source_rgb);
	}

	// RoughnessTexture.
	if (materialData.roughnessTexture.textureParam.imageIndex >= 0) {
		m_outputTextureData(pathStr, materialData, USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_roughness, materialData.roughnessTexture.textureSource);
	}

	// MetallicTexture.
	if (materialData.metallicTexture.textureParam.imageIndex >= 0) {
		m_outputTextureData(pathStr, materialData, USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_metallic, materialData.metallicTexture.textureSource);
	}

	// OcclusionTexture.
	if (materialData.occlusionTexture.textureParam.imageIndex >= 0) {
		m_outputTextureData(pathStr, materialData, USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_occlusion, materialData.occlusionTexture.textureSource);
	}

	// OpacityTexture.
	if (materialData.opacityTexture.textureParam.imageIndex >= 0) {
		m_outputTextureData(pathStr, materialData, USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_opacity, materialData.opacityTexture.textureSource);
	}

	// MaterialからShaderをつなぐ.
	mat.CreateSurfaceOutput().ConnectToSource(shader, TfToken("surface"));
}

/**
 * テクスチャ情報を出力.
 * @param[in] pathStr        USD上のパス (/root/xxx/red).
 * @param[in] materialData  マテリアル情報クラス.
 * @param[in] patternType   テクスチャの種類.
 * @param[in] textureSource テクスチャの参照要素.
 */
void CUSDExporter::m_outputTextureData (const std::string& pathStr, const CMaterialData& materialData, const USD_DATA::TEXTURE_PATTERN_TYPE& patternType, const USD_DATA::TEXTURE_SOURE& textureSource)
{
	UsdPrim primMat = g_stage->GetPrimAtPath(SdfPath(pathStr));
	if (!primMat.IsValid()) return;
	UsdShadeMaterial mat(primMat);

	UsdPrim primShader = g_stage->GetPrimAtPath(SdfPath(pathStr + std::string("/PBRShader")));
	if (!primShader.IsValid()) return;
	UsdShadeShader shader(primShader);

	// マッピングの種類ごとの情報を取得.
	CTextureMappingData mappingD;
	std::string mappingSource = "";
	std::string texName = "";
	std::string connectSource = "";
	switch (patternType) {
	case USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_difuseColor:
		mappingD = materialData.diffuseTexture;
		mappingSource = "rgb";
		texName = "/diffuseTexture";
		connectSource = "diffuseColor";
		break;

	case USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_normal:
		mappingD = materialData.normalTexture;
		mappingSource = "rgb";
		texName = "/normalTexture";
		connectSource = "normal";
		break;

	case USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_emissiveColor:
		mappingD = materialData.emissiveTexture;
		mappingSource = "rgb";
		texName = "/emissiveTexture";
		connectSource = "emissiveColor";
		break;

	case USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_metallic:
		mappingD = materialData.metallicTexture;
		mappingSource = USD_DATA::getTextureSourceString(textureSource);
		if (mappingSource == "rgb") mappingSource = "r";
		texName = "/metallicTexture";
		connectSource = "metallic";
		break;

	case USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_roughness:
		mappingD = materialData.roughnessTexture;
		mappingSource = USD_DATA::getTextureSourceString(textureSource);
		if (mappingSource == "rgb") mappingSource = "r";
		texName = "/roughnessTexture";
		connectSource = "roughness";
		break;

	case USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_opacity:
		mappingD = materialData.opacityTexture;
		mappingSource = USD_DATA::getTextureSourceString(textureSource);
		if (mappingSource == "rgb") mappingSource = "r";
		texName = "/opacityTexture";
		connectSource = "opacity";
		break;

	case USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_occlusion:
		mappingD = materialData.occlusionTexture;
		mappingSource = USD_DATA::getTextureSourceString(textureSource);
		if (mappingSource == "rgb") mappingSource = "r";

		texName = "/occlusionTexture";
		connectSource = "occlusion";
		break;
	}

	if (mappingD.textureParam.imageIndex < 0 || mappingSource == "" || texName == "") return;
	if (m_imagesList.size() <= mappingD.textureParam.imageIndex) return;

	// グレイスケールに変換したテクスチャの場合、"r"を採用.
	const bool convGrayscale = m_imagesList[mappingD.textureParam.imageIndex].texTransform.convGrayscale;
	if (convGrayscale) mappingSource = "r";

	// UVレイヤ番号 (0 or 1).
	const int uvIndex = mappingD.textureParam.uvLayerIndex;

	// UVのReader.
	const std::string stReaderPath = pathStr + std::string((uvIndex == 0) ? "/stReader" : "/stReader2");
	UsdPrim primReader = g_stage->GetPrimAtPath(SdfPath(stReaderPath));
	UsdShadeShader shaderReader;
	if (!primReader.IsValid()) {
		primReader = g_stage->DefinePrim(SdfPath(stReaderPath), TfToken("Shader"));
		shaderReader = UsdShadeShader(primReader);
		shaderReader.CreateIdAttr().Set(TfToken("UsdPrimvarReader_float2"));
	} else {
		shaderReader = UsdShadeShader(primReader);
	}

	// UsdTransform2d.
	// テクスチャの反復指定をUsdTransform2dのscaleで表現.
	bool useTransform2D = false;
	UsdPrim transform2DShader;
	if (mappingD.textureParam.repeatU > 1 || mappingD.textureParam.repeatV > 1) {
		useTransform2D = true;
		const std::string transform2DName = pathStr + std::string(texName) + std::string("_Transform2d");
		transform2DShader = g_stage->DefinePrim(SdfPath(transform2DName), TfToken("Shader"));
		UsdShadeShader tShader = UsdShadeShader(transform2DShader);
		tShader.CreateIdAttr().Set(TfToken("UsdTransform2d"));

		// float2 inputs:inは、Readerに接続.
		tShader.CreateInput(TfToken("in"), SdfValueTypeNames->Token).ConnectToSource(shaderReader, TfToken("result"));

		// 回転/移動スケール/を指定.
		tShader.CreateInput(TfToken("rotation"), SdfValueTypeNames->Float).Set(0.0f);
		tShader.CreateInput(TfToken("translation"), SdfValueTypeNames->Float2).Set(GfVec2f(0.0f, 0.0f));
		const float scaleU = (float)mappingD.textureParam.repeatU;
		const float scaleV = (float)mappingD.textureParam.repeatV;
		tShader.CreateInput(TfToken("scale"), SdfValueTypeNames->Float2).Set(GfVec2f(scaleU, scaleV));
	}

	UsdPrim texShader = g_stage->DefinePrim(SdfPath(pathStr + std::string(texName)), TfToken("Shader"));
	UsdShadeShader shaderTexture(texShader);

	shaderTexture.CreateIdAttr().Set(TfToken("UsdUVTexture"));

	// ファイル名を指定.
	const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
	shaderTexture.CreateInput(TfToken("file"), SdfValueTypeNames->Asset).Set(SdfAssetPath(fileName));

	// UV0の場合は"st"、UV1の場合は"st2"とつなぐ.
	if (useTransform2D) {
		// UsdTransform2dに接続.
		shaderTexture.CreateInput(TfToken((uvIndex == 0) ? "st" : "st2"), SdfValueTypeNames->Float2).ConnectToSource(UsdShadeShader(transform2DShader), TfToken("result"));
	} else {
		// UVのためのReaderと接続.
		shaderTexture.CreateInput(TfToken((uvIndex == 0) ? "st" : "st2"), SdfValueTypeNames->Float2).ConnectToSource(shaderReader, TfToken("result"));
	}

	// wrap指定 (repeatで繰り返し指定).
	const std::string wrapS = mappingD.textureParam.wrapRepeat ? "repeat" : "clamp";
	shaderTexture.CreateInput(TfToken("wrapS"), SdfValueTypeNames->Token).Set(TfToken(wrapS));
	shaderTexture.CreateInput(TfToken("wrapT"), SdfValueTypeNames->Token).Set(TfToken(wrapS));

	// scaleは、テクスチャに対してRGBなどのFactorを乗算したい場合に使用する.
	// input.bias / input.scaleは効かない ? (USD 19.07とiOS12.4.1).
	// iOS13ではinput.scaleが効く.
#if false
	shaderTexture.CreateInput(TfToken("bias"), SdfValueTypeNames->Float4).Set(GfVec4f(0.0f, 0.0f, 0.0f, 0.0f));
	shaderTexture.CreateInput(TfToken("scale"), SdfValueTypeNames->Float4).Set(GfVec4f(1.0f, 1.0f, 1.0f, 1.0f));
#endif

	// Textureをshaderの対象要素にマッピング.
	if (mappingSource == "rgb") {
		if (patternType == USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_normal) {
			shader.CreateInput(TfToken(connectSource), SdfValueTypeNames->Normal3f).ConnectToSource(shaderTexture, TfToken(mappingSource));
		} else {
			shader.CreateInput(TfToken(connectSource), SdfValueTypeNames->Color3f).ConnectToSource(shaderTexture, TfToken(mappingSource));
		}
	} else {
		shader.CreateInput(TfToken(connectSource), SdfValueTypeNames->Float).ConnectToSource(shaderTexture, TfToken(mappingSource));
	}

#if false
	// ColorSpaceを指定.
	if (patternType == USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_difuseColor || patternType == USD_DATA::TEXTURE_PATTERN_TYPE::texture_pattern_type_emissiveColor) {
		shaderTexture.CreateInput(TfToken("sourceColorSpace"), SdfValueTypeNames->Token).Set(TfToken("sRGB"));
	} else {
		shaderTexture.CreateInput(TfToken("sourceColorSpace"), SdfValueTypeNames->Token).Set(TfToken("raw"));
	}
#endif
	
	// UVの接続.
	UsdShadeInput stInput = mat.CreateInput(TfToken((uvIndex == 0) ? "frame:stPrimvarName" : "frame:stPrimvarName2"), SdfValueTypeNames->Token);
	stInput.Set(TfToken((uvIndex == 0) ? "st" : "st2"));

	// Textureに割り当てるUVを接続.
	shaderReader.CreateInput(TfToken("varname"), SdfValueTypeNames->Token).ConnectToSource(stInput);
}

/**
 * 指定のUSDのパスにマテリアル情報を格納 (OmniverseのMDL用).
 * @param[in] pathStr        USD上のパス (/root/xxx/red).
 * @param[in] materialData   マテリアルデータ.
 */
void CUSDExporter::m_appendNodeMaterial_OmniverseMDL (const std::string& pathStr, const CMaterialData& materialData)
{
	// "OmniGlass"として格納.
	if (materialData.useTransparency) {
		m_appendNodeMaterial_OmniverseMDL_Glass(pathStr, materialData);
		return;
	}

	UsdPrim primMat = g_stage->GetPrimAtPath(SdfPath(pathStr));
	if (!primMat.IsValid()) return;

	UsdShadeMaterial mat(primMat);
	const TfToken mdlToken("mdl");

	// OmniPBRの作成.
	UsdPrim primShader = g_stage->DefinePrim(SdfPath(pathStr + std::string("/Shader")), TfToken("Shader"));
	UsdShadeShader shader(primShader);

	shader.SetSourceAsset(SdfAssetPath("OmniPBR.mdl"), mdlToken);
	shader.GetPrim().CreateAttribute(TfToken("info:mdl:sourceAsset:subIdentifier"), SdfValueTypeNames->Token, false, SdfVariabilityUniform).Set(TfToken("OmniPBR")); 

	//-----------------------------------------------.
	// BaseColorの指定.
	//-----------------------------------------------.
	if (materialData.diffuseTexture.textureParam.imageIndex < 0) {
		// 色をリニアにする.
		float vR, vG, vB;
		vR = materialData.diffuseColor[0];
		vG = materialData.diffuseColor[1];
		vB = materialData.diffuseColor[2];
		USD_DATA::convColorLinear(vR, vG, vB);

		UsdShadeInput in = shader.CreateInput(TfToken("diffuse_color_constant"), SdfValueTypeNames->Color3f);
		in.Set(GfVec3f(vR, vG, vB));
		UsdAttribute attr = in.GetAttr();
		attr.SetDisplayGroup(std::string("Albedo"));
		attr.SetDisplayName(std::string("Base Color"));

		// デフォルトの値を指定.
		attr.SetCustomDataByKey(TfToken("default"), VtValue(GfVec3f(0.2f, 0.2f, 0.2f)));
#if USE_DICTIONARY_RANGE
		{
			VtDictionary dic;
			dic.SetValueAtPath("max", VtValue(GfVec3f(100000, 100000, 100000)));
			dic.SetValueAtPath("min", VtValue(GfVec3f(0, 0, 0)));
			attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
		}
#endif

	} else {
		// BaseColorテクスチャを指定.
		const CTextureMappingData& mappingD = materialData.diffuseTexture;

		{
			const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
			UsdShadeInput in = shader.CreateInput(TfToken("diffuse_texture"), SdfValueTypeNames->Asset);
			in.Set(SdfAssetPath(fileName));

			UsdAttribute attr = in.GetAttr();
			attr.SetColorSpace(TfToken("sRGB"));
			attr.SetDisplayGroup(std::string("Albedo"));
			attr.SetDisplayName(std::string("Albedo Map"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(SdfAssetPath("")));
		}

		// Diffuseテクスチャに乗算する色を指定.
		{
			// 色をリニアにする.
			float vR, vG, vB;
			vR = materialData.diffuseColor[0];
			vG = materialData.diffuseColor[1];
			vB = materialData.diffuseColor[2];
			USD_DATA::convColorLinear(vR, vG, vB);

			// Unlitの場合は無視.
			if (materialData.unlitMode) {
				vR = vG = vB = 1.0f;
			}

			UsdShadeInput in = shader.CreateInput(TfToken("diffuse_tint"), SdfValueTypeNames->Color3f);
			in.Set(GfVec3f(vR, vG, vB));
			UsdAttribute attr = in.GetAttr();
			attr.SetDisplayGroup(std::string("Albedo"));
			attr.SetDisplayName(std::string("Color Tint"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(GfVec3f(1, 1, 1)));
#if USE_DICTIONARY_RANGE
			{
				VtDictionary dic;
				dic.SetValueAtPath("max", VtValue(GfVec3f(100000, 100000, 100000)));
				dic.SetValueAtPath("min", VtValue(GfVec3f(0, 0, 0)));
				attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
			}
#endif
		}
	}

	//-----------------------------------------------.
	// Normal mapの指定.
	//-----------------------------------------------.
	if (materialData.normalTexture.textureParam.imageIndex >= 0) {
		const CTextureMappingData& mappingD = materialData.normalTexture;

		{
			const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
			UsdShadeInput in = shader.CreateInput(TfToken("normalmap_texture"), SdfValueTypeNames->Asset);
			in.Set(SdfAssetPath(fileName));

			UsdAttribute attr = in.GetAttr();
			attr.SetColorSpace(TfToken("raw"));
			attr.SetDisplayGroup(std::string("Normal"));
			attr.SetDisplayName(std::string("Normal Map"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(SdfAssetPath("")));
		}

		// Normal Mapの強さを指定.
		{
			UsdShadeInput in = shader.CreateInput(TfToken("bump_factor"), SdfValueTypeNames->Float);
			in.Set(materialData.normalStrength);
			UsdAttribute attr = in.GetAttr();
			attr.SetDisplayGroup(std::string("Normal"));
			attr.SetDisplayName(std::string("Normal Map Strength"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(1.0f));
#if USE_DICTIONARY_RANGE
			{
				VtDictionary dic;
				dic.SetValueAtPath("max", VtValue(100000.0f));
				dic.SetValueAtPath("min", VtValue(-100000.0f));
				attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
			}
#endif
		}
	}

	//-----------------------------------------------.
	// Metallicの指定.
	//-----------------------------------------------.
	{
		const CTextureMappingData& mappingD = materialData.metallicTexture;

		if (mappingD.textureParam.imageIndex < 0) {
			UsdShadeInput in = shader.CreateInput(TfToken("metallic_constant"), SdfValueTypeNames->Float);
			in.Set(materialData.metallic);
			UsdAttribute attr = in.GetAttr();
			attr.SetDisplayGroup(std::string("Reflectivity"));
			attr.SetDisplayName(std::string("Metallic Amount"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(0.0f));
#if USE_DICTIONARY_RANGE
			{
				VtDictionary dic;
				dic.SetValueAtPath("max", VtValue(1.0f));
				dic.SetValueAtPath("min", VtValue(0.0f));
				attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
			}
#endif
		} else {
			{
				const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
				UsdShadeInput in = shader.CreateInput(TfToken("metallic_texture"), SdfValueTypeNames->Asset);
				in.Set(SdfAssetPath(fileName));

				UsdAttribute attr = in.GetAttr();
				if (m_exportParam.grayscaleTexturesColorSpace == USD_DATA::EXPORT::TEXTURE_COLOR_SPACE::texture_colorspace_raw) {
					attr.SetColorSpace(TfToken("raw"));
				} else {
					attr.SetColorSpace(TfToken("sRGB"));
				}

				attr.SetDisplayGroup(std::string("Reflectivity"));
				attr.SetDisplayName(std::string("Metallic Map"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(SdfAssetPath("")));
			}
			{
				UsdShadeInput in = shader.CreateInput(TfToken("metallic_texture_influence"), SdfValueTypeNames->Float);
				in.Set(materialData.metallic);
				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Reflectivity"));
				attr.SetDisplayName(std::string("Metallic Map Influence"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(0.0f));
#if USE_DICTIONARY_RANGE
				{
					VtDictionary dic;
					dic.SetValueAtPath("max", VtValue(1.0f));
					dic.SetValueAtPath("min", VtValue(0.0f));
					attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
				}
#endif
			}
		}
	}

	//-----------------------------------------------.
	// Roughnessの指定.
	//-----------------------------------------------.
	{
		const CTextureMappingData& mappingD = materialData.roughnessTexture;

		if (mappingD.textureParam.imageIndex < 0) {
			UsdShadeInput in = shader.CreateInput(TfToken("reflection_roughness_constant"), SdfValueTypeNames->Float);
			in.Set(materialData.roughness);
			UsdAttribute attr = in.GetAttr();
			attr.SetDisplayGroup(std::string("Reflectivity"));
			attr.SetDisplayName(std::string("Roughness Amount"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(0.5f));
#if USE_DICTIONARY_RANGE
			{
				VtDictionary dic;
				dic.SetValueAtPath("max", VtValue(1.0f));
				dic.SetValueAtPath("min", VtValue(0.0f));
				attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
			}
#endif
		} else {
			{
				const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
				UsdShadeInput in = shader.CreateInput(TfToken("reflectionroughness_texture"), SdfValueTypeNames->Asset);
				in.Set(SdfAssetPath(fileName));

				UsdAttribute attr = in.GetAttr();
				if (m_exportParam.grayscaleTexturesColorSpace == USD_DATA::EXPORT::TEXTURE_COLOR_SPACE::texture_colorspace_raw) {
					attr.SetColorSpace(TfToken("raw"));
				} else {
					attr.SetColorSpace(TfToken("sRGB"));
				}

				attr.SetDisplayGroup(std::string("Reflectivity"));
				attr.SetDisplayName(std::string("Roughness Map"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(SdfAssetPath("")));
			}

			// reflection_roughness_texture_influenceで1.0を指定すると、Roughness Mapの値をそのまま反映.
			{
				UsdShadeInput in = shader.CreateInput(TfToken("reflection_roughness_texture_influence"), SdfValueTypeNames->Float);
				in.Set(1.0f);
				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Reflectivity"));
				attr.SetDisplayName(std::string("Roughness Map Influence"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(0.0f));
#if USE_DICTIONARY_RANGE
				{
					VtDictionary dic;
					dic.SetValueAtPath("max", VtValue(1.0f));
					dic.SetValueAtPath("min", VtValue(0.0f));
					attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
				}
#endif
			}
		}
	}

	//-----------------------------------------------.
	// Occlusionの指定.
	//-----------------------------------------------.
	{
		const CTextureMappingData& mappingD = materialData.occlusionTexture;
		if (mappingD.textureParam.imageIndex >= 0) {
			{
				const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
				UsdShadeInput in = shader.CreateInput(TfToken("ao_texture"), SdfValueTypeNames->Asset);
				in.Set(SdfAssetPath(fileName));

				UsdAttribute attr = in.GetAttr();
				if (m_exportParam.grayscaleTexturesColorSpace == USD_DATA::EXPORT::TEXTURE_COLOR_SPACE::texture_colorspace_raw) {
					attr.SetColorSpace(TfToken("raw"));
				} else {
					attr.SetColorSpace(TfToken("sRGB"));
				}

				attr.SetDisplayGroup(std::string("AO"));
				attr.SetDisplayName(std::string("Ambient Occlusion Map"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(SdfAssetPath("")));
			}
			{
				UsdShadeInput in = shader.CreateInput(TfToken("ao_to_diffuse"), SdfValueTypeNames->Float);
				in.Set(1.0f);
				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("AO"));
				attr.SetDisplayName(std::string("AO to Diffuse"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(0.0f));
#if USE_DICTIONARY_RANGE
				{
					VtDictionary dic;
					dic.SetValueAtPath("max", VtValue(100000.0f));
					dic.SetValueAtPath("min", VtValue(-100000.0f));
					attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
				}
#endif
			}
		}
	}

	//-----------------------------------------------.
	// Opacityの指定.
	//-----------------------------------------------.
	{
		const CTextureMappingData& mappingD = materialData.opacityTexture;
		if (mappingD.textureParam.imageIndex >= 0) {
			{
				UsdShadeInput in = shader.CreateInput(TfToken("enable_opacity"), SdfValueTypeNames->Bool);
				in.Set(true);
				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Opacity"));
				attr.SetDisplayName(std::string("Enable Opacity"));

				attr.SetCustomDataByKey(TfToken("default"), VtValue(false));
			}

			{
				UsdShadeInput in = shader.CreateInput(TfToken("enable_opacity_texture"), SdfValueTypeNames->Bool);
				in.Set(true);
				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Opacity"));
				attr.SetDisplayName(std::string("Enable Opacity Texture"));

				attr.SetCustomDataByKey(TfToken("default"), VtValue(false));
			}

			// Opacityの影響度.
			{
				UsdShadeInput in = shader.CreateInput(TfToken("opacity_constant"), SdfValueTypeNames->Float);
				in.Set(1.0f);
				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Opacity"));
				attr.SetDisplayName(std::string("Opacity Amount"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(1.0f));
#if USE_DICTIONARY_RANGE
				{
					VtDictionary dic;
					dic.SetValueAtPath("max", VtValue(1.0f));
					dic.SetValueAtPath("min", VtValue(0.0f));
					attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
				}
#endif
			}
			
			// Mono Sourceを"mono_alpha"とすると、BaseColorのAlphaをOpacityとすることになる.
			{
				UsdShadeInput in = shader.CreateInput(TfToken("opacity_mode"), SdfValueTypeNames->Int);
				in.Set(materialData.useDiffuseAlpha ? 0 : 1);
				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Opacity"));
				attr.SetDisplayName(std::string("Opacity Mono Source"));

				attr.SetMetadata(TfToken("renderType"), VtValue("::base::mono_mode"));

				{
					NdrTokenMap tMap;
					tMap[TfToken("__SDR__enum_value")].append("mono_average");
					tMap[TfToken("options")].append("mono_alpha:0|mono_average:1|mono_luminance:2|mono_maximum:3");

					in.SetSdrMetadata(tMap);
				}

				attr.SetCustomDataByKey(TfToken("default"), VtValue(1));
			}

			// テクスチャの指定.
			{
				const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
				UsdShadeInput in = shader.CreateInput(TfToken("opacity_texture"), SdfValueTypeNames->Asset);
				in.Set(SdfAssetPath(fileName));

				UsdAttribute attr = in.GetAttr();
				if (m_exportParam.grayscaleTexturesColorSpace == USD_DATA::EXPORT::TEXTURE_COLOR_SPACE::texture_colorspace_raw) {
					attr.SetColorSpace(TfToken("raw"));
				} else {
					attr.SetColorSpace(TfToken("sRGB"));
				}

				attr.SetDisplayGroup(std::string("Opacity"));
				attr.SetDisplayName(std::string("Opacity Map"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(SdfAssetPath("")));
			}

			// Cutout(cutoff)の指定.
			{
				UsdShadeInput in = shader.CreateInput(TfToken("opacity_threshold"), SdfValueTypeNames->Float);
				in.Set(0.0f);
				if (materialData.alphaModeParam.alphaModeType == CommonParam::alpha_mode_type::alpha_mode_mask) {
					in.Set(materialData.alphaModeParam.alphaCutoff);
				}

				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Opacity"));
				attr.SetDisplayName(std::string("Opacity Threshold"));

				attr.SetCustomDataByKey(TfToken("default"), VtValue(0.0f));
#if USE_DICTIONARY_RANGE
				{
					VtDictionary dic;
					dic.SetValueAtPath("max", VtValue(1.0f));
					dic.SetValueAtPath("min", VtValue(0.0f));
					attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
				}
#endif
			}
		}
	}

	//-----------------------------------------------.
	// Emissiveの指定.
	//-----------------------------------------------.
	const float emissiveMinV = 0.001f;
	if (!materialData.unlitMode && (materialData.emissiveColor[0] > emissiveMinV || materialData.emissiveColor[1] > emissiveMinV || materialData.emissiveColor[2] > emissiveMinV)) {
		{
			UsdShadeInput in = shader.CreateInput(TfToken("enable_emission"), SdfValueTypeNames->Bool);
			in.Set(true);
			UsdAttribute attr = in.GetAttr();
			attr.SetDisplayGroup(std::string("Emissive"));
			attr.SetDisplayName(std::string("Emissive Emission"));

			attr.SetCustomDataByKey(TfToken("default"), VtValue(false));
		}

		const CTextureMappingData& mappingD = materialData.emissiveTexture;
		if (mappingD.textureParam.imageIndex >= 0) {
			const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
			UsdShadeInput in = shader.CreateInput(TfToken("emissive_color_texture"), SdfValueTypeNames->Asset);
			in.Set(SdfAssetPath(fileName));

			UsdAttribute attr = in.GetAttr();
			attr.SetColorSpace(TfToken("auto"));
			attr.SetDisplayGroup(std::string("Emissive"));
			attr.SetDisplayName(std::string("Emissive Color map"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(SdfAssetPath("")));

		}
		{
			// 色をリニアにする.
			float vR, vG, vB;
			vR = materialData.emissiveColor[0];
			vG = materialData.emissiveColor[1];
			vB = materialData.emissiveColor[2];
			USD_DATA::convColorLinear(vR, vG, vB);

			UsdShadeInput in = shader.CreateInput(TfToken("emissive_color"), SdfValueTypeNames->Color3f);
			in.Set(GfVec3f(vR, vG, vB));
			UsdAttribute attr = in.GetAttr();
			attr.SetDisplayGroup(std::string("Emissive"));
			attr.SetDisplayName(std::string("Emissive Color"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(GfVec3f(1.0f, 0.1f, 0.1f)));
#if USE_DICTIONARY_RANGE
			{
				VtDictionary dic;
				dic.SetValueAtPath("max", VtValue(GfVec3f(100000, 100000, 100000)));
				dic.SetValueAtPath("min", VtValue(GfVec3f(0, 0, 0)));
				attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
			}
#endif
		}

		{
			UsdShadeInput in = shader.CreateInput(TfToken("emissive_intensity"), SdfValueTypeNames->Float);
			in.Set(materialData.emissiveIntensity * 5000.0f);
			UsdAttribute attr = in.GetAttr();
			attr.SetDisplayGroup(std::string("Emissive"));
			attr.SetDisplayName(std::string("Emissive Intensity"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(40.0f));
#if USE_DICTIONARY_RANGE
			{
				VtDictionary dic;
				dic.SetValueAtPath("max", VtValue(100000.0f));
				dic.SetValueAtPath("min", VtValue(-100000.0f));
				attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
			}
#endif
		}
	}

	// Unlitの場合はBaseColorに格納(Unlitは無効とする).
	if (materialData.unlitMode && (materialData.emissiveColor[0] > emissiveMinV || materialData.emissiveColor[1] > emissiveMinV || materialData.emissiveColor[2] > emissiveMinV)) {
		const CTextureMappingData& mappingD = materialData.emissiveTexture;
		if (mappingD.textureParam.imageIndex >= 0) {
			{
				const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
				UsdShadeInput in = shader.CreateInput(TfToken("diffuse_texture"), SdfValueTypeNames->Asset);
				in.Set(SdfAssetPath(fileName));

				UsdAttribute attr = in.GetAttr();
				attr.SetColorSpace(TfToken("sRGB"));
				attr.SetDisplayGroup(std::string("Albedo"));
				attr.SetDisplayName(std::string("Albedo Map"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(SdfAssetPath("")));
			}
		}
	}

	//-----------------------------------------------.
	// テクスチャの繰り返しの指定.
	// すべてのテクスチャで、同一の繰り返し数の場合のみ反映される.
	//-----------------------------------------------.
	{
		int texRepeatX, texRepeatY;
		texRepeatX = texRepeatY = 0;
		bool sameF = true;
		{
			{
				const CTextureMappingData& mappingD = materialData.diffuseTexture;
				if (mappingD.textureParam.imageIndex >= 0) {
					texRepeatX = mappingD.textureParam.repeatU;
					texRepeatY = mappingD.textureParam.repeatV;
				}
			}
			{
				const CTextureMappingData& mappingD = materialData.normalTexture;
				if (mappingD.textureParam.imageIndex >= 0) {
					if (texRepeatX == 0) {
						texRepeatX = mappingD.textureParam.repeatU;
						texRepeatY = mappingD.textureParam.repeatV;
					} else {
						if (texRepeatX != mappingD.textureParam.repeatU || texRepeatY != mappingD.textureParam.repeatV) {
							sameF = false;
						}
					}
				}
			}
			{
				const CTextureMappingData& mappingD = materialData.metallicTexture;
				if (mappingD.textureParam.imageIndex >= 0) {
					if (texRepeatX == 0) {
						texRepeatX = mappingD.textureParam.repeatU;
						texRepeatY = mappingD.textureParam.repeatV;
					} else {
						if (texRepeatX != mappingD.textureParam.repeatU || texRepeatY != mappingD.textureParam.repeatV) {
							sameF = false;
						}
					}
				}
			}
			{
				const CTextureMappingData& mappingD = materialData.roughnessTexture;
				if (mappingD.textureParam.imageIndex >= 0) {
					if (texRepeatX == 0) {
						texRepeatX = mappingD.textureParam.repeatU;
						texRepeatY = mappingD.textureParam.repeatV;
					} else {
						if (texRepeatX != mappingD.textureParam.repeatU || texRepeatY != mappingD.textureParam.repeatV) {
							sameF = false;
						}
					}
				}
			}
			{
				const CTextureMappingData& mappingD = materialData.occlusionTexture;
				if (mappingD.textureParam.imageIndex >= 0) {
					if (texRepeatX == 0) {
						texRepeatX = mappingD.textureParam.repeatU;
						texRepeatY = mappingD.textureParam.repeatV;
					} else {
						if (texRepeatX != mappingD.textureParam.repeatU || texRepeatY != mappingD.textureParam.repeatV) {
							sameF = false;
						}
					}
				}
			}
			{
				const CTextureMappingData& mappingD = materialData.opacityTexture;
				if (mappingD.textureParam.imageIndex >= 0) {
					if (texRepeatX == 0) {
						texRepeatX = mappingD.textureParam.repeatU;
						texRepeatY = mappingD.textureParam.repeatV;
					} else {
						if (texRepeatX != mappingD.textureParam.repeatU || texRepeatY != mappingD.textureParam.repeatV) {
							sameF = false;
						}
					}
				}
			}
			{
				const CTextureMappingData& mappingD = materialData.emissiveTexture;
				if (mappingD.textureParam.imageIndex >= 0) {
					if (texRepeatX == 0) {
						texRepeatX = mappingD.textureParam.repeatU;
						texRepeatY = mappingD.textureParam.repeatV;
					} else {
						if (texRepeatX != mappingD.textureParam.repeatU || texRepeatY != mappingD.textureParam.repeatV) {
							sameF = false;
						}
					}
				}
			}

			if (!sameF || texRepeatX == 0) {
				texRepeatX = texRepeatY = 1;
			}

			if (texRepeatX != 1 || texRepeatY != 1) {
				UsdShadeInput in = shader.CreateInput(TfToken("texture_scale"), SdfValueTypeNames->Float2);
				in.Set(GfVec2f(texRepeatX, texRepeatY));

				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("UV"));
				attr.SetDisplayName(std::string("Texture Scale"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(GfVec2f(1, 1)));
#if USE_DICTIONARY_RANGE
				{
					VtDictionary dic;
					dic.SetValueAtPath("max", VtValue(GfVec2f(100000, 100000)));
					dic.SetValueAtPath("min", VtValue(GfVec2f(-100000, -100000)));
					attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
				}
#endif
			}
		}
	}

	// MaterialからShaderをつなぐ.
	UsdShadeOutput mdlOutput = mat.CreateSurfaceOutput(mdlToken);
	mdlOutput.ConnectToSource(shader, TfToken("out"));
}

/**
 * 指定のUSDのパスにマテリアル情報を格納 (OmniverseのMDL用(Glass)).
 * @param[in] pathStr        USD上のパス (/root/xxx/red).
 * @param[in] materialData   マテリアルデータ.
 */
void CUSDExporter::m_appendNodeMaterial_OmniverseMDL_Glass (const std::string& pathStr, const CMaterialData& materialData)
{
	UsdPrim primMat = g_stage->GetPrimAtPath(SdfPath(pathStr));
	if (!primMat.IsValid()) return;

	UsdShadeMaterial mat(primMat);
	const TfToken mdlToken("mdl");

	// OmniPBRの作成.
	UsdPrim primShader = g_stage->DefinePrim(SdfPath(pathStr + std::string("/Shader")), TfToken("Shader"));
	UsdShadeShader shader(primShader);

	shader.SetSourceAsset(SdfAssetPath("OmniGlass.mdl"), mdlToken);
	shader.GetPrim().CreateAttribute(TfToken("info:mdl:sourceAsset:subIdentifier"), SdfValueTypeNames->Token, false, SdfVariabilityUniform).Set(TfToken("OmniGlass")); 

	//-----------------------------------------------.
	// Depth.
	//-----------------------------------------------.
	{
		UsdShadeInput in = shader.CreateInput(TfToken("depth"), SdfValueTypeNames->Float);
		in.Set(0.001f);
		UsdAttribute attr = in.GetAttr();
		attr.SetDisplayGroup(std::string("Color"));
		attr.SetDisplayName(std::string("Volume Absorption Scale"));

		// デフォルトの値を指定.
		attr.SetCustomDataByKey(TfToken("default"), VtValue(0.001f));
#if USE_DICTIONARY_RANGE
		{
			VtDictionary dic;
			dic.SetValueAtPath("max", VtValue(1000.0f));
			dic.SetValueAtPath("min", VtValue(0.0f));
			attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
		}
#endif
	}

	//-----------------------------------------------.
	// Roughness.
	//-----------------------------------------------.
	{
		const CTextureMappingData& mappingD = materialData.roughnessTexture;
		{
			UsdShadeInput in = shader.CreateInput(TfToken("frosting_roughness"), SdfValueTypeNames->Float);
			in.Set(materialData.roughness);
			UsdAttribute attr = in.GetAttr();
			attr.SetDisplayGroup(std::string("Roughness"));
			attr.SetDisplayName(std::string("Glass Roughness"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(0.0f));
#if USE_DICTIONARY_RANGE
			{
				VtDictionary dic;
				dic.SetValueAtPath("max", VtValue(1.0f));
				dic.SetValueAtPath("min", VtValue(0.0f));
				attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
			}
#endif
		}

		if (mappingD.textureParam.imageIndex >= 0) {
			{
				const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
				UsdShadeInput in = shader.CreateInput(TfToken("roughness_texture"), SdfValueTypeNames->Asset);
				in.Set(SdfAssetPath(fileName));

				UsdAttribute attr = in.GetAttr();
				if (m_exportParam.grayscaleTexturesColorSpace == USD_DATA::EXPORT::TEXTURE_COLOR_SPACE::texture_colorspace_raw) {
					attr.SetColorSpace(TfToken("raw"));
				} else {
					attr.SetColorSpace(TfToken("sRGB"));
				}
				attr.SetDisplayGroup(std::string("Roughness"));
				attr.SetDisplayName(std::string("Roughness Texture"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(SdfAssetPath("")));
			}
			{
				UsdShadeInput in = shader.CreateInput(TfToken("roughness_texture_influence"), SdfValueTypeNames->Float);
				in.Set(1.0f);
				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Roughness"));
				attr.SetDisplayName(std::string("Roughness Texture Influence"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(1.0f));
#if USE_DICTIONARY_RANGE
				{
					VtDictionary dic;
					dic.SetValueAtPath("max", VtValue(100000.f));
					dic.SetValueAtPath("min", VtValue(-100000.f));
					attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
				}
#endif
			}
		}
	}

	//-----------------------------------------------.
	// Normal.
	//-----------------------------------------------.
	if (materialData.normalTexture.textureParam.imageIndex >= 0) {
		const CTextureMappingData& mappingD = materialData.normalTexture;

		{
			const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
			UsdShadeInput in = shader.CreateInput(TfToken("normal_map_texture"), SdfValueTypeNames->Asset);
			in.Set(SdfAssetPath(fileName));

			UsdAttribute attr = in.GetAttr();
			attr.SetColorSpace(TfToken("raw"));
			attr.SetDisplayGroup(std::string("Normal"));
			attr.SetDisplayName(std::string("Normal Map Texture"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(SdfAssetPath("")));
		}

		// Normal Mapの強さを指定.
		{
			UsdShadeInput in = shader.CreateInput(TfToken("normal_map_strength"), SdfValueTypeNames->Float);
			in.Set(materialData.normalStrength);
			UsdAttribute attr = in.GetAttr();
			attr.SetDisplayGroup(std::string("Normal"));
			attr.SetDisplayName(std::string("Normal Map Strength"));

			// デフォルトの値を指定.
			attr.SetCustomDataByKey(TfToken("default"), VtValue(1.0f));
#if USE_DICTIONARY_RANGE
			{
				VtDictionary dic;
				dic.SetValueAtPath("max", VtValue(10.0f));
				dic.SetValueAtPath("min", VtValue(0.0f));
				attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
			}
#endif
		}
	}

	//-----------------------------------------------.
	// Glass Color.
	//-----------------------------------------------.
	{
		// 色をリニアにする.
		float vR, vG, vB;
		vR = materialData.transparencyColor[0];
		vG = materialData.transparencyColor[1];
		vB = materialData.transparencyColor[2];
		USD_DATA::convColorLinear(vR, vG, vB);

		UsdShadeInput in = shader.CreateInput(TfToken("glass_color"), SdfValueTypeNames->Color3f);
		in.Set(GfVec3f(vR, vG, vB));
		UsdAttribute attr = in.GetAttr();
		attr.SetDisplayGroup(std::string("Color"));
		attr.SetDisplayName(std::string("Glass Color"));

		// デフォルトの値を指定.
		attr.SetCustomDataByKey(TfToken("default"), VtValue(GfVec3f(1, 1, 1)));
#if USE_DICTIONARY_RANGE
		{
			VtDictionary dic;
			dic.SetValueAtPath("max", VtValue(GfVec3f(1, 1, 1)));
			dic.SetValueAtPath("min", VtValue(GfVec3f(0, 0, 0)));
			attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
		}
#endif
	}

	//-----------------------------------------------.
	// Glass IOR.
	//-----------------------------------------------.
	{
		UsdShadeInput in = shader.CreateInput(TfToken("glass_ior"), SdfValueTypeNames->Float);
		in.Set(materialData.ior);
		UsdAttribute attr = in.GetAttr();
		attr.SetDisplayGroup(std::string("Refraction"));
		attr.SetDisplayName(std::string("Glass IOR"));

		// デフォルトの値を指定.
		attr.SetCustomDataByKey(TfToken("default"), VtValue(1.491f));
#if USE_DICTIONARY_RANGE
		{
			VtDictionary dic;
			dic.SetValueAtPath("max", VtValue(4.0f));
			dic.SetValueAtPath("min", VtValue(1.0f));
			attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
		}
#endif
	}

	//-----------------------------------------------.
	// Opacity.
	//-----------------------------------------------.
	{
		const CTextureMappingData& mappingD = materialData.opacityTexture;
		if (mappingD.textureParam.imageIndex >= 0) {
			{
				UsdShadeInput in = shader.CreateInput(TfToken("enable_opacity"), SdfValueTypeNames->Bool);
				in.Set(true);
				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Opacity"));
				attr.SetDisplayName(std::string("Enable Opacity"));

				attr.SetCustomDataByKey(TfToken("default"), VtValue(false));
			}

			// Opacityの影響度.
			{
				UsdShadeInput in = shader.CreateInput(TfToken("cutout_opacity"), SdfValueTypeNames->Float);
				in.Set(1.0f);
				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Opacity"));
				attr.SetDisplayName(std::string("Opacity Amount"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(1.0f));
#if USE_DICTIONARY_RANGE
				{
					VtDictionary dic;
					dic.SetValueAtPath("max", VtValue(1.0f));
					dic.SetValueAtPath("min", VtValue(0.0f));
					attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
				}
#endif
			}
			
			// Mono Sourceを"mono_alpha"とすると、BaseColorのAlphaをOpacityとすることになる.
			{
				UsdShadeInput in = shader.CreateInput(TfToken("cutout_opacity_mono_source"), SdfValueTypeNames->Int);
				in.Set(materialData.useDiffuseAlpha ? 0 : 1);
				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Opacity"));
				attr.SetDisplayName(std::string("Opacity Mono Source"));

				attr.SetMetadata(TfToken("renderType"), VtValue("::base::mono_mode"));

				{
					NdrTokenMap tMap;
					tMap[TfToken("__SDR__enum_value")].append("mono_alpha");
					tMap[TfToken("options")].append("mono_alpha:0|mono_average:1|mono_luminance:2|mono_maximum:3");

					in.SetSdrMetadata(tMap);
				}

				attr.SetCustomDataByKey(TfToken("default"), VtValue(0));
			}

			// テクスチャの指定.
			{
				const std::string fileName = m_imagesList[mappingD.textureParam.imageIndex].fileName;
				UsdShadeInput in = shader.CreateInput(TfToken("cutout_opacity_texture"), SdfValueTypeNames->Asset);
				in.Set(SdfAssetPath(fileName));

				UsdAttribute attr = in.GetAttr();
				if (m_exportParam.grayscaleTexturesColorSpace == USD_DATA::EXPORT::TEXTURE_COLOR_SPACE::texture_colorspace_raw) {
					attr.SetColorSpace(TfToken("raw"));
				} else {
					attr.SetColorSpace(TfToken("sRGB"));
				}

				attr.SetDisplayGroup(std::string("Opacity"));
				attr.SetDisplayName(std::string("Opacity Map"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(SdfAssetPath("")));
			}

			// Cutout(cutoff)の指定.
			{
				UsdShadeInput in = shader.CreateInput(TfToken("opacity_threshold"), SdfValueTypeNames->Float);
				in.Set(0.0f);
				if (materialData.alphaModeParam.alphaModeType == CommonParam::alpha_mode_type::alpha_mode_mask) {
					in.Set(materialData.alphaModeParam.alphaCutoff);
				}

				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("Opacity"));
				attr.SetDisplayName(std::string("Opacity Threshold"));

				attr.SetCustomDataByKey(TfToken("default"), VtValue(0.0f));
#if USE_DICTIONARY_RANGE
				{
					VtDictionary dic;
					dic.SetValueAtPath("max", VtValue(1.0f));
					dic.SetValueAtPath("min", VtValue(0.0f));
					attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
				}
#endif
			}
		}
	}

	//-----------------------------------------------.
	// thin.
	//-----------------------------------------------.
	{
		UsdShadeInput in = shader.CreateInput(TfToken("thin_walled"), SdfValueTypeNames->Bool);
		in.Set(materialData.glassThin);
		UsdAttribute attr = in.GetAttr();
		attr.SetDisplayGroup(std::string("Refraction"));
		attr.SetDisplayName(std::string("Thin Walled"));

		attr.SetCustomDataByKey(TfToken("default"), VtValue(false));
	}

	//-----------------------------------------------.
	// テクスチャの繰り返しの指定.
	// すべてのテクスチャで、同一の繰り返し数の場合のみ反映される.
	//-----------------------------------------------.
	{
		int texRepeatX, texRepeatY;
		texRepeatX = texRepeatY = 0;
		bool sameF = true;
		{
			{
				const CTextureMappingData& mappingD = materialData.normalTexture;
				if (mappingD.textureParam.imageIndex >= 0) {
					if (texRepeatX == 0) {
						texRepeatX = mappingD.textureParam.repeatU;
						texRepeatY = mappingD.textureParam.repeatV;
					} else {
						if (texRepeatX != mappingD.textureParam.repeatU || texRepeatY != mappingD.textureParam.repeatV) {
							sameF = false;
						}
					}
				}
			}
			{
				const CTextureMappingData& mappingD = materialData.roughnessTexture;
				if (mappingD.textureParam.imageIndex >= 0) {
					if (texRepeatX == 0) {
						texRepeatX = mappingD.textureParam.repeatU;
						texRepeatY = mappingD.textureParam.repeatV;
					} else {
						if (texRepeatX != mappingD.textureParam.repeatU || texRepeatY != mappingD.textureParam.repeatV) {
							sameF = false;
						}
					}
				}
			}
			{
				const CTextureMappingData& mappingD = materialData.opacityTexture;
				if (mappingD.textureParam.imageIndex >= 0) {
					if (texRepeatX == 0) {
						texRepeatX = mappingD.textureParam.repeatU;
						texRepeatY = mappingD.textureParam.repeatV;
					} else {
						if (texRepeatX != mappingD.textureParam.repeatU || texRepeatY != mappingD.textureParam.repeatV) {
							sameF = false;
						}
					}
				}
			}

			if (!sameF || texRepeatX == 0) {
				texRepeatX = texRepeatY = 1;
			}

			if (texRepeatX != 1 || texRepeatY != 1) {
				UsdShadeInput in = shader.CreateInput(TfToken("texture_scale"), SdfValueTypeNames->Float2);
				in.Set(GfVec2f(texRepeatX, texRepeatY));

				UsdAttribute attr = in.GetAttr();
				attr.SetDisplayGroup(std::string("UV"));
				attr.SetDisplayName(std::string("Texture Scale"));

				// デフォルトの値を指定.
				attr.SetCustomDataByKey(TfToken("default"), VtValue(GfVec2f(1, 1)));
#if USE_DICTIONARY_RANGE
				{
					VtDictionary dic;
					dic.SetValueAtPath("max", VtValue(GfVec2f(100000, 100000)));
					dic.SetValueAtPath("min", VtValue(GfVec2f(-100000, -100000)));
					attr.SetCustomDataByKey(TfToken("range"), VtValue(dic));
				}
#endif
			}
		}
	}

	// MaterialからShaderをつなぐ.
	UsdShadeOutput mdlOutput = mat.CreateSurfaceOutput(mdlToken);
	mdlOutput.ConnectToSource(shader, TfToken("out"));
}

/**
 * NULLノードを出力.
 * @param[in] nodeName  ノード名 (/root/xxx/mesh1 などのパス形式).
 * @param[in] matrix    変換要素.
 * @param[in] jointMotion  ノードでモーション情報を持つ場合のモーション情報.
 */
void CUSDExporter::appendNodeNull (const std::string& nodeName, const USD_DATA::NodeMatrixData& matrix, const CJointMotionData& jointMotion)
{
	if (!g_stage) return;

	std::string nodePath = nodeName;
	UsdPrim node = g_stage->DefinePrim(SdfPath(nodePath), TfToken("Xform"));

	// 変換行列を指定.
	if (!jointMotion.hasMotion()) {		// モーション情報を持たない場合.
		::m_setMatrix(node, matrix);
	}

	// モーション情報を指定.
	m_setTransformAnimation(nodeName, jointMotion);
}

/**
 * 参照を指定.
 * @param[in] nodeName           ノード名 (/root/xxx/mesh1 などのパス形式).
 * @param[in] refNodeName        参照するノード名 (/root/xxx/mesh1 などのパス形式).
 * @param[in] refMaterialName    参照するマテリアル名 (/root/materials/xxx1 などのパス形式).
 */
void CUSDExporter::setShapeReference (const std::string& nodeName, const std::string& refNodeName, const std::string& refMaterialName)
{
	if (!g_stage) return;

	int iPos = nodeName.find_last_of("/");
	if (iPos == std::string::npos) return;

	const std::string sPath = nodeName.substr(0, iPos);
	const std::string sName = nodeName.substr(iPos + 1);

	std::string rName = sName;
	iPos = refNodeName.find_last_of("/");
	if (iPos != std::string::npos) {
		rName = refNodeName.substr(iPos + 1);
	}

	UsdPrim node = g_stage->GetPrimAtPath(SdfPath(sPath));
	if (!node.IsValid()) return;

	std::string nodePath = sPath + std::string("/ref_") + rName;
	UsdPrim nodeRef = g_stage->OverridePrim(SdfPath(nodePath));
	nodeRef.GetReferences().AddInternalReference(SdfPath(refNodeName));

	// マテリアルを割り当て.
	if (refMaterialName != "") {
		UsdPrim primMat = g_stage->GetPrimAtPath(SdfPath(refMaterialName));
		if (primMat.IsValid()) {
			UsdShadeMaterial mat(primMat);
			UsdShadeMaterialBindingAPI(nodeRef).Bind(mat);
		}
	}
}

/**
 * ノードに対してモーション情報(transform animation)を格納.
 */
void CUSDExporter::m_setTransformAnimation (const std::string& nodeName, const CJointMotionData& jointMotion)
{
	if (!g_stage) return;
	if (!jointMotion.hasMotion()) return;

	UsdPrim node = g_stage->GetPrimAtPath(SdfPath(nodeName));
	if (!node.IsValid()) return;

	// 移動アニメーションを指定.
	if (!jointMotion.translations.empty()) {
		UsdGeomXformOp transOp = UsdGeomXform(node).AddTranslateOp(UsdGeomXformOp::PrecisionFloat);
		for (size_t i = 0; i < jointMotion.translations.size(); ++i) {
			const CJointTranslationData& transD = jointMotion.translations[i];
			transOp.Set(GfVec3f(transD.x, transD.y, transD.z), transD.frame);
		}
	}

	// 回転アニメーションを指定.
	if (!jointMotion.rotations.empty()) {
#if false
		// iOS 12.4.1では以下は効かず。iOS 13では効く.
		// クォータニオンで渡す.
		UsdGeomXformOp transOp = UsdGeomXform(node).AddOrientOp(UsdGeomXformOp::PrecisionFloat);

		// 第一引数は値、第二引数はフレーム位置.
		for (size_t i = 0; i < jointMotion.rotations.size(); ++i) {
			const CJointRotationData& rotD = jointMotion.rotations[i];
			transOp.Set(GfQuatf(rotD.w, rotD.x, rotD.y, rotD.z), rotD.frame);
		}
#else
		// Eulerの回転角度で渡す.
		UsdGeomXformOp transOp = UsdGeomXform(node).AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat);

		// 第一引数は値、第二引数はフレーム位置.
		for (size_t i = 0; i < jointMotion.rotations.size(); ++i) {
			const CJointRotationData& rotD = jointMotion.rotations[i];
			transOp.Set(GfVec3f(rotD.eulerX, rotD.eulerY, rotD.eulerZ), rotD.frame);
		}

#endif

	}

	// スケールアニメーションを指定.
	if (!jointMotion.scales.empty()) {
		UsdGeomXformOp transOp = UsdGeomXform(node).AddScaleOp(UsdGeomXformOp::PrecisionFloat);

		// 第一引数は値、第二引数はフレーム位置.
		for (size_t i = 0; i < jointMotion.scales.size(); ++i) {
			const CJointScaleData& scaleD = jointMotion.scales[i];
			transOp.Set(GfVec3f(scaleD.x, scaleD.y, scaleD.z), scaleD.frame);
		}
	}
}

/**
 * 指定のメッシュがスキンを持つか.
 */
bool CUSDExporter::m_hasSkinMesh (const USD_DATA::MeshData& meshData)
{
	if (meshData.skinSkeletonIndex < 0 || m_skeletonsList.empty() || meshData.skinWeights.empty()) return false;

	const size_t versCou  = meshData.vertices.size() / 3;
	const std::vector< std::vector<float> >& skinWeights = meshData.skinWeights;
	if (skinWeights.size() != versCou) return false;
	return true;
}

/**
 * Meshノードを出力.
 * @param[in] nodeName    ノード名 (/root/xxx/mesh1 などのパス形式).
 * @param[in] matrix      変換要素.
 * @param[in] meshData    メッシュ情報.
 * @param[in] doubleSided 両面表示するか.
 */
void CUSDExporter::appendNodeMesh (const std::string& nodeName, const USD_DATA::NodeMatrixData& matrix, const USD_DATA::MeshData& meshData, const bool doubleSided)
{
	if (!g_stage) return;

	// スキン使用時は、Skeleton内にMeshを入れる必要がある.
	std::string meshPath = nodeName;
	if (m_hasSkinMesh(meshData)) {
		const CSkeletonData& skelD = m_skeletonsList[meshData.skinSkeletonIndex];

		std::string meshName = nodeName;
		const int iPos = meshName.find_last_of("/");
		if (iPos != std::string::npos) {
			if (meshData.faceGroupMesh) {		// face groupのメッシュの場合、その1つ親もパスに入れる.
				std::string parentPath = "";
				const std::string name2 = meshName.substr(iPos + 1);
				meshName = meshName.substr(0, iPos);
				const int iPos2 = meshName.find_last_of("/");
				if (iPos2 != std::string::npos) {
					const std::string parentName = meshName.substr(iPos2 + 1);
					meshName = parentName + std::string("/") + name2;
					parentPath = std::string(SKELETONS_ROOT_PATH) + std::string("/") + skelD.rootName + std::string("/") + parentName;
				}

				// 親のノードがない場合は作成.
				if (parentPath != "") {
					UsdPrim parentPrim = g_stage->GetPrimAtPath(SdfPath(parentPath));
					if (!parentPrim.IsValid()) {
						g_stage->DefinePrim(SdfPath(parentPath), TfToken("Xform"));
					}
				}

			} else {
				meshName = meshName.substr(iPos + 1);
			}
		}
		meshPath = std::string(SKELETONS_ROOT_PATH) + std::string("/") + skelD.rootName + std::string("/") + meshName;
	}

	UsdPrim prim = g_stage->DefinePrim(SdfPath(meshPath), TfToken("Mesh"));
	UsdGeomMesh geomMesh(prim);

	// 変換行列を指定.
	::m_setMatrix(prim, matrix);

	const size_t versCou  = meshData.vertices.size() / 3;
	const size_t facesCou = meshData.faceVertexCounts.size();

	// Meshの頂点を格納.
	if (versCou >= 1) {
		std::vector<GfVec3f> vList;
		vList.resize(versCou);
		for (size_t i = 0, iPos = 0; i < versCou; ++i, iPos += 3) {
			vList[i].Set(meshData.vertices[iPos + 0], meshData.vertices[iPos + 1], meshData.vertices[iPos + 2]);
		}

		UsdAttribute attr = geomMesh.CreatePointsAttr();
		VtVec3fArray ar = VtVec3fArray(vList.begin(), vList.end());
		attr.Set(ar);
	}

	// 頂点の法線を格納.
	if (!meshData.normals.empty()) {
		std::vector<GfVec3f> vList;
		vList.resize(versCou);
		for (size_t i = 0, iPos = 0; i < versCou; ++i, iPos += 3) {
			vList[i].Set(meshData.normals[iPos + 0], meshData.normals[iPos + 1], meshData.normals[iPos + 2]);
		}

		UsdAttribute attr = geomMesh.CreateNormalsAttr();
		VtVec3fArray ar = VtVec3fArray(vList.begin(), vList.end());
		attr.Set(ar);
	}

	// 頂点カラーを格納.
	if (!meshData.color0.empty()) {
		std::vector<GfVec3f> vList;
		vList.resize(versCou);
		for (size_t i = 0, iPos = 0; i < versCou; ++i, iPos += 3) {
			vList[i].Set(meshData.color0[iPos + 0], meshData.color0[iPos + 1], meshData.color0[iPos + 2]);
		}

		UsdGeomPrimvar primV = geomMesh.CreatePrimvar(TfToken("colors"), SdfValueTypeNames->Color3fArray, UsdGeomTokens->vertex);
		UsdAttribute attr = primV.GetAttr();
		attr.Set(VtVec3fArray(vList.begin(), vList.end()));
	}

	// 面情報を格納.
	if (facesCou > 0) {
		{
			UsdAttribute attr = geomMesh.CreateFaceVertexCountsAttr();
			VtIntArray ar = VtIntArray(meshData.faceVertexCounts.begin(), meshData.faceVertexCounts.end());
			attr.Set(ar);
		}
		{
			UsdAttribute attr = geomMesh.CreateFaceVertexIndicesAttr();
			VtIntArray ar = VtIntArray(meshData.faceIndices.begin(), meshData.faceIndices.end());
			attr.Set(ar);
		}
	}

	// UV0を格納.
	if (!meshData.faceUV0.empty()) {
		// 面ごとにUVを格納.
		{
			std::vector<GfVec2f> uvList;
			for (size_t i = 0, iPos = 0; i < facesCou; ++i) {
				const int fvCou = meshData.faceVertexCounts[i];
				for (int j = 0; j < fvCou; ++j) {
					uvList.push_back(GfVec2f(meshData.faceUV0[iPos + 0], meshData.faceUV0[iPos + 1]));
					iPos += 2;
				}
			}
			UsdGeomPrimvar primV = geomMesh.CreatePrimvar(TfToken("st"), SdfValueTypeNames->TexCoord2fArray, UsdGeomTokens->faceVarying);
			UsdAttribute attr = primV.GetAttr();
			attr.Set(VtVec2fArray(uvList.begin(), uvList.end()));
		}

		// UVでの面のインデックスを格納.
		{
			std::vector<int> uvFaceIndexList;
			for (size_t i = 0, iPos = 0; i < facesCou; ++i) {
				const int fvCou = meshData.faceVertexCounts[i];
				for (int j = 0; j < fvCou; ++j) {
					uvFaceIndexList.push_back(iPos);
					iPos++;
				}
			}
			UsdGeomPrimvar primV = geomMesh.GetPrimvar(TfToken("st"));
			if (primV) {
				UsdAttribute attr = primV.CreateIndicesAttr();
				attr.Set(VtIntArray(uvFaceIndexList.begin(), uvFaceIndexList.end()));
			}
		}
	}

	// UV1を格納.
	if (!meshData.faceUV1.empty()) {
		// 面ごとにUVを格納.
		{
			std::vector<GfVec2f> uvList;
			for (size_t i = 0, iPos = 0; i < facesCou; ++i) {
				const int fvCou = meshData.faceVertexCounts[i];
				for (int j = 0; j < fvCou; ++j) {
					uvList.push_back(GfVec2f(meshData.faceUV1[iPos + 0], meshData.faceUV1[iPos + 1]));
					iPos += 2;
				}
			}
			UsdGeomPrimvar primV = geomMesh.CreatePrimvar(TfToken("st2"), SdfValueTypeNames->TexCoord2fArray, UsdGeomTokens->faceVarying);
			UsdAttribute attr = primV.GetAttr();
			attr.Set(VtVec2fArray(uvList.begin(), uvList.end()));
		}

		// UVでの面のインデックスを格納.
		{
			std::vector<int> uvFaceIndexList;
			for (size_t i = 0, iPos = 0; i < facesCou; ++i) {
				const int fvCou = meshData.faceVertexCounts[i];
				for (int j = 0; j < fvCou; ++j) {
					uvFaceIndexList.push_back(iPos);
					iPos++;
				}
			}
			UsdGeomPrimvar primV = geomMesh.GetPrimvar(TfToken("st2"));
			if (primV) {
				UsdAttribute attr = primV.CreateIndicesAttr();
				attr.Set(VtIntArray(uvFaceIndexList.begin(), uvFaceIndexList.end()));
			}
		}
	}

	// マテリアルの参照を追加.
	if (meshData.refMaterialName != "") {
		UsdPrim primMat = g_stage->GetPrimAtPath(SdfPath(meshData.refMaterialName));
		if (primMat.IsValid()) {
			UsdShadeMaterial mat(primMat);

			// USD v.21.02ではUsdShadeMaterialBindingAPIを使用する.
			//mat.Bind(prim);
			UsdShadeMaterialBindingAPI(prim).Bind(mat);
		}

		// doubleSidedの指定.
		if (doubleSided) {
			geomMesh.CreateDoubleSidedAttr(VtValue(true));
		}
	}

	// Subdivision情報を格納.
	{
		UsdAttribute attr = geomMesh.CreateSubdivisionSchemeAttr();
		attr.Set(TfToken(meshData.subdivision ? "catmullClark" : "none"));
	}

	// スキン情報を格納.
	if (m_hasSkinMesh(meshData)) {
		const std::vector< std::vector<float> >& skinWeights = meshData.skinWeights;

		// ウエイト値を格納.
		{
			std::vector<float> weights;
			weights.resize(versCou * 4);
			for (size_t i = 0, iPos = 0; i < versCou; ++i, iPos += 4) {
				for (int j = 0; j < 4; ++j) {
					weights[iPos + j] = skinWeights[i][j];
				}
			}

			// 頂点ごとに4つのジョイントを割り振れる指定.
			UsdGeomPrimvar primV = geomMesh.CreatePrimvar(TfToken("skel:jointWeights"), SdfValueTypeNames->FloatArray, TfToken("vertex"), 4);
			UsdAttribute attr = primV.GetAttr();
			attr.Set(VtFloatArray(weights.begin(), weights.end()));
		}

		// ジョイントインデックスを格納.
		{
			std::vector<int> jointIndices;
			jointIndices.resize(versCou * 4);
			for (size_t i = 0, iPos = 0; i < versCou; ++i, iPos += 4) {
				for (int j = 0; j < 4; ++j) {
					jointIndices[iPos + j] = meshData.skinJoints[i][j];
					if (jointIndices[iPos + j] < 0) jointIndices[iPos + j] = 0;
				}
			}

			// 頂点ごとに4つのジョイントを割り振れる指定.
			UsdGeomPrimvar primV = geomMesh.CreatePrimvar(TfToken("skel:jointIndices"), SdfValueTypeNames->IntArray, TfToken("vertex"), 4);
			UsdAttribute attr = primV.GetAttr();
			attr.Set(VtIntArray(jointIndices.begin(), jointIndices.end()));
		}

		// スケルトンの割り当て.
		{
			// MeshからSkelAnimationへの参照.
			// USDの仕様ではいらないが、iOS12.4では以下がないとスキンアニメーションが動作しない。.
			// iOS13ではこれが不要になった.
			const CSkeletonData& skelD = m_skeletonsList[meshData.skinSkeletonIndex];
			{
				const std::string skelPath = std::string(SKELETONS_ROOT_PATH) + std::string("/") + skelD.rootName + std::string("/Skel/Anim");
				UsdPrim prim = g_stage->GetPrimAtPath(SdfPath(skelPath));
				if (prim.IsValid()) {
					UsdSkelBindingAPI binding = UsdSkelBindingAPI::Apply(geomMesh.GetPrim());
					binding.CreateAnimationSourceRel().SetTargets(SdfPathVector({prim.GetPath()}));
				}
			}

			// SkeletonからMeshへのバインド.
			{
				const std::string skelPath = std::string(SKELETONS_ROOT_PATH) + std::string("/") + skelD.rootName + std::string("/Skel");
				UsdPrim prim = g_stage->GetPrimAtPath(SdfPath(skelPath));
				if (prim.IsValid()) {
					UsdSkelBindingAPI binding = UsdSkelBindingAPI::Apply(geomMesh.GetPrim());
					binding.CreateSkeletonRel().SetTargets(SdfPathVector({prim.GetPath()}));
				}
			}
		}
	}
}

/**
 * 主にSubdivisionの指定 (Omniverseにパラメータがある).
 * @param[in] nodeName        ノード名 (/root/xxx/mesh1 などのパス形式).
 * @param[in] enableOverride   refinementEnableOverrideの指定.
 * @param[in] refinementLevel  refinementLevelの指定.
 */
void CUSDExporter::setRefinement (const std::string& nodeName, const bool enableOverride, const int refinementLevel)
{
	UsdPrim prim = g_stage->GetPrimAtPath(SdfPath(nodeName));
	if (!prim.IsValid()) return;

	{
		UsdAttribute attr = prim.GetAttribute(TfToken("refinementEnableOverride"));
		attr.Set(enableOverride);
	}
	{
		UsdAttribute attr = prim.GetAttribute(TfToken("refinementLevel"));
		attr.Set(refinementLevel);
	}
}

/**
 * usdzファイルを出力.
 * @param[in] filePath  出力ファイルパス.
 * @param[in] files     出力で追加するファイル（すべて絶対パスで指定）.
 */
bool CUSDExporter::exportUSDZ (const std::string& filePath, std::vector<std::string>& files)
{
	UsdZipFileWriter zipWriter = UsdZipFileWriter::CreateNew(filePath);

	for (size_t i = 0; i < files.size(); ++i) {
		// ファイル名を取得.
		const std::string fileName2 = StringUtil::getFileName(files[i]);

		// 第一引数は絶対パスでのファイル名指定、第二引数はzipアーカイブでの指定.
		zipWriter.AddFile(files[i], fileName2);
	}

	zipWriter.Save();

	return true;
}

/**
 * アニメーション情報を出力.
 * @param[in] startFrame  開始フレーム.
 * @param[in] endFrame    終了フレーム.
 * @param[in] framerate   フレームレート.
 */
void CUSDExporter::setAnimationData (const float startFrame, const float endFrame, const float framerate)
{
	if (!g_stage) return;

	g_stage->SetFramesPerSecond((double)framerate);
	g_stage->SetStartTimeCode((double)startFrame);
	g_stage->SetEndTimeCode((double)endFrame);
}

/**
 * Skeleton情報を出力.
 */
void CUSDExporter::appendSkeletonData (const CSkeletonData& skelData)
{
	if (!g_stage) return;

	// Skeletonのルート.
	{
		UsdPrim skelP = g_stage->GetPrimAtPath(SdfPath(SKELETONS_ROOT_PATH));
		if (!skelP.IsValid()) {
			skelP = g_stage->DefinePrim(SdfPath(SKELETONS_ROOT_PATH), TfToken("Scope"));
		}
	}

	const std::string rootName = std::string(SKELETONS_ROOT_PATH) + std::string("/") + skelData.rootName;
	const std::string skelName = rootName + std::string("/Skel");
	const std::string animName = skelName + std::string("/Anim");
	UsdPrim prim1 = g_stage->DefinePrim(SdfPath(rootName), TfToken("SkelRoot"));
	UsdSkelRoot skelRoot(prim1);
	UsdPrim prim2 = g_stage->DefinePrim(SdfPath(skelName), TfToken("Skeleton"));
	UsdSkelSkeleton skeleton(prim2);
	UsdPrim prim3 = g_stage->DefinePrim(SdfPath(animName), TfToken("SkelAnimation"));
	UsdSkelAnimation skelAnim(prim3);

	// 対応するジョイント名を格納.
	std::vector<TfToken> jointTokens;
	{
		UsdAttribute attr = skeleton.CreateJointsAttr();
		jointTokens.resize(skelData.joints.size());
		for (size_t i = 0; i < skelData.joints.size(); ++i) {
			jointTokens[i] = TfToken(skelData.joints[i].name);
		}
		attr.Set(VtTokenArray(jointTokens.begin(), jointTokens.end()));
	}

	// bindTransformsの変換行列を格納.
	{
		UsdAttribute attr = skeleton.CreateBindTransformsAttr();
		std::vector<GfMatrix4d> vList;
		vList.resize(skelData.joints.size());
		for (size_t i = 0; i < skelData.joints.size(); ++i) {
			const CSkelJointData& jointD = skelData.joints[i];
			for (int j = 0; j < 16; ++j) {
				*(vList[i].data() + j) = (double)jointD.bindTransforms[j];
			}
		}
		attr.Set(VtMatrix4dArray(vList.begin(), vList.end()));
	}

	// restTransformsの変換行列を格納.
	{
		UsdAttribute attr = skeleton.CreateRestTransformsAttr();
		std::vector<GfMatrix4d> vList;
		vList.resize(skelData.joints.size());
		for (size_t i = 0; i < skelData.joints.size(); ++i) {
			const CSkelJointData& jointD = skelData.joints[i];
			for (int j = 0; j < 16; ++j) {
				*(vList[i].data() + j) = (double)jointD.restTransforms[j];
			}
		}
		attr.Set(VtMatrix4dArray(vList.begin(), vList.end()));
	}

	// SkelAnimationにキーフレーム情報を格納.
	{
		// Skeletonで、SkelAnimationをBind.
		UsdSkelBindingAPI binding = UsdSkelBindingAPI::Apply(skeleton.GetPrim());
		//binding.CreateSkeletonRel().SetTargets(SdfPathVector({skelAnim.GetPath()}));
		binding.CreateAnimationSourceRel().SetTargets(SdfPathVector({skelAnim.GetPath()}));

		// joint名を格納.
		skelAnim.GetJointsAttr().Set(VtTokenArray(jointTokens.begin(), jointTokens.end()));

		const size_t jointsCou = skelData.joints.size();
		std::vector<float> keyframes;

		bool useTranslation = false;
		bool useRotation    = false;
		bool useScale       = false;

		// 移動情報を格納.
		{
			std::vector< std::vector<CJointTranslationData> > transList;
			std::vector<GfVec3f> trans;
			if (skelData.getJointsTranslationData(keyframes, transList) >= 1) {		// キーフレーム情報を取得.
				UsdAttribute attr = skelAnim.CreateTranslationsAttr();
				const size_t keyframesCou = keyframes.size();
				for (size_t i = 0; i < keyframesCou; ++i) {
					trans.resize(jointsCou);
					for (size_t j = 0; j < jointsCou; ++j) {
						const CJointTranslationData& sD = transList[j][i];
						trans[j] = GfVec3f(sD.x, sD.y, sD.z);
					}
					attr.Set(VtVec3fArray(trans.begin(), trans.end()), UsdTimeCode(keyframes[i]));
				}
				useTranslation = true;
			}
		}

		// 回転情報を格納.
		{
			std::vector< std::vector<CJointRotationData> > rotList;
			std::vector<GfQuatf> quats;
			if (skelData.getJointsRotationData(keyframes, rotList) >= 1) {		// キーフレーム情報を取得.
				UsdAttribute attr = skelAnim.CreateRotationsAttr();
				const size_t keyframesCou = keyframes.size();
				for (size_t i = 0; i < keyframesCou; ++i) {
					quats.resize(jointsCou);
					for (size_t j = 0; j < jointsCou; ++j) {
						const CJointRotationData& sD = rotList[j][i];
						quats[j] = GfQuatf(sD.w, sD.x, sD.y, sD.z);
					}
					attr.Set(VtQuatfArray(quats.begin(), quats.end()), UsdTimeCode(keyframes[i]));
				}
				useRotation = true;

			} else {
				// translation情報のキーフレームがある場合、rotation/scaleも出力する必要がある.
				if (useTranslation) {
					UsdAttribute attr = skelAnim.CreateRotationsAttr();
					for (size_t j = 0; j < jointsCou; ++j) {
						quats.push_back(GfQuatf(1, 0, 0, 0));
					}
					attr.Set(VtQuatfArray(quats.begin(), quats.end()));
					useRotation = true;
				}
			}
		}

		// スケール情報を格納.
		{
			std::vector< std::vector<CJointScaleData> > scalesList;
			std::vector<GfVec3h> scales;
			if (skelData.getJointsScaleData(keyframes, scalesList) >= 1) {		// キーフレーム情報を取得.
				UsdAttribute attr = skelAnim.CreateScalesAttr();
				const size_t keyframesCou = keyframes.size();
				for (size_t i = 0; i < keyframesCou; ++i) {
					scales.resize(jointsCou);
					for (size_t j = 0; j < jointsCou; ++j) {
						const CJointScaleData& sD = scalesList[j][i];
						scales[j] = GfVec3h(sD.x, sD.y, sD.z);
					}
					attr.Set(VtVec3hArray(scales.begin(), scales.end()), UsdTimeCode(keyframes[i]));
				}
				useScale = true;

			} else {
				// translation情報のキーフレームがある場合、rotation/scaleも出力する必要がある.
				if (useTranslation || useRotation) {
					UsdAttribute attr = skelAnim.CreateScalesAttr();
					for (size_t j = 0; j < jointsCou; ++j) {
						scales.push_back(GfVec3h(1, 1, 1));
					}
					attr.Set(VtVec3hArray(scales.begin(), scales.end()));
					useScale = true;
				}
			}
		}
	}
}

namespace {
	/**
	 * 指定のノードで参照されるメッシュのノードパスを取得.
	 */
	void m_getMeshNodeNameList (UsdPrim prim, const std::string& basePath, std::vector<std::string>& nodeNameList) {
		TfTokenVector chNames = prim.GetAllChildrenNames();
	
		for (size_t i = 0; i < chNames.size(); ++i) {
			TfToken tk = chNames[i];
			const std::string pathStr2 = basePath + std::string("/") + tk.GetString();

			UsdPrim prim2 = g_stage->GetPrimAtPath(SdfPath(pathStr2));
			if (!prim2.IsValid()) continue;

			if (prim2.GetTypeName().GetString() == std::string("Mesh")) {
				nodeNameList.push_back(pathStr2);
			}
			m_getMeshNodeNameList(prim2, pathStr2, nodeNameList);
		}
	}
}

/**
 * マテリアルを複製.
 * これはShade3Dのリンク使用時に、マスターオブジェクトのスコープ内でマテリアルを参照できるようにする.
 * @param[in]  nodeName       対象のノードパス.
 * @param[in]  materialsList  マテリアル情報のリスト.
 */
void CUSDExporter::setMaterialsInScope (const std::string& nodeName, const std::vector<CMaterialData>& materialsList)
{
	if (!g_stage) return;

	// すでにnodeName内に"Materials"のノードがある場合はスキップ.
	const std::string materialsName = nodeName + std::string("/Materials");
	UsdPrim primMaterials = g_stage->GetPrimAtPath(SdfPath(materialsName));
	if (primMaterials.IsValid()) return;

	UsdPrim prim = g_stage->GetPrimAtPath(SdfPath(nodeName));

	std::vector<std::string> nodeNameList;
	::m_getMeshNodeNameList(prim, nodeName, nodeNameList);
	if (nodeNameList.empty()) return;

	// nodeNameの下に"Materials"のスコープを作成.
	g_stage->DefinePrim(SdfPath(materialsName), TfToken("Scope"));

	// materialsListよりマテリアル名を取り出し.
	std::vector<std::string> materialNameList;
	for (size_t i = 0; i < materialsList.size(); ++i) {
		const std::string name = materialsList[i].name;
		const int iPos = name.find_last_of("/");
		if (iPos != std::string::npos) {
			materialNameList.push_back(name.substr(iPos + 1));
		} else {
			materialNameList.push_back("");
		}
	}

	// nodeNameList[]のノードより、マテリアルの参照を取得.
	for (size_t i = 0; i < nodeNameList.size(); ++i) {
		const std::string pathStr = nodeNameList[i];
		
		UsdPrim prim2 = g_stage->GetPrimAtPath(SdfPath(pathStr));
		if (!prim2.IsValid()) continue;

		// マテリアルのバインドを取得.
		UsdRelationship relationS = UsdShadeMaterialBindingAPI(prim2).GetDirectBindingRel();

		// マテリアルの参照を取得.
		SdfPathVector pathList;
		if (!relationS.GetTargets(&pathList)) continue;
		if (pathList.empty()) continue;

		const std::string matPath = pathList[0].GetString();		// オリジナルのマテリアルのパスが返る.

		const int iPos = matPath.find_last_of("/");
		if (iPos != std::string::npos) {
			const std::string orgMaterialName = matPath.substr(iPos + 1);

			// 対象のマテリアルデータを探す.
			int mIndex = -1;
			for (size_t i = 0; i < materialNameList.size(); ++i) {
				if (materialNameList[i] == orgMaterialName) {
					mIndex = (int)i;
					break;
				}
			}
			if (mIndex < 0) continue;

			// マテリアルの実体.
			UsdPrim primMat = g_stage->GetPrimAtPath(SdfPath(matPath));

			// マテリアルのノードを作成.
			const std::string newPath = materialsName + std::string("/") + orgMaterialName;
			UsdPrim newPrimMat = g_stage->DefinePrim(SdfPath(newPath), TfToken("Material"));

			// マテリアル情報を追加.
			if (!m_exportParam.useShaderMDL()) {
				m_appendNodeMaterial(newPath, materialsList[mIndex]);
			} else {
				// OmniverseのMDLの場合.
				m_appendNodeMaterial_OmniverseMDL(newPath, materialsList[mIndex]);
			}

			// 参照を置き換え.
			UsdShadeMaterial mat(newPrimMat);
			UsdShadeMaterialBindingAPI(prim2).Bind(mat);
		}
	}
}

/**
 * 非表示化.
 * @param[in] nodeName    ノード名 (/root/xxx/mesh1 などのパス形式).
 */
void CUSDExporter::setActive (const std::string& nodeName, const bool activeV)
{
	UsdPrim prim = g_stage->GetPrimAtPath(SdfPath(nodeName));
	if (!prim.IsValid()) return;

	// 以下は、iOS 14.4では効かない.
	//UsdGeomXform node(prim);
	//if (visible) node.MakeVisible();
	//else node.MakeInvisible();

	// これはiOS 14.4で効いている.
	prim.SetActive(activeV);
}

/**
 * 非表示化。これはiOS 14.4で非表示となるわけではない.
 * @param[in] nodeName    ノード名 (/root/xxx/mesh1 などのパス形式).
 */
void CUSDExporter::setVisible (const std::string& nodeName, const bool visible)
{
	UsdPrim prim = g_stage->GetPrimAtPath(SdfPath(nodeName));
	if (!prim.IsValid()) return;

	// 以下は、iOS 14.4では効かない.
	const TfToken typeName = prim.GetTypeName();
	if (typeName == TfToken("Xform") || typeName == TfToken("Scope")) {
		UsdGeomXform node(prim);
		if (visible) node.MakeVisible();
		else node.MakeInvisible();
	}
}

