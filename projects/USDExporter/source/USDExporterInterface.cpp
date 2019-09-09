/**
 * USD Exporter Interface.
 */

#include "USDExporterInterface.h"
#include "USDExporter.h"
#include "USDData.h"
#include "Shade3DUtil.h"
#include "StreamCtrl.h"
#include "MathUtil.h"

// ダイアログボックスのパラメータ.
enum {
	dlg_file_export_type = 101,				// 出力形式.
	dlg_file_usdz = 102,					// usdzを出力.
	dlg_option_texture = 201,				// テクスチャ出力.
	dlg_option_max_texture_size = 202,		// 最大テクスチャサイズ.
	dlg_option_bone_skin = 203,				// ボーンとスキンを出力.
	dlg_option_vertex_color = 204,			// 頂点カラーを出力.
	dlg_option_animation = 205,				// アニメーションを出力.
	dlg_option_subdivision = 206,			// Subdivision情報を出力.
};

CUSDExporterInterface::CUSDExporterInterface (sxsdk::shade_interface& shade) : shade(shade)
{
	m_dlgOK = false;
	m_oldSequenceMode = false;
	m_oldDirty = false;
}

CUSDExporterInterface::~CUSDExporterInterface ()
{
}

/**
 * ファイル拡張子.
 */
const char *CUSDExporterInterface::get_file_extension (void *)
{
	return (m_exportParam.exportFileType == USD_DATA::EXPORT::FILE_TYPE::file_type_usda) ? "usda" : "usdc";
}

/**
 * ファイルの説明文.
 */
const char *CUSDExporterInterface::get_file_description (void *)
{
	return "USD (Universal Scene Description)";
}

/**
 * シーンを開いたときに呼ばれる.
 */
void CUSDExporterInterface::scene_opened (bool &b, sxsdk::scene_interface *scene, void *)
{
	try {
		if (scene) {
			// streamから、ダイアログボックスのパラメータを取得.
			StreamCtrl::loadExportDialogParam(shade, m_exportParam);
		}
	} catch (...) { }
}


/**
 * エクスポート処理を行う.
 */
void CUSDExporterInterface::do_export (sxsdk::plugin_exporter_interface *plugin_exporter, void *)
{
	compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());

	// streamに、ダイアログボックスのパラメータを保存.
	StreamCtrl::saveExportDialogParam(shade, m_exportParam);

	{
		CUSDExporter usdExp;
		usdExp.testCreateSphereWithAnimation("K:\\Modeling\\USD\\Shade3D\\OcclusionTest\\anim.usda");
	}

	m_sceneData.clear();
	m_sceneData.setSceneInterface(scene);
	m_sceneData.setExportParam(m_exportParam);

	try {
		m_pluginExporter = plugin_exporter;
		m_pluginExporter->AddRef();

		m_stream      = m_pluginExporter->get_stream_interface();
		m_text_stream = m_pluginExporter->get_text_stream_interface();
	} catch(...) { }

	// 出力先のファイルパス.
	m_orgFilePath = m_pluginExporter->get_file_path();

	m_shapeStack.clear();
	m_currentDepth = 0;
	m_pScene = scene;

	shade.message("----- USD Exporter -----");

	// シーケンスモード時は、出力時にシーケンスOffにして出す.
	// シーンの変更フラグは後で元に戻す.
	m_oldSequenceMode = m_pScene->get_sequence_mode();
	m_oldDirty = m_pScene->get_dirty();
	if (m_exportParam.optOutputBoneSkin) {
		if (m_oldSequenceMode) {
			m_pScene->set_sequence_mode(false);
		}
	}

	// エクスポートを開始.
	plugin_exporter->do_export();
}

/**
 * trueを返す場合、ポリゴンメッシュの面はSubdivisionされる (デフォルトtrue).
 */
bool CUSDExporterInterface::must_round_polymesh (void *)
{
	return !m_exportParam.optSubdivision;
}

