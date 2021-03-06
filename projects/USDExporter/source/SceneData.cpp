﻿/**
 * 一時的なシーン情報を保持.
 */
#include "SceneData.h"
#include "StringUtil.h"
#include "USDData.h"
#include "USDExporter.h"
#include "Shade3DUtil.h"
#include "MathUtil.h"
#include "StreamCtrl.h"
#include "SkeletonData.h"
#include "AnimKeyframeBake.h"

#define MATERIAL_ROOT_PATH  "/root/Materials"
#define SKELETONS_ROOT_PATH  "/root/Skeletons"
#define ROOT_PATH  "/root"
#define MASTER_OBJECT_PART_PATH "/root/MasterObjects"

CSceneData::CSceneData ()
{
	clear();
}

CSceneData::~CSceneData ()
{
}

void CSceneData::clear ()
{
	m_materialTextureBake.reset();
	m_pScene = NULL;
	filePath = "";
	m_findNames.clear();
	m_elementsList.clear();
	tmpMeshData.clear();
	nodesList.clear();
	materialsList.clear();
	m_exportFilesList.clear();
}

/**
 * エクスポート開始の情報を渡す.
 * @param[in] scene        Shade3Dのシーンクラス.
 * @param[in] exportParam  エクスポートパラメータ.
 */
void CSceneData::setupExport (sxsdk::scene_interface* scene, const CExportParam& exportParam)
{
	m_pScene = scene;
	m_exportParam = exportParam;

	m_materialTextureBake.reset(new CMaterialTextureBake(m_pScene, m_exportParam));
}

/**
 * 指定の形状がメッシュに変換できるか調べる.
 */
bool CSceneData::checkConvertMesh (sxsdk::shape_class* shape)
{
	const int type = shape->get_type();
	if (type == sxsdk::enums::polygon_mesh || type == sxsdk::enums::sphere || type == sxsdk::enums::disk) return true;

	if (type == sxsdk::enums::part) {
		const int partType = shape->get_part().get_part_type();
		if (partType == sxsdk::enums::simple_part) return false;
		if (partType == sxsdk::enums::ball_joint || partType == sxsdk::enums::bone_joint) return false;
		if (partType == sxsdk::enums::surface_part) return true;
	} else if (type == sxsdk::enums::line) {
		if (shape->get_line().get_closed()) return true;
	}

	return false;
}

/**
 * 指定の形状を格納する.
 * @param[in] shape  形状の参照.
 * @param[in] linkedParentShape  リンク時の親の参照.
 * @return USDで格納する際の形状パス.
 */
std::string CSceneData::appendShape (sxsdk::shape_class* shape, sxsdk::shape_class* linkedParentShape)
{
	const std::string name = shape->get_name();
	const int type = shape->get_type();

	// 名前をパスの形にする.
	std::string name2 = getShapePath(shape, linkedParentShape);

	// ルートノード名.
	if (!shape->has_dad()) {
		name2 = std::string(ROOT_PATH);
	}

	// マスターオブジェクトパートの場合.
	if (shape->get_type() == sxsdk::enums::part) {
		sxsdk::part_class& part = shape->get_part();
		if (part.get_part_type() == sxsdk::enums::master_shape_part) {
			name2 = std::string(MASTER_OBJECT_PART_PATH);
		}
	}

	// 要素名を格納.
	USD_DATA::NODE_TYPE nodeType = USD_DATA::NODE_TYPE::null_node;
	if (checkConvertMesh(shape)) nodeType = USD_DATA::NODE_TYPE::mesh_node;

	name2 = m_findNames.appendName(name2, nodeType);

	// 要素情報を格納.
	m_elementsList.push_back(CElementData());
	CElementData& elemD = m_elementsList.back();
	elemD.orgName   = name;
	elemD.storeName = name2;
	elemD.shape     = shape;

	return name2;
}

/**
 * 同一の形状がすでに格納済みの場合は、既存のパスを取得.
 */
std::string CSceneData::searchSamePath (sxsdk::shape_class* shape)
{
	const std::string name = shape->get_name();
	std::string name2 = name;

	// すでに同様のものを格納済みか.
	{
		bool chkF = false;
		void* sHandle = shape->get_handle();
		for (size_t i = 0; i < m_elementsList.size(); ++i) {
			const CElementData& elemD = m_elementsList[i];
			if (elemD.shape) {
				if (elemD.shape->get_handle() == sHandle) {
					name2 = elemD.storeName;
					chkF = true;
					break;
				}
			}
		}
		if (chkF) return name2;
	}
	return "";
}

/**
 * 指定のパスを格納する。このときにユニークな名前を取得.
 * @param[in] nodeName    "/root/xxx/cylinder"のようなUSDでのパス.
 * @return ユニークな形状パス.
 */
std::string CSceneData::appendUniquePath (sxsdk::shape_class* shape, const std::string& nodeName)
{
	bool chkF = false;

	int cou = 0;
	std::string name = nodeName;

	while (1) {
		chkF = false;
		for (size_t i = 0; i < m_elementsList.size(); ++i) {
			const CElementData& elemD = m_elementsList[i];
			if (elemD.storeName == name) {
				chkF = true;
				break;
			}
		}
		if (!chkF) break;

		cou++;
		name = nodeName + std::string("_") + std::to_string(cou);
	}

	// 要素情報を格納.
	m_elementsList.push_back(CElementData());
	CElementData& elemD = m_elementsList.back();
	elemD.orgName   = nodeName;
	elemD.storeName = name;
	elemD.shape     = shape;

	return name;
}


/**
 * 情報として指定の形状がすでに格納済みかチェック（リンクやマスターオブジェクト格納時の最適化用）.
 */
bool CSceneData::existStoreShape (sxsdk::shape_class* shape)
{
	const std::string pathStr = searchSamePath(shape);
	if (pathStr == "") return false;

	bool existF = false;
	for (size_t i = 0; i < nodesList.size(); ++i) {
		CNodeBaseData& nodeBaseD = *nodesList[i];
		if (nodeBaseD.name == pathStr) {
			existF = true;
			break;
		}
	}
	return existF;
}

