/**
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

#define MATERIAL_ROOT_PATH  "/Materials"

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
 * @return USDで格納する際の形状パス.
 */
std::string CSceneData::appendShape (sxsdk::shape_class* shape)
{
	const std::string name = shape->get_name();
	const int type = shape->get_type();

	// 名前をパスの形にする.
	std::string name2 = getShapePath(shape);

	// ルートノード名.
	if (!shape->has_dad()) {
		if (!StringUtil::checkASCII(name2)) {
			name2 = std::string("/root");
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
 * 指定の形状に対応するUSDでのパスを取得.
 * @param[in] shape  形状の参照.
 * @return USDで格納する際の形状パス.
 */
std::string CSceneData::getShapePath (sxsdk::shape_class* shape)
{
	const std::string name = shape->get_name();
	std::string name2 = name;

	// 先頭の文字がアルファベットでない場合は"_"を入れる.
	if (name2 != "") {
		const char* pStr = name2.c_str();
		if (!((pStr[0] >= 'a' && pStr[0] <= 'z') || (pStr[0] >= 'A' && pStr[0] <= 'Z'))) {
			name2 = std::string("_") + name2;
		}
	}

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

	sxsdk::shape_class* s = shape;
	if (s->has_dad()) {
		s = s->get_dad();
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
	if (Shade3DUtil::isBone(*shape)) nodeD.nodeType = USD_DATA::NODE_TYPE::bone_node;
	if (Shade3DUtil::isBallJoint(*shape)) nodeD.nodeType = USD_DATA::NODE_TYPE::ball_joint_node;

	nodeD.shapeHandle = shape->get_handle();

	m_getJointMotionData(shape, nodeD);
}

/**
 * ボーンのジョイントモーション情報を取得.
 */
void CSceneData::m_getJointMotionData (sxsdk::shape_class* shape, CNodeNullData& nodeD)
{
	nodeD.jointMotion.clear();
	if (!shape->has_motion()) return;

	// ルートボーンの場合、変換行列を計算.
	sxsdk::mat4 boneRootMat = sxsdk::mat4::identity;
	try {
		if (shape->has_dad()) {
			sxsdk::shape_class* pParentShape = shape->get_dad();
			if (pParentShape->get_type() == sxsdk::enums::part) {
				if (pParentShape->get_part().get_part_type() == sxsdk::enums::simple_part) {
					boneRootMat = shape->get_local_to_world_matrix();
				}
			}
		}
	} catch (...) { }

	if (Shade3DUtil::isBone(*shape)) {				// ボーンの場合.
		try {
			const sxsdk::vec3 boneWCenter = Shade3DUtil::getJointCenter(*shape, NULL);
			const sxsdk::mat4 lwMat = shape->get_local_to_world_matrix();
			const sxsdk::vec3 boneLCenter = (boneWCenter * inv(lwMat)) * boneRootMat;

			compointer<sxsdk::motion_interface> motion(shape->get_motion_interface());
			const int pointsCou = motion->get_number_of_motion_points();
			if (pointsCou == 0) return;

			// 移動(offset)/回転要素をキーフレームとして格納.
			CJointMotionData& jointMotionD = nodeD.jointMotion;
			CJointTranslationData transD;
			CJointRotationData rotD;
			float oldSeqPos = -1.0f;
			for (int loop = 0; loop < pointsCou; ++loop) {
				compointer<sxsdk::motion_point_interface> motionPoint(motion->get_motion_point_interface(loop));
				float seqPos = motionPoint->get_sequence();

				// 同一のフレーム位置が格納済みの場合はスキップ.
				if (MathUtil::isZero(seqPos - oldSeqPos)) continue;
				if (oldSeqPos < 0.0f) oldSeqPos = seqPos;
				oldSeqPos = seqPos;

				const sxsdk::vec3 offset        = motionPoint->get_offset();
				sxsdk::vec3 offset2             = Shade3DUtil::convUnit_mm_to_cm(offset + boneLCenter);		// cmに変換.
				const sxsdk::quaternion_class q = motionPoint->get_rotation();

				// 移動情報を格納.
				transD.frame = seqPos;
				transD.x = offset2.x;
				transD.y = offset2.y;
				transD.z = offset2.z;
				jointMotionD.translations.push_back(transD);

				// 回転情報を格納.
				rotD.frame = seqPos;
				rotD.x = q.x;
				rotD.y = q.y;
				rotD.z = q.z;
				rotD.w = -q.w;		// wはマイナスにしないと回転が逆になる.

				jointMotionD.rotations.push_back(rotD);
			}

		} catch (...) { }

	} else if (Shade3DUtil::isBallJoint(*shape)) {				// ボールジョイントの場合.
		try {
			const sxsdk::mat4 lwMat = shape->get_local_to_world_matrix();
			const sxsdk::vec3 wCenter = Shade3DUtil::getJointCenter(*shape, NULL);

			compointer<sxsdk::motion_interface> motion(shape->get_motion_interface());
			const int pointsCou = motion->get_number_of_motion_points();
			if (pointsCou == 0) return;

			// 移動(offset)/回転要素をキーフレームとして格納.
			CJointMotionData& jointMotionD = nodeD.jointMotion;
			CJointTranslationData transD;
			CJointRotationData rotD;
			float oldSeqPos = -1.0f;
			sxsdk::vec3 dirV;
			for (int loop = 0; loop < pointsCou; ++loop) {
				compointer<sxsdk::motion_point_interface> motionPoint(motion->get_motion_point_interface(loop));
				float seqPos = motionPoint->get_sequence();

				// 同一のフレーム位置が格納済みの場合はスキップ.
				if (oldSeqPos >= 0.0f && MathUtil::isZero(seqPos - oldSeqPos)) continue;
				if (oldSeqPos < 0.0f) oldSeqPos = seqPos;
				oldSeqPos = seqPos;

				const sxsdk::vec3 offset        = motionPoint->get_offset();
				sxsdk::vec3 offset2             = Shade3DUtil::convUnit_mm_to_cm(offset + wCenter);		// cmに変換.
				const sxsdk::quaternion_class q = motionPoint->get_rotation();

				// 移動情報を格納.
				transD.frame = seqPos;
				transD.x = offset2.x;
				transD.y = offset2.y;
				transD.z = offset2.z;
				jointMotionD.translations.push_back(transD);

				// 回転情報を格納.
				rotD.frame = seqPos;
				rotD.x = q.x;
				rotD.y = q.y;
				rotD.z = q.z;
				rotD.w = -q.w;		// wはマイナスにしないと回転が逆になる.

				jointMotionD.rotations.push_back(rotD);
			}

		} catch (...) { }
	}

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

	// 形状自身のマテリアル、マスターサーフェスのマテリアルを格納.
	std::vector<CMaterialData> tmpMaterials;
	std::vector<sxsdk::master_surface_class *> tmpMaterialMasterSurfaces;

	// shapeの形状に割り当てられているマテリアルを取得.
	m_materialTextureBake->getMaterialDataFromShape(shape, materialD);
	tmpMaterials.push_back(materialD);
	tmpMaterialMasterSurfaces.push_back(NULL);

	// フェイスグループのマテリアル情報を取得.
	if (shape->get_type() == sxsdk::enums::polygon_mesh) {
		sxsdk::polygon_mesh_class& pMesh = shape->get_polygon_mesh();
		const int fgCou = pMesh.get_number_of_face_groups();
		for (int i = 0; i < fgCou; ++i) {
			sxsdk::master_surface_class* pMasterSurface = pMesh.get_face_group_surface(i);
			if (pMasterSurface) {
				if (m_materialTextureBake->getMaterialFromMasterSurface(pMasterSurface, NULL, materialD)) {
					tmpMaterialMasterSurfaces.push_back(pMasterSurface);
					tmpMaterials.push_back(materialD);
				}
			}
		}
	}

	// 複数のフェイスグループで構成される場合は、namePathのノードを作り、その中に「mesh_x」名のメッシュを格納する.
	if (meshes.size() >= 2) {
		CNodeNullData& nodeD = static_cast<CNodeNullData &>(*(nodesList.back()));
		nodeD.name = namePath;
		nodeD.matrix = matrix;
	}

	// メッシュごとに格納.
	for (size_t meshLoop = 0; meshLoop < meshes.size(); ++meshLoop) {
		CNodeMeshData& nodeD = meshes[meshLoop];
		nodeD.name = namePath;
		if (meshes.size() >= 2) {
			nodeD.name += std::string("/mesh_") + std::to_string(meshLoop);
		} else {
			nodeD.matrix = matrix;
		}

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

		matIndex = -1;
		const CMaterialData& materialD2 = tmpMaterials[tmpMaterialIndex];

		// マテリアルで同一のものがあるか探す.
		if (materialD2.pMasterSurfaceHandle) {
			for (size_t i = 0; i < materialsList.size(); ++i) {
				const CMaterialData& mD = materialsList[i];
				if (mD.pMasterSurfaceHandle == materialD2.pMasterSurfaceHandle) {
					matIndex = (int)i;
					break;
				}
			}
		} else {
			for (size_t i = 0; i < materialsList.size(); ++i) {
				const CMaterialData& mD = materialsList[i];
				if (mD.pMasterSurfaceHandle) continue;
				if (mD.isSame(materialD2)) {
					matIndex = (int)i;
					break;
				}
			}
		}

		if (matIndex < 0) {
			// ユニークなマテリアル名を取得.
			const std::string newName = m_findNames.appendName(materialD2.name, USD_DATA::NODE_TYPE::material_node);
			materialD = materialD2;
			materialD.name = newName;
			matIndex = (int)materialsList.size();
			materialsList.push_back(materialD);
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
	usdExport.beginExport(filePath);

	// マテリアルを追加.
	if (!materialsList.empty()) {
		for (size_t i = 0; i < materialsList.size(); ++i) {
			const CMaterialData& matD = materialsList[i];
			usdExport.appendNodeMaterial(matD);
		}
	}

	// アニメーション情報を出力.
	if (m_exportParam.optOutputAnimation) {
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
			if ((nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::null_node || (nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::ball_joint_node) {
				CNodeNullData& nodeD = static_cast<CNodeNullData &>(nodeBaseD);

				// モーション情報を持つ場合、モーション要素にnodeD.matrixを乗算する.
				if (nodeD.jointMotion.hasMotion()) {
					if (!nodeD.jointMotion.translations.empty()) {
						for (size_t j = 0; j < nodeD.jointMotion.translations.size(); ++j) {
							CJointTranslationData& transD = nodeD.jointMotion.translations[j];
							sxsdk::vec3 v(transD.x, transD.y, transD.z);
							v = v * nodeD.matrix;
							transD.x = v.x;
							transD.y = v.y;
							transD.z = v.z;
						}
					}

					if (!nodeD.jointMotion.rotations.empty()) {
						sxsdk::mat4 m = nodeD.matrix;
						m[3][0] = m[3][1] = m[3][2] = 0.0f;
						sxsdk::vec3 eularV;
						for (size_t j = 0; j < nodeD.jointMotion.rotations.size(); ++j) {
							CJointRotationData& rotD = nodeD.jointMotion.rotations[j];
							sxsdk::quaternion_class q(-rotD.w, sxsdk::vec3(rotD.x, rotD.y, rotD.z));

							q = sxsdk::quaternion_class(sxsdk::mat4(q) * m);
							rotD.x = q.x;
							rotD.y = q.y;
							rotD.z = q.z;
							rotD.w = -q.w;
							q.get_euler(eularV);
						}
					}

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
		if (imageD.fileName != "" && imageD.pMasterImageHandle) {
			try {
				const std::string fileName = fileDir + StringUtil::getFileSeparator() + imageD.fileName;
				sxsdk::master_image_class& masterImage = m_pScene->get_shape_by_handle(imageD.pMasterImageHandle)->get_master_image();
				compointer<sxsdk::image_interface> image(masterImage.get_image());
				if (image) {
					// テクスチャのピクセルを加工する場合.
					if (imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_r ||
						imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_g ||
						imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_b) {

						compointer<sxsdk::image_interface> image2(Shade3DUtil::createImageWithTransform(image, imageD.textureSource, imageD.texTransform));
						image2->save(fileName.c_str());

					} else {
						if (!imageD.texTransform.isDefault()) {		// 変換要素がある場合.
							compointer<sxsdk::image_interface> image2(Shade3DUtil::createImageWithTransform(image, imageD.textureSource, imageD.texTransform));
							image2->save(fileName.c_str());

						} else {
							image->save(fileName.c_str());
						}
					}
					// USDZ出力時のためのファイル名保持.
					m_exportFilesList.push_back(fileName);
				}
			} catch (...) { }
		}
	}
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