/**
 * エクスポートの開始.
 */
void CUSDExporterInterface::start (void *)
{
}

/**
 * エクスポートの終了
 */
void CUSDExporterInterface::finish (void *)
{
}

/**
 * エクスポート処理が完了した後に呼ばれる.
 * ここで、ファイル出力するとstreamとかぶらない.
 */
void CUSDExporterInterface::clean_up (void *)
{
	// USDのファイルの種類により拡張子を変える.
	std::string filePath2 = m_orgFilePath;
	{
		std::string fileExt;
		filePath2 = m_orgFilePath;
		fileExt = "";
		int iPos = m_orgFilePath.find_last_of(".");
		if (iPos != std::string::npos) {
			filePath2 = m_orgFilePath.substr(0, iPos);
			fileExt = m_orgFilePath.substr(iPos + 1);
		}

		if (m_exportParam.exportFileType == USD_DATA::EXPORT::FILE_TYPE::file_type_usda) {
			filePath2 = filePath2 + std::string(".usda");
		} else {
			filePath2 = filePath2 + std::string(".usdc");
		}
	}

	// USDファイルを出力.
	m_sceneData.exportUSD(shade, filePath2);

	// USDZファイルを出力.
	if (m_exportParam.exportUSDZ) {
		// usdzのファイルパス.
		std::string usdzFilePath, fileExt;
		usdzFilePath = m_orgFilePath;
		fileExt = "";
		int iPos = m_orgFilePath.find_last_of(".");
		if (iPos != std::string::npos) {
			usdzFilePath = usdzFilePath.substr(0, iPos);
			fileExt = m_orgFilePath.substr(iPos + 1);
		}
		usdzFilePath = usdzFilePath + std::string(".usdz");

		m_sceneData.exportUSDZ(usdzFilePath);		// usdzファイルとして出力.
	}

	shade.message(filePath2);

	// 元のシーケンスモードに戻す.
	if (m_exportParam.optOutputBoneSkin) {
		if (m_oldSequenceMode) {
			m_pScene->set_sequence_mode(m_oldSequenceMode);
		}
	}

	m_pScene->set_dirty(m_oldDirty);
}

/**
 * カレント形状の処理の開始.
 */
void CUSDExporterInterface::begin (void *)
{
	m_pCurrentShape = NULL;
	m_skip = false;
	m_curShapeHasSubdivision = false;

	// カレントの形状管理クラスのポインタを取得.
	m_pCurrentShape        = m_pluginExporter->get_current_shape();
	const sxsdk::mat4 gMat = m_pluginExporter->get_transformation();
	const std::string name(m_pCurrentShape->get_name());

	m_shapeStack.push(m_currentDepth, m_pCurrentShape, gMat);

	const int type = m_pCurrentShape->get_type();

	// 面の反転フラグ.
	m_flipFace = m_shapeStack.isFlipFace();

	// 掃引体の場合は面が反転しているので、反転.
	if (type == sxsdk::enums::line) {
		sxsdk::line_class& lineC = m_pCurrentShape->get_line();
		if (lineC.is_extruded()) {
			m_flipFace = !m_flipFace;
		}
	}
	if (type == sxsdk::enums::disk) {
		sxsdk::disk_class& diskC = m_pCurrentShape->get_disk();
		if (diskC.is_extruded()) {
			m_flipFace = !m_flipFace;
		}
	}

	m_spMat = sxsdk::mat4::identity;
	m_currentLWMatrix = gMat;
	m_currentDepth++;

	// 形状情報を追加.
	// 戻り値は、USDのパスとしての名前.
	m_currentPathName = m_sceneData.appendShape(m_pCurrentShape);

	if (type == sxsdk::enums::light || type == sxsdk::enums::camera_joint || type == sxsdk::enums::master_surface || type == sxsdk::enums::master_image) {
		m_skip = true;
		return;
	}
	if (type == sxsdk::enums::part) {
		const int partType = m_pCurrentShape->get_part().get_part_type();
		if (partType == sxsdk::enums::master_image_part || partType == sxsdk::enums::master_surface_part) {
			m_skip = true;
			return;
		}
	}

	if (type == sxsdk::enums::line) {
		// 開いた線形状の場合.
		if (!m_pCurrentShape->get_line().get_closed()) {
			m_skip = true;
			return;
		}
	}

	if (!m_skip) {
		// 形状がメッシュに変換できない場合は、NULLノードとする.
		if (!m_sceneData.checkConvertMesh(m_pCurrentShape)) {
			// Shade3Dでのmm単位をcmに変換.
			const sxsdk::mat4 m = Shade3DUtil::convUnit_mm_to_cm(m_pCurrentShape->get_transformation());

			m_sceneData.appendNodeNull(m_pCurrentShape, m_currentPathName, m, Shade3DUtil::isBone(*m_pCurrentShape));
		}
	}
}