/**
 * 指定の形状に対応するUSDでのパスを取得.
 * @param[in] shape  形状の参照.
 * @param[in] linkedParentShape  リンク時の親の参照.
 * @return USDで格納する際の形状パス.
 */
std::string CSceneData::getShapePath (sxsdk::shape_class* shape, sxsdk::shape_class* linkedParentShape)
{
	const std::string name = shape->get_name();
	std::string name2 = name;

	if (name2 != "") {
		// 先頭の文字がアルファベットでない場合は"_"を入れる.
		{
			const char* pStr = name2.c_str();
			if (!((pStr[0] >= 'a' && pStr[0] <= 'z') || (pStr[0] >= 'A' && pStr[0] <= 'Z'))) {
				name2 = std::string("_") + name2;
			}
		}

		// "/"がある場合は、"_"に置き換え.
		{
			for (size_t i = 0; i < name2.length(); ++i) {
				const char chD = name2[i];
				if (chD == '/') name2[i] = '_';
			}
		}
	}

	if (!linkedParentShape) {
		bool chkF = false;
		void* sHandle = shape->get_handle();
		for (size_t i = 0; i < m_elementsList.size(); ++i) {
			const CElementData& elemD = m_elementsList[i];
			if (elemD.shape) {
				if (elemD.shape->get_handle() == sHandle) {
					name2 = elemD.storeName;
					chkF = true;
					break;
				}
			}
		}
		if (chkF) return name2;
	}

	sxsdk::shape_class* s = shape;
	if (s->has_dad() || linkedParentShape) {
		s = linkedParentShape ? linkedParentShape : (s->get_dad());
		void* sHandle = s->get_handle();

		for (size_t i = 0; i < m_elementsList.size(); ++i) {
			const CElementData& elemD = m_elementsList[i];
			if (elemD.shape) {
				if (elemD.shape->get_handle() == sHandle) {
					name2 = elemD.storeName + std::string("/") + name2;
					break;
				}
			}
		}
	} else {
		name2 = std::string("/") + name2;
	}

	return name2;
}

/**
 * パートをNULLノードとして格納.
 * @param[in] shape     対象の形状クラス.
 * @param[in] namePath  形状パス (/root/objects/xxx のような形式).
 * @param[in] matrix    変換行列.
 */
void CSceneData::appendNodeNull (sxsdk::shape_class* shape, const std::string& namePath, const sxsdk::mat4& matrix)
{
	nodesList.push_back(std::make_shared<CNodeNullData>());

	CNodeNullData& nodeD = static_cast<CNodeNullData &>(*(nodesList.back()));
	nodeD.name     = namePath;
	nodeD.matrix   = matrix;
	nodeD.nodeType = USD_DATA::NODE_TYPE::null_node;
	if (Shade3DUtil::isBone(*shape)) {
		nodeD.nodeType = USD_DATA::NODE_TYPE::bone_node;
	}

	if (Shade3DUtil::isBallJoint(*shape)) {
		nodeD.nodeType = USD_DATA::NODE_TYPE::ball_joint_node;
	}

	nodeD.shapeHandle = shape->get_handle();

	m_getJointMotionData(shape, nodeD);
}

/**
 * リンクとしての参照を追加.
 * @param[in] shape     対象形状.
 * @param[in] namePath  形状パス (/root/objects/xxx のような形式).
 */
void CSceneData::appendNodeReference (sxsdk::shape_class* shape, const std::string& namePath)
{
	nodesList.push_back(std::make_shared<CNodeRefData>());

	CNodeRefData& nodeD = static_cast<CNodeRefData &>(*(nodesList.back()));
	nodeD.name        = namePath;
	nodeD.shapeHandle = shape->get_handle();
}

/**
 * ボーンのジョイントモーション情報を取得.
 */
void CSceneData::m_getJointMotionData (sxsdk::shape_class* shape, CNodeNullData& nodeD)
{
	nodeD.jointMotion.clear();
	if (!shape->has_motion()) return;
	if (m_exportParam.animKeyframeMode == USD_DATA::EXPORT::ANIM_KEYFRAME_MODE::anim_keyframe_none) return;

	// ジョイント値を取得。ステップ数での再分割も行う.
	const CAnimationData animD = m_getAnimationData(m_pScene);
	CAnimKeyframeBake keyframeBake(m_pScene, m_exportParam);
	keyframeBake.storeKeyframes(shape, animD.startFrame, animD.endFrame);

	const std::vector<CAnimKeyframeData>& keyframeData = keyframeBake.getKeyframes();

	try {
		compointer<sxsdk::motion_interface> motion(shape->get_motion_interface());
		const int pointsCou = motion->get_number_of_motion_points();
		if (pointsCou == 0) return;

		// 移動(offset)/回転/スケール要素をキーフレームとして格納.
		CJointMotionData& jointMotionD = nodeD.jointMotion;
		CJointTranslationData transD;
		CJointRotationData rotD;
		CJointScaleData scaleD;
		for (size_t loop = 0; loop < keyframeData.size(); ++loop) {
			const CAnimKeyframeData& animD = keyframeData[loop];

			const sxsdk::vec3 offset        = animD.offset;
			sxsdk::vec3 offset2             = Shade3DUtil::convUnit_mm_to_cm(offset);		// cmに変換.
			const sxsdk::quaternion_class q = animD.quat;

			// 移動情報を格納.
			transD.frame = animD.framePos;
			transD.x = offset2.x;
			transD.y = offset2.y;
			transD.z = offset2.z;
			jointMotionD.translations.push_back(transD);

			// 回転情報を格納.
			rotD.frame = animD.framePos;
			rotD.x = q.x;
			rotD.y = q.y;
			rotD.z = q.z;
			rotD.w = -q.w;		// wはマイナスにしないと回転が逆になる.
			jointMotionD.rotations.push_back(rotD);

			// スケール情報を格納.
			scaleD.x = animD.scale.x;
			scaleD.y = animD.scale.y;
			scaleD.z = animD.scale.z;
			jointMotionD.scales.push_back(scaleD);
		}

	} catch (...) { }
}

