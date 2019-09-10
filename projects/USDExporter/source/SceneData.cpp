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
	m_pScene = NULL;
	filePath = "";
	m_findNames.clear();
	m_findImageFileNames.clear();
	m_elementsList.clear();
	tmpMeshData.clear();
	nodesList.clear();
	materialsList.clear();
	imagesList.clear();
	m_exportFilesList.clear();
}

/**
 * シーンクラスを受け取る.
 */
void CSceneData::setSceneInterface (sxsdk::scene_interface* scene)
{
	m_pScene = scene;
}

/**
 * エクスポートパラメータを指定.
 */
void CSceneData::setExportParam (const CExportParam& exportParam)
{
	m_exportParam = exportParam;
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
 * @param[in] isBone    ボーンの場合はtrue.
 */
void CSceneData::appendNodeNull (sxsdk::shape_class* shape, const std::string& namePath, const sxsdk::mat4& matrix, const bool isBone)
{
	nodesList.push_back(std::make_shared<CNodeNullData>());

	CNodeNullData& nodeD = static_cast<CNodeNullData &>(*(nodesList.back()));
	nodeD.name     = namePath;
	nodeD.matrix   = matrix;
	nodeD.nodeType = isBone ? USD_DATA::NODE_TYPE::bone_node : USD_DATA::NODE_TYPE::null_node;
	nodeD.shapeHandle = shape->get_handle();

	m_getJointMotionData(shape, nodeD);
}

/**
 * ボーンのジョイントモーション情報を取得.
 */
void CSceneData::m_getJointMotionData (sxsdk::shape_class* shape, CNodeNullData& nodeD)
{
	nodeD.jointMotion.clear();
	if (!Shade3DUtil::isBone(*shape)) return;

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

	try {
		if (!shape->has_motion()) return;

		const sxsdk::vec3 boneWCenter = Shade3DUtil::getBoneCenter(*shape, NULL);
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

	getMaterialFromShape(shape, materialD);		// shapeの形状に割り当てられているマテリアルを取得.
	tmpMaterials.push_back(materialD);
	tmpMaterialMasterSurfaces.push_back(NULL);

	// フェイスグループのマテリアル情報を取得.
	if (shape->get_type() == sxsdk::enums::polygon_mesh) {
		sxsdk::polygon_mesh_class& pMesh = shape->get_polygon_mesh();
		const int fgCou = pMesh.get_number_of_face_groups();
		for (int i = 0; i < fgCou; ++i) {
			sxsdk::master_surface_class* pMasterSurface = pMesh.get_face_group_surface(i);
			if (pMasterSurface) {
				if (m_getMaterialFromMasterSurface(pMasterSurface, NULL, materialD)) {
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
			if ((nodeBaseD.nodeType) == USD_DATA::NODE_TYPE::null_node) {
				CNodeNullData& nodeD = static_cast<CNodeNullData &>(nodeBaseD);

				// 変換行列.
				const USD_DATA::NodeMatrixData usdMatrix = m_convMatrix(nodeD.matrix);

				usdExport.appendNodeNull(nodeD.name, usdMatrix);

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
	for (size_t i = 0; i < imagesList.size(); ++i) {
		const CImageData& imageD = imagesList[i];
		if (imageD.fileName != "" && imageD.pMasterImageHandle) {
			try {
				const std::string fileName = fileDir + std::string("/") + imageD.fileName;
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

						// USDZ出力時のためのファイル名保持。画像の場合は、ファイル名のみを追加.
//						m_exportFilesList.push_back(imageD.fileName);
						m_exportFilesList.push_back(fileName);
					}
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
}

/**
 * 同一のマスターイメージがすでにimagesListに格納済みか.
 * @param[in]  masterImage     追加するMasterImage.
 * @param[in]  texTransform    マッピングの変換情報.
 * @param[in]  channelMix      mapping_layerのchanelMixの指定.
 */
int CSceneData::m_findMasterImageInImagesList (sxsdk::master_image_class* masterImage, const CTextureTransform& texTransform, const int channelMix)
{
	if (!masterImage || imagesList.empty()) return -1;
	void* mHandle = masterImage->get_handle();

	int retI = -1;
	for (size_t i = 0; i < imagesList.size(); ++i) {
		const CImageData& imageD = imagesList[i];

		// 有効なRGBA要素をチェック.
		bool chkF = false;
		switch (channelMix) {
			case sxsdk::enums::mapping_grayscale_red_mode:
				if (imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_r && imageD.texTransform.isSame(texTransform)) {
					chkF = true;
				}
				break;

			case sxsdk::enums::mapping_grayscale_green_mode:
				if (imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_g && imageD.texTransform.isSame(texTransform)) {
					chkF = true;
				}
				break;

			case sxsdk::enums::mapping_grayscale_blue_mode:
				if (imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_b && imageD.texTransform.isSame(texTransform)) {
					chkF = true;
				}
				break;

			case sxsdk::enums::mapping_grayscale_alpha_mode:
				if (imageD.textureSource == USD_DATA::TEXTURE_SOURE::texture_source_a && imageD.texTransform.isSame(texTransform)) {
					chkF = true;
				}
				break;

			default:
				if (imageD.texTransform.isSame(texTransform)) chkF = true;
		}
		if (!chkF) continue;

		if (imageD.pMasterImageHandle == mHandle) {
			retI = (int)i;
			break;
		}
	}

	return retI;
}

/**
 * 指定の形状から、表面材質情報を取得.
 * なお、その形状が表面材質を持たない場合は親をたどる.
 * @param[in]  shape         対象形状.
 * @param[out] materialDat   マテリアル情報が返る.
 * @return マテリアル情報を取得した場合はtrue.
 */
bool CSceneData::getMaterialFromShape (sxsdk::shape_class* shape, CMaterialData& materialDat)
{
	materialDat.clear();
	materialDat.name = std::string(MATERIAL_ROOT_PATH) + std::string("/material");

	// 表面材質情報を持つ形状をたどる (なければshape自身).
	sxsdk::shape_class* pS = Shade3DUtil::getHasSurfaceParentShape(shape);
	if (!(pS->has_surface())) return false;

	sxsdk::master_surface_class* pMasterSurface = pS->get_master_surface();

	return m_getMaterialFromMasterSurface(pMasterSurface, pS->get_surface(), materialDat);
}

/**
 * 指定のマスターサーフェスから、表面材質情報を取得.
 * @param[in]  masterSurface  対象のマスターサーフェス.
 * @param[in]  surface        masterSurfaceがnullの場合はこのsurfaceを参照する.
 * @param[out] materialDat   マテリアル情報が返る.
 * @return マテリアル情報を取得した場合はtrue.
 */
bool CSceneData::m_getMaterialFromMasterSurface (sxsdk::master_surface_class* masterSurface, sxsdk::surface_class* surface, CMaterialData& materialDat)
{
	materialDat.clear();
	materialDat.name = std::string(MATERIAL_ROOT_PATH) + std::string("/material");

	try {
		// マスターサーフェス名を取得.
		if (masterSurface) {
			std::string mName = std::string(masterSurface->get_name());
			materialDat.name = std::string(MATERIAL_ROOT_PATH) + std::string("/") + mName;
			materialDat.pMasterSurfaceHandle = masterSurface->get_handle();

			// doublesidedが含まれる場合は、doubleSidedを持つとする.
			std::transform(mName.begin(), mName.end(), mName.begin(), ::tolower);
			if (mName.find("doublesided") != std::string::npos) {
				materialDat.doubleSided = true;
			}
			surface = masterSurface->get_surface();
		}
		if (!surface) return false;

		// 表面材質情報を取得.
		if (surface->get_has_diffuse()) {
			const sxsdk::rgb_class col = (surface->get_diffuse_color()) * (surface->get_diffuse());
			materialDat.diffuseColor[0] = col.red;
			materialDat.diffuseColor[1] = col.green;
			materialDat.diffuseColor[2] = col.blue;
		}

		if (surface->get_has_glow()) {
			const sxsdk::rgb_class col = (surface->get_glow_color()) * (surface->get_glow());
			materialDat.emissiveColor[0] = col.red;
			materialDat.emissiveColor[1] = col.green;
			materialDat.emissiveColor[2] = col.blue;
		}

		if (surface->get_has_reflection()) {
			materialDat.metallic = surface->get_reflection();
		}

		if (surface->get_has_roughness()) {
			materialDat.roughness = surface->get_roughness();
		}
		if (surface->get_has_transparency()) {
			materialDat.opacity = 1.0f - (surface->get_transparency());
		}
		if (surface->get_has_refraction()) {
			materialDat.ior = surface->get_refraction();
		}

		const int mappingLayersCou = surface->get_number_of_mapping_layers();
		if (!(surface->get_has_mapping_layers()) || mappingLayersCou == 0) return true;

		// テクスチャを取得.
		for (int mLoop = 0; mLoop < mappingLayersCou; ++mLoop) {
			sxsdk::mapping_layer_class& mappingLayer = surface->mapping_layer(mLoop);
			const int mType = mappingLayer.get_type();
			CTextureTransform texTransform;
			texTransform.flipColor = mappingLayer.get_flip_color();

			// オクルージョンレイヤの場合.
			if (Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) {
				const float weight = mappingLayer.get_weight();
				texTransform.occlusion = true;
				texTransform.textureWeight = weight;
				m_setTextureMappingData(mappingLayer, texTransform, materialDat.occlusionTexture, true);
			}

			if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;

			if (mType == sxsdk::enums::diffuse_mapping) {
				// アルファ透明を使用.
				if (mappingLayer.get_channel_mix() == sxsdk::enums::mapping_transparent_alpha_mode) {
					materialDat.useDiffuseAlpha = true;
				}
				texTransform.multiR = materialDat.diffuseColor[0];
				texTransform.multiG = materialDat.diffuseColor[1];
				texTransform.multiB = materialDat.diffuseColor[2];
				m_setTextureMappingData(mappingLayer, texTransform, materialDat.diffuseTexture, materialDat.useDiffuseAlpha);
			}

			if (mType == sxsdk::enums::glow_mapping) {
				texTransform.multiR = materialDat.emissiveColor[0];
				texTransform.multiG = materialDat.emissiveColor[1];
				texTransform.multiB = materialDat.emissiveColor[2];
				m_setTextureMappingData(mappingLayer, texTransform, materialDat.emissiveTexture);
			}
			if (mType == sxsdk::enums::normal_mapping) {
				const float weight = mappingLayer.get_weight();
				texTransform.textureNormal = true;
				texTransform.textureWeight = weight;
				m_setTextureMappingData(mappingLayer, texTransform, materialDat.normalTexture);
			}
			if (mType == sxsdk::enums::roughness_mapping) {
				texTransform.multiR = texTransform.multiG = texTransform.multiB = materialDat.roughness;
				m_setTextureMappingData(mappingLayer, texTransform, materialDat.roughnessTexture);
			}
			if (mType == sxsdk::enums::reflection_mapping) {
				texTransform.multiR = texTransform.multiG = texTransform.multiB = materialDat.metallic;
				m_setTextureMappingData(mappingLayer, texTransform, materialDat.metallicTexture);
			}
		}

	} catch (...) { }

	return true;
}

/**
 * テクスチャマッピング情報を追加.
 * @param[in]  mappingLayer    マッピングレイヤ情報.
 * @param[in]  texTransform    マッピングの変換情報.
 * @param[out] texMappingData  マッピング情報の格納先.
 */
void CSceneData::m_setTextureMappingData (sxsdk::mapping_layer_class& mappingLayer, const CTextureTransform& texTransform, CTextureMappingData& texMappingData, 
	const bool occlusionF)
{
	if (texMappingData.fileName != "") return;

	try {
		compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
		if (!image) return;
		if (!image->has_image()) return;

		// imageからマスターイメージを取得.
		sxsdk::master_image_class* masterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);
		if (!masterImage) return;

		std::string masterImageName = "";
		const int channelMix = mappingLayer.get_channel_mix();

		// 同一のマスターイメージが格納済みか.
		int imageIndex = m_findMasterImageInImagesList(masterImage, texTransform, channelMix);
		if (imageIndex >= 0) {
			const CImageData& imageD = imagesList[imageIndex];
			masterImageName = imageD.fileName;
		} else {
			// イメージ名をASCIIのファイル名にする.
			masterImageName = masterImage->get_name();
			if (!StringUtil::checkASCII(masterImageName)) {
				masterImageName = "texture";
			}

			// エクスポートオプションm_exportParam.optTextureTypeにより、pngにするかjpgにするか決める.
			if (m_exportParam.optTextureType == USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_use_image_name) {
				// 拡張子がある場合はそれを採用し、ない場合はpngにする.
				masterImageName = StringUtil::SetFileImageExtension(masterImageName, "png");

			} else {
				const bool usePng = (m_exportParam.optTextureType == USD_DATA::EXPORT::TEXTURE_TYPE::texture_type_replace_png);
				masterImageName = StringUtil::SetFileImageExtension(masterImageName, usePng ? "png" : "jpg", true);
			}

			// ユニークファイル名として追加.
			// 同一名がある場合は、連番付きで返す.
			masterImageName = m_findImageFileNames.appendName(masterImageName, USD_DATA::NODE_TYPE::texture_node, true);

			imageIndex = (int)imagesList.size();
			imagesList.push_back(CImageData());
		}

		if (imageIndex >= 0) {
			CImageData& imageD = imagesList[imageIndex];
			imageD.pMasterImageHandle = masterImage->get_handle();
			imageD.fileName = masterImageName;
			imageD.occlusionF = occlusionF;

			texMappingData.fileName = masterImageName;
			texMappingData.textureParam.imageIndex = imageIndex;
			texMappingData.textureParam.uvLayerIndex = mappingLayer.get_uv_mapping();
			if (texMappingData.textureParam.uvLayerIndex < 0 || texMappingData.textureParam.uvLayerIndex > 1) {
				texMappingData.textureParam.uvLayerIndex = 0;
			}
			if (occlusionF) {
				COcclusionShaderData occlusionShaderD;
				if (StreamCtrl::loadOcclusionParam(mappingLayer, occlusionShaderD)) {
					texMappingData.textureParam.uvLayerIndex = occlusionShaderD.uvIndex;
				}
			}

			texMappingData.textureParam.repeatU = mappingLayer.get_repetition_x();
			texMappingData.textureParam.repeatV = mappingLayer.get_repetition_y();

			switch (channelMix) {
			case sxsdk::enums::mapping_grayscale_red_mode:
				texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_r;
				imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_r;
				break;

			case sxsdk::enums::mapping_grayscale_green_mode:
				texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_g;
				imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_g;
				break;

			case sxsdk::enums::mapping_grayscale_blue_mode:
				texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_b;
				imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_b;
				break;

			case sxsdk::enums::mapping_grayscale_alpha_mode:
				texMappingData.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_a;
				imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_a;
				break;

			default:
				imageD.textureSource = USD_DATA::TEXTURE_SOURE::texture_source_rgb;
			}
			imageD.texTransform = texTransform;
		}
	} catch (...) { }
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