/**
 * カレント形状の処理の終了.
 */
void CUSDExporterInterface::end (void *)
{
	if (!m_skip) {
	}

	m_shapeStack.pop();
	m_pCurrentShape = NULL;

	sxsdk::shape_class *pShape;
	sxsdk::mat4 lwMat;
	int depth = 0;

	m_shapeStack.getShape(&pShape, &lwMat, &depth);
	if (pShape && depth > 0) {
		m_pCurrentShape  = pShape;
		m_currentDepth   = depth;
		m_currentLWMatrix = inv(pShape->get_transformation()) * m_shapeStack.getLocalToWorldMatrix();

		// 面の反転フラグ.
		m_flipFace = m_shapeStack.isFlipFace();
	}

}

/**
 * カレント形状が掃引体の上面部分の場合、掃引に相当する変換マトリクスが渡される.
 */
void CUSDExporterInterface::set_transformation (const sxsdk::mat4 &t, void *)
{
	m_spMat = t;
	m_flipFace = !m_flipFace;	// 掃引体の場合、上ふたの面が反転するため面反転を行う.
}

/**
 * カレント形状が掃引体の上面部分の場合の行列クリア.
 */
void CUSDExporterInterface::clear_transformation (void *)
{
	m_spMat = sxsdk::mat4::identity;
	m_flipFace = !m_flipFace;	// 面反転を戻す.
}

/**
 * ポリゴンメッシュの開始時に呼ばれる.
 */
void CUSDExporterInterface::begin_polymesh (void *)
{
	if (m_skip) return;
	m_currentFaceGroupIndex = -1;
	m_faceGroupCount = 0;

	m_LWMat = m_spMat * m_currentLWMatrix;
	m_WLMat = inv(m_LWMat);

	m_sceneData.tmpMeshData.clear();

	// サブディビジョン情報を持つか.
	m_curShapeHasSubdivision = false;
	if (m_pCurrentShape->get_type() == sxsdk::enums::polygon_mesh) {
		sxsdk::polygon_mesh_class& pMesh = m_pCurrentShape->get_polygon_mesh();
		if (pMesh.get_roundness_type() != 0) {
			m_curShapeHasSubdivision = true;
		}
	}
}

/**
 * ポリゴンメッシュの頂点情報格納時に呼ばれる.
 */
void CUSDExporterInterface::begin_polymesh_vertex (int n, void *)
{
	if (m_skip) return;

	m_sceneData.tmpMeshData.vertices.resize(n);
	m_sceneData.tmpMeshData.skinJointsHandle.clear();
	m_sceneData.tmpMeshData.skinWeights.clear();
}

/**
 * 頂点が格納されるときに呼ばれる.
 */