/**
 * Meshを追加.
 * Mesh追加時に、マテリアルがある場合はそれもmaterialsListに追加する.
 * @param[in] shape     対象形状.
 * @param[in] namePath  形状パス (/root/objects/xxx のような形式).
 * @param[in] matrix    変換行列.
 * @param[in] meshData  メッシュ情報.
 */
void CSceneData::appendNodeMesh (sxsdk::shape_class* shape, const std::string& namePath, const sxsdk::mat4& matrix, const CTempMeshData& _tempMeshData)
{
	// Mesh情報を、格納用に変換.
	// このときにフェイスグループ別にMeshを分ける.
	std::vector<CNodeMeshData> meshes;
	CNodeMeshData::convert(_tempMeshData, meshes);
	if (meshes.empty()) return;

	// マテリアルを格納.
	int matIndex = -1;
	CMaterialData materialD;

	// 表面材質を持つ形状までたどる.
	sxsdk::shape_class* pS = Shade3DUtil::getHasSurfaceParentShape(shape);
	sxsdk::surface_class* pCurrentSurface = NULL;
	if (pS && (pS->has_surface())) pCurrentSurface = pS->get_surface();

	// 形状自身のマテリアル、マスターサーフェスのマテリアルを格納.
	std::vector<sxsdk::master_surface_class *> tmpMaterialMasterSurfaces;
	std::vector<int> tmpMaterialIndexList;
	std::vector<sxsdk::surface_class*> tmpMaterialSurfaceList;

	// 同一のマテリアルを持つかチェック.
	{
		const int matIndex = m_findSameMaterial(pCurrentSurface);
		tmpMaterialIndexList.push_back(matIndex);
		tmpMaterialSurfaceList.push_back(pCurrentSurface);
	}
	tmpMaterialMasterSurfaces.push_back(NULL);

	// フェイスグループのマテリアル情報を取得.
	if (shape->get_type() == sxsdk::enums::polygon_mesh) {
		sxsdk::polygon_mesh_class& pMesh = shape->get_polygon_mesh();
		const int fgCou = pMesh.get_number_of_face_groups();
		for (int i = 0; i < fgCou; ++i) {
			sxsdk::master_surface_class* pMasterSurface = pMesh.get_face_group_surface(i);
			if (pMasterSurface) {
				// 同一のマテリアルを持つかチェック.
				const int matIndex = m_findSameMaterial(pMasterSurface);
				tmpMaterialIndexList.push_back(matIndex);
				tmpMaterialSurfaceList.push_back(pMasterSurface->get_surface());
				tmpMaterialMasterSurfaces.push_back(pMasterSurface);
			}
		}
	}

	// 複数のフェイスグループで構成される場合は、namePathのノードを作り、その中に「mesh_x」名のメッシュを格納する.
	if (meshes.size() >= 2) {
		nodesList.push_back(std::make_shared<CNodeNullData>());
		CNodeNullData& nodeD = static_cast<CNodeNullData &>(*(nodesList.back()));
		nodeD.name = namePath;
		nodeD.matrix = matrix;
		nodeD.hasFaceGroup = true;
	}

	// メッシュごとに格納.
	for (size_t meshLoop = 0; meshLoop < meshes.size(); ++meshLoop) {
		CNodeMeshData& nodeD = meshes[meshLoop];
		nodeD.name = namePath;
		if (meshes.size() >= 2) {
			nodeD.name += std::string("/mesh_") + std::to_string(meshLoop);
			nodeD.faceGroupMesh = true;
		} else {
			nodeD.matrix = matrix;
		}
		nodeD.shapeHandle = shape->get_handle();

		// フェイスグループの場合、マスターサーフェスを取得.
		sxsdk::master_surface_class* masterSurface = NULL;
		if (nodeD.masterSurfaceHangle) {
			masterSurface = m_pScene->get_shape_by_handle(nodeD.masterSurfaceHangle)->get_master_surface();
		}

		int tmpMaterialIndex = -1;
		for (size_t i = 0; i < tmpMaterialMasterSurfaces.size(); ++i) {
			if (masterSurface == NULL && tmpMaterialMasterSurfaces[i] == NULL) {
				if (tmpMaterialIndex < 0) {
					tmpMaterialIndex = i;
				}
				break;
			}
			if (!masterSurface) continue;

			if (tmpMaterialMasterSurfaces[i] == masterSurface) {
				tmpMaterialIndex = i;
				break;
			}
		}
		if (tmpMaterialIndex < 0) continue;

		// マテリアルで同一のものがある場合、0以上のインデックスが入る.
		matIndex = tmpMaterialIndexList[tmpMaterialIndex];

		if (matIndex < 0) {
			// マテリアルを作成.
			if (!masterSurface) {
				m_materialTextureBake->getMaterialDataFromShape(shape, materialD);
			} else {
				m_materialTextureBake->getMaterialFromMasterSurface(masterSurface, NULL, materialD);
			}

			// ユニークなマテリアル名を取得.
			const std::string newName = m_findNames.appendName(materialD.name, USD_DATA::NODE_TYPE::material_node);
			materialD.name = newName;
			materialD.pSurface = (void *)tmpMaterialSurfaceList[tmpMaterialIndex];		// sxsdk::surface_classの識別用.
			materialD.refCount = 1;
			matIndex = (int)materialsList.size();
			materialsList.push_back(materialD);

			for (size_t j = 0; j < tmpMaterialIndexList.size(); ++j) {
				if ((void *)tmpMaterialSurfaceList[j] == materialD.pSurface) {
					tmpMaterialIndexList[j] = matIndex;
				}
			}

		} else {
			materialsList[matIndex].refCount++;
		}

		// Meshにマテリアルの参照を渡す.
		{
			const CMaterialData& targetMaterialD = materialsList[matIndex];
			nodeD.refMaterialName = targetMaterialD.name;
			nodeD.materialIndex   = matIndex;
		}

		nodesList.push_back(std::make_shared<CNodeMeshData>());
		static_cast<CNodeMeshData &>(*(nodesList.back())) = nodeD;
	}
}