void CUSDExporterInterface::polymesh_vertex (int i, const sxsdk::vec3 &v, const sxsdk::skin_class *skin)
{
	if (m_skip) return;

	sxsdk::vec3 pos = v;
	if (skin) {
		// スキン変換前の座標値を計算.
		if (m_exportParam.optOutputBoneSkin) {
			const sxsdk::mat4 skin_m = skin->get_skin_world_matrix();
			const sxsdk::mat4 skin_m_inv = inv(skin_m);
			sxsdk::vec4 v4 = sxsdk::vec4(pos, 1) * m_LWMat * skin_m_inv;
			pos = sxsdk::vec3(v4.x, v4.y, v4.z);
		}
	}

	// 頂点の座標変換 (Shade3Dはmm、USDはcm).
	const sxsdk::vec3 v2 = Shade3DUtil::convUnit_mm_to_cm(pos);

	m_sceneData.tmpMeshData.vertices[i] = v2;

	if (m_exportParam.optOutputBoneSkin) {
		if (skin && m_pCurrentShape->get_skin_type() == 1) {		// スキンを持っており、頂点ブレンドで格納されている場合.
			const int bindsCou = skin->get_number_of_binds();
			if (bindsCou > 0) {
				// スキンの影響ジョイント(bone/ball)とweight値を格納.
				m_sceneData.tmpMeshData.skinJointsHandle.push_back(sx::vec<void*,4>(NULL, NULL, NULL, NULL));
				m_sceneData.tmpMeshData.skinWeights.push_back(sxsdk::vec4(0, 0, 0, 0));
				for (int j = 0; j < bindsCou && j < 4; ++j) {
					const sxsdk::skin_bind_class& skinBind = skin->get_bind(j);
					m_sceneData.tmpMeshData.skinJointsHandle[i][j] = skinBind.get_shape()->get_handle();
					m_sceneData.tmpMeshData.skinWeights[i][j]      = skinBind.get_weight();
				}

				if (bindsCou < 4) {
					// ボーンの親をたどり、skinのweight 0(スキンの影響なし)として割り当てる.
					// これは、出力時にスキン対象とするボーン(node)を認識しやすくするため.
					std::vector<sxsdk::shape_class *> shapeList;
					const sxsdk::skin_bind_class& skinBind = skin->get_bind(bindsCou - 1);
					sxsdk::shape_class* pS = skinBind.get_shape();
					if (pS->has_dad()) {
						pS = pS->get_dad();
						while ((pS->get_part().get_part_type()) == sxsdk::enums::bone_joint || (pS->get_part().get_part_type()) == sxsdk::enums::ball_joint) {
							shapeList.push_back(pS);
							if (shapeList.size() > 3) break;
							if (!pS->has_dad()) break;
							pS = pS->get_dad();
						}
					}
					for (int j = 0; j + bindsCou < 4 && j < shapeList.size(); ++j) {
						m_sceneData.tmpMeshData.skinJointsHandle[i][j + bindsCou] = shapeList[j];
						m_sceneData.tmpMeshData.skinWeights[i][j + bindsCou]      = 0.0f;
					}
				}

				// skinWeights[]が大きい順に並び替え.
				for (int j = 0; j < 4; ++j) {
					for (int k = j + 1; k < 4; ++k) {
						if (m_sceneData.tmpMeshData.skinWeights[i][j] < m_sceneData.tmpMeshData.skinWeights[i][k]) {
							std::swap(m_sceneData.tmpMeshData.skinWeights[i][j], m_sceneData.tmpMeshData.skinWeights[i][k]);
							std::swap(m_sceneData.tmpMeshData.skinJointsHandle[i][j], m_sceneData.tmpMeshData.skinJointsHandle[i][k]);
						}
					}
				}

				// Weight値の合計が1.0になるように正規化.
				float sumWeight = 0.0f;
				for (int j = 0; j < 4; ++j) sumWeight += m_sceneData.tmpMeshData.skinWeights[i][j];
				if (!MathUtil::isZero(sumWeight) && !MathUtil::isZero(1.0f - sumWeight)) {
					for (int j = 0; j < 4; ++j) m_sceneData.tmpMeshData.skinWeights[i][j] /= sumWeight;
				}
			}
		}
	}
}

/**
 * ポリゴンメッシュの面情報が格納されるときに呼ばれる（Shade12の追加機能）.
 */
void CUSDExporterInterface::polymesh_face_uvs (int n_list, const int list[], const sxsdk::vec3 *normals, const sxsdk::vec4 *plane_equation, const int n_uvs, const sxsdk::vec2 *uvs, void *)
{
	if (m_skip) return;
	if (n_list <= 2) return;

	m_sceneData.tmpMeshData.faceVertexCounts.push_back(n_list);

	if (list) {
		for (int i = 0; i < n_list; ++i) {
			m_sceneData.tmpMeshData.faceIndices.push_back(list[i]);
		}
	}

	if (normals) {
		for (int i = 0; i < n_list; ++i) {
			m_sceneData.tmpMeshData.faceNormals.push_back(normals[i]);
		}
	}

	m_sceneData.tmpMeshData.faceFaceGroupIndex.push_back(m_currentFaceGroupIndex);
	if (m_currentFaceGroupIndex >= 0) {
		m_sceneData.tmpMeshData.faceGroupFacesCount[m_currentFaceGroupIndex]++;
	}

	// USDの場合は、V値が逆転している.
	if (n_uvs >= 1 && uvs) {
		for (int i = 0, iPos = 0; i < n_list; ++i, iPos += n_uvs) {
			sxsdk::vec2 uv = uvs[iPos];
			uv.y = 1.0f - uv.y;
			m_sceneData.tmpMeshData.faceUV0.push_back(uv);
		}
		if (n_uvs >= 2) {
			for (int i = 0, iPos = 1; i < n_list; ++i, iPos += n_uvs) {
				sxsdk::vec2 uv = uvs[iPos];
				uv.y = 1.0f - uv.y;
				m_sceneData.tmpMeshData.faceUV1.push_back(uv);
			}
		}
	}
}

/**
 * ポリゴンメッシュの終了時に呼ばれる.
 */
void CUSDExporterInterface::end_polymesh (void *)
{
	if (m_skip) return;

	m_sceneData.tmpMeshData.flipFaces = m_flipFace;
	m_sceneData.tmpMeshData.subdivision = m_exportParam.optSubdivision && m_curShapeHasSubdivision;

	// メッシュ情報を格納.
	if (!m_sceneData.tmpMeshData.vertices.empty()) {
		// Shade3Dでのmm単位をcmに変換.
		sxsdk::mat4 m = sxsdk::mat4::identity;
		if (m_pCurrentShape->get_type() != sxsdk::enums::polygon_mesh) {
			m = m_spMat * (m_pCurrentShape->get_transformation());
			m = Shade3DUtil::convUnit_mm_to_cm(m);
		}

		// フェイスグループ情報を取得.
		if (m_faceGroupCount > 0 && (m_pCurrentShape->get_type()) == sxsdk::enums::polygon_mesh) {
			sxsdk::polygon_mesh_class& pMesh = m_pCurrentShape->get_polygon_mesh();
			if (pMesh.get_number_of_face_groups() == m_faceGroupCount) {
				m_sceneData.tmpMeshData.faceGroupMasterSurfaces.resize(m_faceGroupCount);
				for (int i = 0; i < m_faceGroupCount; ++i) {
					m_sceneData.tmpMeshData.faceGroupMasterSurfaces[i] = pMesh.get_face_group_surface(i);

					// 面を持つか.
					const int facesCou = m_sceneData.tmpMeshData.faceGroupFacesCount[i];
					if (facesCou == 0) {
						m_sceneData.tmpMeshData.faceGroupMasterSurfaces[i] = NULL;
					}
				}
			}
		}

		m_sceneData.appendNodeMesh(m_pCurrentShape, m_currentPathName, m, m_sceneData.tmpMeshData);
	}
}