/**
 * 指定の表面材質を持つマテリアル番号を取得.
 */
int CSceneData::m_findSameMaterial (sxsdk::surface_class* pSurface)
{
	int matIndex = -1;
	for (size_t i = 0; i < materialsList.size(); ++i) {
		const CMaterialData& mD = materialsList[i];
		if (mD.pSurface == (void *)pSurface) {
			matIndex = (int)i;
			break;
		}
	}
	return matIndex;
}

int CSceneData::m_findSameMaterial (sxsdk::master_surface_class* pMasterSurface)
{
	if (!pMasterSurface) return -1;
	sxsdk::surface_class* pSurface = pMasterSurface->get_surface();
	if (!pSurface) return -1;
	return m_findSameMaterial(pSurface);
}

/**
 * Shade3Dの変換行列から USD_DATA::NodeMatrixData に変換.
 */
USD_DATA::NodeMatrixData CSceneData::m_convMatrix (const sxsdk::mat4& m)
{
	sxsdk::vec3 scale ,shear, rotation, trans;
	USD_DATA::NodeMatrixData retM;

	// 要素ごとに分解.
	m.unmatrix(scale ,shear, rotation, trans);

	retM.translate[0] = trans.x;
	retM.translate[1] = trans.y;
	retM.translate[2] = trans.z;

	// 回転を度数に変換して格納.
	retM.rotate[0] = rotation.x * 180.0f / sx::pi;
	retM.rotate[1] = rotation.y * 180.0f / sx::pi;
	retM.rotate[2] = rotation.z * 180.0f / sx::pi;

	retM.scale[0] = scale.x;
	retM.scale[1] = scale.y;
	retM.scale[2] = scale.z;

	return retM;
}

/**
 * USDファイルを出力.
 * @param[in] shade        shade_interface
 * @param[in] filePath     出力ファイル名（絶対パス）.
 */
void CSceneData::exportUSD (sxsdk::shade_interface& shade, const std::string& filePath)
{
	CUSDExporter usdExport;
	USD_DATA::MeshData tmpMeshData;

	m_usdzFileName = "";
	m_exportFilesList.clear();
	m_exportFilesList.push_back(filePath);

	// テクスチャを出力.
	m_exportTextures(filePath);

	// エクスポート開始.
	usdExport.beginExport(filePath, m_exportParam);

	// プラグインバージョンを取得して渡す.
	{
		std::string verStr = "";
		verStr += std::to_string(USD_SHADE3D_PLUGIN_MAJOR_VERSION) + std::string(".");
		verStr += std::to_string(USD_SHADE3D_PLUGIN_MINOR_VERSION) + std::string(".");
		verStr += std::to_string(USD_SHADE3D_PLUGIN_MICRO_VERSION) + std::string(".");
		verStr += std::to_string(USD_SHADE3D_PLUGIN_BUILD_NUMBER);
		usdExport.setVersionString(verStr);
	}

	// スキンを持つ形状で、名前の重複がある場合は別名を付ける.
	m_makeUniqueName();

	// マテリアルを追加.
	if (!materialsList.empty()) {
		usdExport.SetImagesList(m_materialTextureBake->getImagesList());
		for (size_t i = 0; i < materialsList.size(); ++i) {
			const CMaterialData& matD = materialsList[i];
			if (!m_exportParam.useShaderMDL()) {
				usdExport.appendNodeMaterial(matD);
			} else {
				usdExport.appendNodeMaterial_OmniverseMDL(matD);
			}
		}
	}

	// アニメーション情報を出力 (ヘッダ部).
	if (m_exportParam.animKeyframeMode != USD_DATA::EXPORT::ANIM_KEYFRAME_MODE::anim_keyframe_none) {
		const CAnimationData animD = m_getAnimationData(m_pScene);
		usdExport.setAnimationData(animD.startFrame, animD.endFrame, animD.frameRate);
	}

	// Sksletonとjoint情報を出力.
	if (m_exportParam.optOutputBoneSkin) {
		m_exportSkeletonAndJoints(usdExport);
	}

	// ノードを追加.
	if (!nodesList.empty()) {
		for (size_t i = 0; i < nodesList.size(); ++i) {
			CNodeBaseData& nodeBaseD = *nodesList[i];
			if ((nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::null_node || (nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::ball_joint_node || (nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::bone_node) {
				CNodeNullData& nodeD = static_cast<CNodeNullData &>(nodeBaseD);

				// モーション情報を持つ場合.
				if (nodeD.jointMotion.hasMotion()) {
					// ジョイントの回転情報を、QuaternionからEulerに変換し格納.
					// これは、iOS(12.4.1)環境でtransform animationのxformOp:orientが機能しないため.
					m_calcJointQuaternionToEuler(nodeD.jointMotion);

					nodeD.matrix = sxsdk::mat4::identity;
				}

				// 変換行列.
				const USD_DATA::NodeMatrixData usdMatrix = m_convMatrix(nodeD.matrix);
				usdExport.appendNodeNull(nodeD.name, usdMatrix, nodeD.jointMotion);

			} else if ((nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::mesh_node) {
				CNodeMeshData& nodeD = static_cast<CNodeMeshData &>(nodeBaseD);

				// メッシュ情報を作業データに変換.
				nodeD.convertTo(tmpMeshData);

				// 変換行列.
				const USD_DATA::NodeMatrixData usdMatrix = m_convMatrix(nodeD.matrix);

				// USDの構造に渡す.
				bool doubleSided = false;
				if (tmpMeshData.materialIndex >= 0) {
					const CMaterialData& matD = this->materialsList[tmpMeshData.materialIndex];
					if (matD.doubleSided) doubleSided = true;
				}

				// スケルトン情報(m_skeletonList)から、メッシュ情報での参照(ジョイントインデックス)を取得/格納.
				m_setMeshSkeletonRef(nodeD, tmpMeshData);

				usdExport.appendNodeMesh(nodeD.name, usdMatrix, tmpMeshData, doubleSided);
			}
		}

		// ノードの参照（Shade3Dでのリンク）の考慮.
		{
			std::vector<std::string> orgNameList;
			std::vector<std::string> orgMaterialNameList;
			for (size_t i = 0; i < nodesList.size(); ++i) {
				CNodeBaseData& nodeBaseD = *nodesList[i];
				if ((nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::ref_node) {
					// リンクの場合は、参照を作る.
					CNodeRefData& nodeD = static_cast<CNodeRefData &>(nodeBaseD);

					std::string orgShapeName, orgMaterialName;
					m_getShapeRef((int)i, nodeD, orgNameList, orgMaterialNameList);
					if (orgNameList.empty()) continue;

					bool linkPartF = false;
					for (size_t j = 0; j < orgNameList.size(); ++j) {
						orgShapeName    = orgNameList[j];
						orgMaterialName = orgMaterialNameList[j];
						if (orgShapeName != "") usdExport.setShapeReference(nodeD.name, orgShapeName, orgMaterialName);
						if (orgMaterialName == "") linkPartF = true;
					}

					// パートをリンクしている場合、参照先のノードはマテリアルもスコープ内に格納する必要がある.
					if (linkPartF) {
						m_setLinkMaterials(usdExport, (int)i, nodeD);
					}
				}
			}
		}

		// マスターオブジェクトノードは非表示にする.
		// iOS 14.4の場合はVisibleではなくActiveフラグで非表示指定になる.
		usdExport.setActive(MASTER_OBJECT_PART_PATH, false);

		// Skeletonsノードを非表示にする.
		//usdExport.setVisible(std::string(SKELETONS_ROOT_PATH), false);
	}

	// エクスポート終了.
	usdExport.endExport();
}

/**
 * テクスチャイメージの出力.
 * filePathのフォルダにテクスチャを出力する.
 * @param[in]  filePath  出力ファイルパス（xxxx.usd などのファイル名も付加される）.
 */
void CSceneData::m_exportTextures (const std::string& filePath)
{
	// ディレクトリパスを取得.
	const std::string fileDir = StringUtil::getFileDir(filePath);

	// テクスチャをファイル出力.
	const std::vector<CImageData>& imagesList = m_materialTextureBake->getImagesList();
	for (size_t i = 0; i < imagesList.size(); ++i) {
		const CImageData& imageD = imagesList[i];
		if (imageD.fileName == "") continue;

		const std::string fileName = fileDir + StringUtil::getFileSeparator() + imageD.fileName;
		if (imageD.pMasterImageHandle) {
			try {
				sxsdk::master_image_class& masterImage = m_pScene->get_shape_by_handle(imageD.pMasterImageHandle)->get_master_image();
				compointer<sxsdk::image_interface> image(masterImage.get_image());
				if (image) {
					// グレイスケールに変換して保存する場合.
					if (imageD.texTransform.convGrayscale) {
						// テクスチャのピクセルを加工する場合.
						compointer<sxsdk::image_interface> image2(Shade3DUtil::createImageWithTransform(image, imageD.textureSource, imageD.texTransform));
						m_saveTextureImage(fileName, image2);

					} else if (imageD.texTransform.isDefault()) {		// 変換要素がない場合.
						m_saveTextureImage(fileName, image);

					} else {					// 変換要素がある場合.
						compointer<sxsdk::image_interface> image2(Shade3DUtil::createImageWithTransform(image, imageD.textureSource, imageD.texTransform));
						m_saveTextureImage(fileName, image2);
					}

					// USDZ出力時のためのファイル名保持.
					m_exportFilesList.push_back(fileName);
				}
			} catch (...) { }

		} else if (!imageD.rgbaBuff.empty()) {
			// ベイクされたカスタムイメージを保存する場合.
			const int width  = imageD.imageWidth;
			const int height = imageD.imageHeight;

			// イメージを作成.
			compointer<sxsdk::image_interface> image(m_pScene->create_image_interface(sx::vec<int,2>(width, height)));
			if (image) {
				std::vector<sx::rgba8_class> lineBuff;
				lineBuff.resize(width);
				int iPos = 0;
				for (int y = 0; y < height; ++y) {
					for (int x = 0; x < width; ++x) {
						lineBuff[x].red   = imageD.rgbaBuff[iPos + 0];
						lineBuff[x].green = imageD.rgbaBuff[iPos + 1];
						lineBuff[x].blue  = imageD.rgbaBuff[iPos + 2];
						lineBuff[x].alpha = imageD.rgbaBuff[iPos + 3];
						iPos += 4;
					}
					image->set_pixels_rgba(0, y, width, 1, &(lineBuff[0]));
				}

				m_saveTextureImage(fileName, image);

				// USDZ出力時のためのファイル名保持.
				m_exportFilesList.push_back(fileName);
			}
		}
	}
}

 /**
  * テクスチャをエクスポートパラメータでリサイズしてファイル出力.
  * @param[in] fileName  出力ファイル名.
  * @param[in] image     imageクラス.
  */
 void CSceneData::m_saveTextureImage (const std::string fileName, sxsdk::image_interface* image)
 {
	const int texSize = USD_DATA::EXPORT::getTextureSize(m_exportParam.optMaxTextureSize);

	 try {
		if (m_exportParam.optMaxTextureSize == USD_DATA::EXPORT::texture_size_none) {
			image->save(fileName.c_str());
		} else {
			// イメージを2の累乗にリサイズ.
			const sx::vec<int,2> newSize = Shade3DUtil::calcImageSizePowerOf2(image->get_size(), texSize);
			compointer<sxsdk::image_interface> image2(Shade3DUtil::resizeImageWithAlpha(m_pScene, image, newSize));
			image2->save(fileName.c_str());
		}
	 } catch (...) { }
 }

/**
 * usdzファイルを出力。exportUSDのあとに実行すること.
 */
void CSceneData::exportUSDZ (const std::string& filePath)
{
	if (m_exportFilesList.empty()) return;

	CUSDExporter usdExport;
	usdExport.exportUSDZ(filePath, m_exportFilesList);
	m_usdzFileName = filePath;
}

/**
 * エクスポートしたファイル一覧を取得 (usdzも含む).
 */
std::vector<std::string> CSceneData::getExportFilesList () const
{
	std::vector<std::string> filesList = m_exportFilesList;
	if (m_usdzFileName != "") filesList.push_back(m_usdzFileName);
	return filesList;
}


/**
 * アニメーション情報を取得.
 */
CAnimationData CSceneData::m_getAnimationData (sxsdk::scene_interface* scene)
{
	CAnimationData animD;
	try {
		const float frameRate = (float)(m_pScene->get_frame_rate());
		const int totalFrames = m_pScene->get_total_frames();
		int startFrame = m_pScene->get_start_frame();
		if (startFrame < 0) startFrame = 0;
		int endFrame = m_pScene->get_end_frame();
		if (endFrame < 0) endFrame = totalFrames;

		animD.frameRate  = frameRate;
		animD.startFrame = (float)startFrame;
		animD.endFrame = (float)endFrame;
	} catch (...) { }

	return animD;
}

/**
 * Skeletonとjoint構造を出力.
 */
void CSceneData::m_exportSkeletonAndJoints (CUSDExporter& usdExport)
{
	m_skeletonList.clear();
	if (nodesList.empty()) return;

	// ルートボーン名を取得.
	const std::string rootN("/root/");
	std::vector<std::string> skelRootNames;
	std::vector<int> rootIndexList;
	std::vector<std::string> jointsList;
	std::vector<int> jointsIndexList;
	for (size_t sLoop = 0; sLoop < nodesList.size(); ++sLoop) {
		CNodeBaseData& nodeBaseD = *nodesList[sLoop];
		if ((nodeBaseD.nodeType) != USD_DATA::NODE_TYPE::bone_node) continue;

		CNodeNullData& nodeD = static_cast<CNodeNullData &>(nodeBaseD);
		if (nodeD.shapeHandle == NULL) continue;
		if (nodeD.name.find(rootN) != 0) continue;
		const std::string boneName = nodeD.name.substr(rootN.length());		// 先頭の「/root/」を省いた名前.

		// 形状の親が普通のパートの場合（ルートパートも含む）は、nodeDがボーンルート.
		bool rootBoneF = false;
		try {
			sxsdk::shape_class* shape = m_pScene->get_shape_by_handle(nodeD.shapeHandle);
			if (shape->has_dad()) {
				sxsdk::shape_class* pParentShape = shape->get_dad();
				if (pParentShape->get_type() == sxsdk::enums::part) {
					if (pParentShape->get_part().get_part_type() == sxsdk::enums::simple_part) {
						rootBoneF = true;
					}
				}
			}
		} catch (...) { }

		if (rootBoneF) {
			rootIndexList.push_back(sLoop);
			skelRootNames.push_back(boneName);
		}
		jointsList.push_back(boneName);
		jointsIndexList.push_back(sLoop);
	}
	if (skelRootNames.empty()) return;

	// Skeleton情報として格納.
	for (size_t sLoop = 0; sLoop < skelRootNames.size(); ++sLoop) {
		const std::string& rootName = skelRootNames[sLoop];

		// ルートボーンの要素を格納.
		CSkeletonData skelD;
		skelD.rootName = rootName;
		{
			const int index = rootIndexList[sLoop];
			CNodeBaseData& nodeBaseD = *nodesList[index];
			CNodeNullData& nodeD = static_cast<CNodeNullData &>(nodeBaseD);

			// ルートボーンのローカルワールド変換行列.
			sxsdk::shape_class* shape = m_pScene->get_shape_by_handle(nodeD.shapeHandle);
			const sxsdk::mat4 lwMat0 = shape->get_local_to_world_matrix();
			const sxsdk::mat4 mat = (shape->get_transformation()) * lwMat0;

			// nodeDのワールド座標変換行列を計算.
			const sxsdk::mat4 lwMat = Shade3DUtil::convUnit_mm_to_cm(mat);

			CSkelJointData jointD;
			jointD.name = rootName;
			USD_DATA::setMatrix4x4(&(lwMat[0][0]), jointD.bindTransforms);
			USD_DATA::setMatrix4x4(&(lwMat[0][0]), jointD.restTransforms);
			jointD.jointMotion = nodeD.jointMotion;
			jointD.shapeHandle = nodeD.shapeHandle;
			skelD.rootShapeHandle = nodeD.shapeHandle;
			skelD.joints.push_back(jointD);
		}

		// ルートボーンを親とする子ボーン要素を格納.
		// このとき、USDに渡すbindTransformsは、ルートボーンからの相対変換行列で渡す (個々のボーンのローカル座標ではない).
		for (size_t i = 0; i < jointsList.size(); ++i) {
			const std::string& jName = jointsList[i];
			if (jName.find(rootName + std::string("/")) != 0) continue;
	
			const int index = jointsIndexList[i];
			CNodeBaseData& nodeBaseD = *nodesList[index];
			CNodeNullData& nodeD = static_cast<CNodeNullData &>(nodeBaseD);

			// 形状のローカル・ワールド変換行列を取得.
			if (!nodeD.shapeHandle) continue;
			sxsdk::shape_class* shape = m_pScene->get_shape_by_handle(nodeD.shapeHandle);
			const sxsdk::mat4 lwMat0 = shape->get_local_to_world_matrix();
			sxsdk::mat4 lwMat = (shape->get_transformation()) * lwMat0;

			// もし、末端ボーンの場合は回転をクリア.
			if (Shade3DUtil::isBoneEnd(*shape)) {
			//	lwMat = Shade3DUtil::clearMatrixRotate(lwMat);
			}

			lwMat = Shade3DUtil::convUnit_mm_to_cm(lwMat);

			CSkelJointData jointD;
			jointD.name = jName;
			USD_DATA::setMatrix4x4(&(lwMat[0][0]), jointD.bindTransforms);
			USD_DATA::setMatrix4x4(&(nodeD.matrix[0][0]), jointD.restTransforms);
			jointD.jointMotion = nodeD.jointMotion;
			jointD.shapeHandle = nodeD.shapeHandle;

			skelD.joints.push_back(jointD);
		}

		m_skeletonList.push_back(skelD);
		usdExport.appendSkeletonData(skelD);
	}

	usdExport.setSkeletonsData(m_skeletonList);
}

/**
 * スケルトン情報(m_skeletonList)から、メッシュ情報での参照(ジョイントインデックス)を取得.
 */
void CSceneData::m_setMeshSkeletonRef (const CNodeMeshData& nodeMeshData, USD_DATA::MeshData& meshData)
{
	meshData.skinSkeletonIndex = -1;

	// Meshの頂点数に相当。頂点に最大4つのジョイントが割り当てられている.
	const size_t jointsVCou = meshData.skinJointsHandle.size();
	if (jointsVCou == 0) return;
	
	int skeletonIndex = -1;

	// Meshの頂点に割り当てられているジョイントのハンドルをいくつかサンプリング.
	std::vector<void *> targetShapeHandle;
	{
		const size_t maxJCou = 4;

		targetShapeHandle.clear();
		for (size_t i = 0; i < jointsVCou; ++i) {
			const std::vector<void *>& jointsHandle = meshData.skinJointsHandle[i];
			for (size_t j = 0; j < jointsHandle.size() && j < 1; ++j) {
				if (!jointsHandle[j]) continue;
				auto iter = std::find(targetShapeHandle.begin(), targetShapeHandle.end(), jointsHandle[j]);
				if (iter == targetShapeHandle.end()) {
					targetShapeHandle.push_back(jointsHandle[j]);
				}
			}
			if (targetShapeHandle.size() > maxJCou) break;
		}
	}
	if (targetShapeHandle.empty()) return;

	// targetShapeHandle[]のジョイントを持つm_skeletonListのインデックスを取得.
	{
		int sIndex;
		sIndex = -1;
		for (size_t loop = 0; loop < m_skeletonList.size(); ++loop) {
			const CSkeletonData& skelD = m_skeletonList[loop];
			const std::vector<CSkelJointData>& skelJointD = skelD.joints;
			sIndex = -1;
			for (size_t i = 0; i < skelJointD.size(); ++i) {
				const CSkelJointData& jointD = skelJointD[i];
				if (!jointD.shapeHandle) continue;

				for (size_t j = 0; j < targetShapeHandle.size(); ++j) {
					if (jointD.shapeHandle == targetShapeHandle[j]) {
						sIndex = loop;
						break;
					}
				}
				if (sIndex >= 0) break;
			}
			if (sIndex >= 0) break;
		}
		if (sIndex >= 0) {
			skeletonIndex = sIndex;
		}
	}

	if (skeletonIndex < 0) return;
	meshData.skinSkeletonIndex = skeletonIndex;

	// ボーンのグループ(skeletonIndex)別に、ボーン番号を格納.
	meshData.skinJoints.resize(jointsVCou);
	{
		const CSkeletonData& skelD = m_skeletonList[skeletonIndex];
		const std::vector<CSkelJointData>& skelJointD = skelD.joints;

		std::vector<void *> jointHandleList;
		jointHandleList.resize(skelJointD.size());
		for (size_t i = 0; i < skelJointD.size(); ++i) {
			jointHandleList[i] = skelJointD[i].shapeHandle;
		}

		for (size_t i = 0; i < jointsVCou; ++i) {
			meshData.skinJoints[i].resize(4);
			for (size_t j = 0; j < 4; ++j) meshData.skinJoints[i][j] = -1;
		}

		for (size_t i = 0; i < jointsVCou; ++i) {
			const std::vector<void *>& jointsHandle = meshData.skinJointsHandle[i];
			if (jointsHandle.size() != 4) continue;
			for (int j = 0; j < 4; ++j) {
				if (jointsHandle[j]) {
					auto iter = std::find(jointHandleList.begin(), jointHandleList.end(), jointsHandle[j]);
					meshData.skinJoints[i][j] = (int)std::distance(jointHandleList.begin(), iter);
				}
			}
		}
	}

}

/**
 * リンク情報を参照として取得.
 */
void CSceneData::m_getShapeRef (const int tIndex, const CNodeRefData& nodeRefData, std::vector<std::string>& orgNameList, std::vector<std::string>& orgMaterialNameList)
{
	orgNameList.clear();
	orgMaterialNameList.clear();
	std::string orgName = "";
	std::string orgMaterialName = "";

	for (size_t i = 0; i < nodesList.size(); ++i) {
		if ((int)i == tIndex) continue;
		CNodeBaseData& nodeBaseD = *nodesList[i];

		if ((nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::mesh_node) {
			CNodeMeshData& nodeD = static_cast<CNodeMeshData &>(nodeBaseD);
			if (nodeRefData.shapeHandle == nodeD.shapeHandle) {
				orgMaterialName = "";
				orgName = nodeD.name;
				if (nodeD.materialIndex >= 0) orgMaterialName = materialsList[nodeD.materialIndex].name;
				orgNameList.push_back(orgName);
				orgMaterialNameList.push_back(orgMaterialName);
			}
		} else if ((nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::null_node) {
			CNodeNullData& nodeD = static_cast<CNodeNullData &>(nodeBaseD);
			if (nodeRefData.shapeHandle == nodeD.shapeHandle) {
				orgMaterialName = "";
				orgName = nodeD.name;
				orgNameList.push_back(orgName);
				orgMaterialNameList.push_back(orgMaterialName);
			}
		}
	}
}

/**
 * マスターオブジェクト的な要素でパートとして参照される場合、スコープ内にマテリアルを移動させる.
 */
void CSceneData::m_setLinkMaterials (CUSDExporter& usdExport, const int tIndex, const CNodeRefData& nodeRefData)
{
	for (size_t i = 0; i < nodesList.size(); ++i) {
		if ((int)i == tIndex) continue;
		CNodeBaseData& nodeBaseD = *nodesList[i];

		if ((nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::null_node) {
			CNodeNullData& nodeD = static_cast<CNodeNullData &>(nodeBaseD);
			if (nodeRefData.shapeHandle == nodeD.shapeHandle) {
				// orgName内にある要素をたどり、「rel material:binding」のマテリアルの参照をorgNameに複製する.
				usdExport.setMaterialsInScope(nodeD.name, materialsList);
				break;
			}
		}
	}
}

/**
 * ジョイントの回転情報を、QuaternionからEulerに変換し格納.
 */
 void CSceneData::m_calcJointQuaternionToEuler (CJointMotionData& motionData)
 {
	 if (motionData.rotations.empty()) return;

	 // QuaternionからEuler(度数)に変換して格納.
	 const size_t mCou = motionData.rotations.size();
	 std::vector<sxsdk::vec3> eulerList;
	 {
		 sxsdk::vec3 eulerV;
		 eulerList.resize(mCou);
		 for (size_t i = 0; i < mCou; ++i) {
			 // rotD.wは格納時にマイナスで指定していたので、ここでは元に戻す.
			 const CJointRotationData& rotD = motionData.rotations[i];
			 sxsdk::quaternion_class q(sxsdk::vec4(rotD.x, rotD.y, rotD.z, -rotD.w));
			 q.get_euler(eulerV);
			 eulerV.x = eulerV.x * 180.0f / sx::pi;
			 eulerV.y = eulerV.y * 180.0f / sx::pi;
			 eulerV.z = eulerV.z * 180.0f / sx::pi;
			 eulerList[i] = eulerV;
		 }
	 }

#if true
	// キーフレーム間で、角度が180度よりも大きい場合は逆転しているとみて補正.
	const float maxAngle = 180.0f;
	float fD = 0.0f;
	for (size_t i = 1; i < mCou; ++i) {
		sxsdk::vec3 eV1 = eulerList[i - 1];
		sxsdk::vec3 eV2 = eulerList[i];

		for (int j = 0; j < 3; ++j) {
			if (std::abs(eV1[j] - eV2[j]) >= maxAngle) {
				fD = 0.0f;
				if (std::abs(eV1[j] - (eV2[j] + 360.0f)) < std::abs(eV1[j] - (eV2[j] - 360.0f))) {
					fD += 360.0f;
				} else {
					fD -= 360.0f;
				}
				for (size_t k = i; k < mCou; ++k) eulerList[k][j] += fD;
			}
		}
	}
#endif

	 for (size_t i = 0; i < mCou; ++i) {
		 CJointRotationData& rotD = motionData.rotations[i];
		 rotD.eulerX = eulerList[i].x;
		 rotD.eulerY = eulerList[i].y;
		 rotD.eulerZ = eulerList[i].z;
	 }
 }

/**
 * スキンを持つ形状で、名前の重複がある場合は別名を付ける.
 * スキンが割り当てられているMeshはSkeletonRootに格納されるため、パスが変わることになる.
 */
void CSceneData::m_makeUniqueName ()
{
	if (nodesList.empty()) return;

	// 各ノードの名前だけを格納.
	std::vector<std::string> nameList;
	nameList.resize(nodesList.size());
	for (size_t i = 0; i < nodesList.size(); ++i) {
		nameList[i] = "";
		const CNodeBaseData& nodeBaseD = *nodesList[i];
		std::string name = nodeBaseD.name;
		const int iPos = name.find_last_of("/");
		if (iPos != std::string::npos) {
			name = name.substr(iPos + 1);
		}
		nameList[i] = name;
	}

	// スキン情報を持つもののみ、名前を残す.
	for (size_t i = 0; i < nodesList.size(); ++i) {
		CNodeBaseData& nodeBaseD = *nodesList[i];
		if ((nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::null_node) {
			CNodeNullData& nodeD = static_cast<CNodeNullData &>(nodeBaseD);
			// face groupを持つノードの場合.
			if (nodeD.hasFaceGroup) {
				bool hasSkin = false;
				size_t j = i + 1;
				for (; j < nodesList.size(); ++j) {
					nameList[j] = "";
					CNodeBaseData& nodeBaseD2 = *nodesList[j];
					if ((nodeBaseD2.nodeType) == USD_DATA::NODE_TYPE::mesh_node) {
						CNodeMeshData& nodeD2 = static_cast<CNodeMeshData &>(nodeBaseD2);
						if (!nodeD2.faceGroupMesh) {
							j--;
							break;
						}
						if (!nodeD2.skinJointsHandle.empty()){
							hasSkin = true;
						}
					} else {
						j--;
						break;
					}
				}
				if (!hasSkin) nameList[i] = "";
				i = j;
			} else {
				nameList[i] = "";
			}

		} else if ((nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::mesh_node) {
			CNodeMeshData& nodeD = static_cast<CNodeMeshData &>(nodeBaseD);
			if (nodeD.skinJointsHandle.empty()) {
				nameList[i] = "";
			}
		} else {
			nameList[i] = "";
		}
	}

	// スキン情報を持つMeshまたはface groupの親のみ、名前が同一である場合にユニークにする.
	for (size_t i = 0; i < nodesList.size(); ++i) {
		const std::string name1 = nameList[i];
		if (name1 == "") continue;

		for (size_t j = i + 1; j < nodesList.size(); ++j) {
			if (name1 == nameList[j]) {
				int cou = 1;
				while (1) {
					const std::string newName = name1 + std::string("_") + std::to_string(cou);
					bool existF = false;
					for (size_t k = 0; k < nodesList.size(); ++k) {
						if (nameList[k] == "") continue;
						if (nameList[k] == newName) {
							existF = true;
							break;
						}
					}
					if (existF) {
						cou++;
						continue;
					}
					nameList[j] = newName;
					break;
				}
			}
		}
	}

	// 名前を変更.
	for (size_t i = 0; i < nodesList.size(); ++i) {
		if (nameList[i] == "") continue;
		CNodeBaseData& nodeBaseD = *nodesList[i];

		const int iPos = nodeBaseD.name.find_last_of("/");
		if (iPos != std::string::npos) {
			nodeBaseD.name = nodeBaseD.name.substr(0, iPos + 1) + nameList[i];
		}
	}
}