/**
 * 面情報格納前に呼ばれる.
 */
void CUSDExporterInterface::begin_polymesh_face2 (int n, int number_of_face_groups, void *)
{
	if (m_skip) return;
	m_currentFaceGroupIndex = -1;
	m_faceGroupCount = number_of_face_groups;

	if (m_faceGroupCount > 0) {
		m_sceneData.tmpMeshData.faceGroupMasterSurfaces.resize(m_faceGroupCount);
		m_sceneData.tmpMeshData.faceGroupFacesCount.resize(m_faceGroupCount);
		for (int i = 0; i < m_faceGroupCount; ++i) {
			m_sceneData.tmpMeshData.faceGroupFacesCount[i] = 0;
		}
	}
}

/**
 * フェイスグループごとの面列挙前に呼ばれる.
 */
void CUSDExporterInterface::begin_polymesh_face_group (int face_group_index, void *)
{
	if (m_skip) return;
	m_currentFaceGroupIndex = face_group_index;
}

/**
 * フェイスグループごとの面列挙後に呼ばれる.
 */
void CUSDExporterInterface::end_polymesh_face_group (void *)
{
	if (m_skip) return;
}

void CUSDExporterInterface::initialize_dialog (sxsdk::dialog_interface& dialog, void *)
{
	// Streamから情報を取得.
	StreamCtrl::loadExportDialogParam(shade, m_exportParam);
}

void CUSDExporterInterface::load_dialog_data (sxsdk::dialog_interface &d,void *)
{
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_file_export_type));
		item->set_selection((int)m_exportParam.exportFileType);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_file_usdz));
		item->set_bool(m_exportParam.exportUSDZ);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_texture));
		item->set_selection((int)m_exportParam.optTextureType);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_max_texture_size));
		item->set_selection((int)m_exportParam.optMaxTextureSize);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_bone_skin));
		item->set_bool(m_exportParam.optOutputBoneSkin);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_vertex_color));
		item->set_bool(m_exportParam.optOutputVertexColor);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_animation));
		item->set_bool(m_exportParam.optOutputAnimation);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_subdivision));
		item->set_bool(m_exportParam.optSubdivision);
	}
}

void CUSDExporterInterface::save_dialog_data (sxsdk::dialog_interface &dialog,void *)
{
}

bool CUSDExporterInterface::respond (sxsdk::dialog_interface &dialog, sxsdk::dialog_item_class &item, int action, void *)
{
	const int id = item.get_id();

	if (id == sx::iddefault) {
		m_exportParam.clear();
		load_dialog_data(dialog);
		return true;
	}

	if (id == sx::idok) {
		m_dlgOK = true;
	}

	if (id == dlg_file_export_type) {
		m_exportParam.exportFileType = (USD_DATA::EXPORT::FILE_TYPE)item.get_selection();
		return true;
	}
	if (id == dlg_file_usdz) {
		m_exportParam.exportUSDZ = item.get_bool();
		return true;
	}
	if (id == dlg_option_texture) {
		m_exportParam.optTextureType = (USD_DATA::EXPORT::TEXTURE_TYPE)item.get_selection();
		return true;
	}
	if (id == dlg_option_max_texture_size) {
		m_exportParam.optMaxTextureSize = (USD_DATA::EXPORT::MAX_TEXTURE_SIZE)item.get_selection();
		return true;
	}
	if (id == dlg_option_bone_skin) {
		m_exportParam.optOutputBoneSkin = item.get_bool();
		return true;
	}
	if (id == dlg_option_vertex_color) {
		m_exportParam.optOutputVertexColor = item.get_bool();
		return true;
	}
	if (id == dlg_option_animation) {
		m_exportParam.optOutputAnimation = item.get_bool();
		return true;
	}
	if (id == dlg_option_subdivision) {
		m_exportParam.optSubdivision = item.get_bool();
		return true;
	}

	return false;
}

