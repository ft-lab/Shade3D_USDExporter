﻿/**
 * USD Exporter Interface.
 */

#include "USDExporterInterface.h"
#include "USDExporter.h"
#include "USDData.h"
#include "Shade3DUtil.h"
#include "StreamCtrl.h"
#include "StringUtil.h"
#include "MathUtil.h"

#include <time.h>

// ダイアログボックスのパラメータ.
enum {
	dlg_file_export_type = 101,				// 出力形式.
	dlg_file_usdz = 102,					// usdzを出力.
	dlg_file_export_apple_usdz = 103,		// Appleのusdz互換.
	dlg_file_export_output_temp_files = 104,	// usdz出力時に作業ファイルを出力.

	dlg_option_bone_skin = 203,				// ボーンとスキンを出力.
	dlg_option_vertex_color = 204,			// 頂点カラーを出力.
	dlg_option_subdivision = 206,			// Subdivision情報を出力.
	dlg_option_divide_poly_tri_quad = 207,	// 多角形を三角形/四角形に分割.
	dlg_option_divide_poly_tri = 208,		// 三角形に分割.
	dlg_option_kind = 209,					// Kind.

	dlg_option_texture = 301,				// テクスチャ出力.
	dlg_option_max_texture_size = 302,		// 最大テクスチャサイズ.
	dlg_option_texture_grayscale = 303,		// R/G/B/A指定をグレイスケールに分けて出力.
	dlg_option_bake_without_processing_textures_id = 305,	// テクスチャを加工せずにベイク.

	dlg_option_anim_keyframe_mode = 401,	// アニメーションのキーフレーム出力モード.
	dlg_option_anim_keyframe_step = 402,	// アニメーションのキーフレームのステップ数.

	dlg_material_shader_type = 501,				// USDでのShaderの種類.
	dlg_separateOpacityAndTransmission = 502,	// 「不透明(Opacity)」と「透明(Transmission)」を分ける.
	dlg_grayscale_texture_colorspace = 503,		// グレイスケールテクスチャのColor Space.
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
	if (m_exportParam.exportAppleUSDZ || (m_exportParam.exportUSDZ && !m_exportParam.exportOutputTempFiles)) return "usdz";
	if (m_exportParam.exportFileType == USD_DATA::EXPORT::FILE_TYPE::file_type_usda) return "usda";
	return "usdc";
}

/**
 * ファイルの説明文.
 */
const char *CUSDExporterInterface::get_file_description (void *)
{
	return "USD (Universal Scene Description)";
}

/**
 * 受け付けることのできるポリゴンメッシュ面の頂点の最大数.
 */
int CUSDExporterInterface::get_max_vertices_per_face (void *) {
	if (m_exportParam.exportAppleUSDZ) return 3;
	if (m_exportParam.optDividePolyTriQuad && m_exportParam.optDividePolyTri) return 3;
	if (m_exportParam.optDividePolyTriQuad) return 4;
	return 65535;
}

/**
 * ポリゴンメッシュの面を分割するか.
 * ここをfalseにしget_max_vertices_per_faceが4の場合は、極力4角形は保つ。5角形以上は4角形と3角形に分割される.
 */
bool CUSDExporterInterface::must_divide_polymesh (void *) {
	return false;
}

/**
 * ポリゴンメッシュの面は三角形分割する.
 */
bool CUSDExporterInterface::must_triangulate_polymesh (void *)
{
	if (m_exportParam.exportAppleUSDZ) return true;
	return (m_exportParam.optDividePolyTriQuad && m_exportParam.optDividePolyTri);
}

/**
 * アプリケーション終了時に呼ばれる.
 */
void CUSDExporterInterface::cleanup (void *)
{
	// 作業フォルダを削除.
	// 作業フォルダは、shade.get_temporary_path()で与えた後に削除すると、.
	// アプリ内では再度作業フォルダを作成できない。そのため、removeはアプリ終了時に呼んでいる.
	try {
		const std::string tempPath = std::string(shade.get_temporary_path("shade3d_temp_usd"));
		if (tempPath != "") {
			shade.remove_directory_and_files(tempPath.c_str());
		}
	} catch (...) { }
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

	m_sceneData.clear();
	m_sceneData.setupExport(scene, m_exportParam);
	m_warningCheck.clear();

	try {
		m_pluginExporter = plugin_exporter;
		m_pluginExporter->AddRef();

		m_stream      = m_pluginExporter->get_stream_interface();
		m_text_stream = m_pluginExporter->get_text_stream_interface();
	} catch(...) { }

	// 出力先のファイルパス.
	m_orgFilePath = m_pluginExporter->get_file_path();

	m_pScene = scene;
	m_traverseMasterObjectsMode = false;
	m_inMasterObjectPart = false;

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

	// リンクのマスター形状を取得.
	Shade3DUtil::getLinkMasterObjects(scene, m_linkMasterList);

	// マスターオブジェクト（外部参照含む）を持つか.
	m_hasMasterObject = false;
	for (size_t i = 0; i < m_linkMasterList.size(); ++i) {
		if (Shade3DUtil::checkInMasterObjectPart(*m_linkMasterList[i])) {
			m_hasMasterObject = true;
			break;
		}
	}

	//-----------------------------------.
	// エクスポートを開始.
	//-----------------------------------.
	if (m_hasMasterObject) {
		// マスターオブジェクトのみを格納.
		// ただし、マスターオブジェクトパート内は走査されないので(begin-endは呼ばれる)実質はパスを保持するだけ.
		m_shapeStack.clear();
		m_linkStack.clear();
		m_currentDepth = 0;

		m_traverseMasterObjectsMode = true;
		m_inMasterObjectPart = false;
		plugin_exporter->do_export();
	}

	// マスターオブジェクト以外を格納.
	{
		m_shapeStack.clear();
		m_linkStack.clear();
		m_currentDepth = 0;

		m_traverseMasterObjectsMode = false;
		m_inMasterObjectPart = false;
		plugin_exporter->do_export();
	}
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
	// マスターオブジェクトのみを格納する操作時は何もせずにスキップ.
	if (m_traverseMasterObjectsMode) return;

	// 警告のメッセージがある場合は表示.
	m_warningCheck.outputWarningMessage(shade);

	// 作業用のパス.
	const std::string tempPath = std::string(shade.get_temporary_path("shade3d_temp_usd"));

	// 出力ファイル名のみを取得.
	std::string sFileName = "";
	bool changedName = false;
	{
		sFileName = StringUtil::getFileName(m_orgFilePath);
		if (!StringUtil::checkASCII(sFileName)) {
			// ASCII名でない場合は、ファイル名を置き換え.
			char szStr[256];
			time_t t = time(NULL);
			strftime(szStr, sizeof(szStr), "%Y%m%d_%H%M%S", localtime(&t));

			sFileName = std::string("output_") + std::string(szStr);
			changedName = true;
		}
	}

	// 作業用ファイル名のフルパス.
	std::string tempFileName = "";
	{
		tempFileName = tempPath + StringUtil::getFileSeparator() + sFileName;
	}

	// USDのファイルの種類により拡張子を変える.
	std::string filePath2 = tempFileName;
	{
		std::string fileExt;
		fileExt = "";
		int iPos = tempFileName.find_last_of(".");
		if (iPos != std::string::npos) {
			filePath2 = tempFileName.substr(0, iPos);
			fileExt = tempFileName.substr(iPos + 1);
		}

		if (m_exportParam.exportFileType == USD_DATA::EXPORT::FILE_TYPE::file_type_usda && !m_exportParam.exportAppleUSDZ) {
			filePath2 = filePath2 + std::string(".usda");
		} else {
			filePath2 = filePath2 + std::string(".usdc");
		}

		sFileName = StringUtil::getFileName(filePath2);
	}

	// USDファイルを出力.
	m_sceneData.exportUSD(shade, filePath2);

	// USDZファイルを出力.
	std::string usdzFilePath = "";
	if (m_exportParam.exportUSDZ || m_exportParam.exportAppleUSDZ) {
		// usdzのファイルパス.
		std::string fileExt;
		usdzFilePath = tempFileName;
		fileExt = "";
		int iPos = tempFileName.find_last_of(".");
		if (iPos != std::string::npos) {
			usdzFilePath = usdzFilePath.substr(0, iPos);
			fileExt = tempFileName.substr(iPos + 1);
		}
		usdzFilePath = usdzFilePath + std::string(".usdz");

		m_sceneData.exportUSDZ(usdzFilePath);		// usdzファイルとして出力.
	}

	// Macでの対策.
	// Macでは、既存のファイルが存在すると上書きされない.
	try {
		shade.delete_file(m_orgFilePath.c_str());
	} catch (...) { }

	// 作業用ディレクトリから、m_orgFilePathのフォルダにコピー.
	{
		const std::string dstDir = StringUtil::getFileDir(m_orgFilePath);
		const std::vector<std::string> filesList = m_sceneData.getExportFilesList();
		for (size_t i = 0; i < filesList.size(); ++i) {
			const std::string srcPathName = filesList[i];
			const std::string srcName = StringUtil::getFileName(srcPathName);
			const std::string dstPathName = dstDir + StringUtil::getFileSeparator() + srcName;
			if (StringUtil::getFileExtension(srcName) == "usdz" || (usdzFilePath == "" || m_exportParam.exportOutputTempFiles)) {
				try {
					shade.copy_file(srcPathName.c_str(), dstPathName.c_str());
				} catch (...) { }
			}
		}

		filePath2 = dstDir + StringUtil::getFileSeparator() + sFileName;
		if (usdzFilePath != "") {
			const std::string fName = StringUtil::getFileName(usdzFilePath);
			usdzFilePath = dstDir + StringUtil::getFileSeparator() + fName;
		}
	}

	if (changedName) {
		shade.message(shade.gettext("msg_not_ascii_file_name"));

		// ファイルサイズが0の出力を削除.
		try {
			shade.delete_file(m_orgFilePath.c_str());
		} catch (...) { }
	}

	if (usdzFilePath == "" || m_exportParam.exportOutputTempFiles) {
		shade.message(std::string("Export : ") + filePath2);
	}

	if (usdzFilePath != "") {
		shade.message(std::string("Export : ") + usdzFilePath);
	}

	// 元のシーケンスモードに戻す.
	if (m_exportParam.optOutputBoneSkin) {
		if (m_oldSequenceMode) {
			m_pScene->set_sequence_mode(m_oldSequenceMode);
		}
	}

	m_pScene->set_dirty(m_oldDirty);
}

/**
 * 指定の形状がスキップ対象か.
 */
bool CUSDExporterInterface::m_checkSkipShape (sxsdk::shape_class* shape)
{
	bool skipF = false;
	if (!shape) return skipF;

	// レンダリング対象でない場合はスキップ.
	const std::string name(shape->get_name());
	if (shape->get_render_flag() == 0) skipF = true;
	if (name != "" && name[0] == '#') skipF = true;

	if (!skipF && shape->get_render_flag() < 0) {		// 継承.
		sxsdk::shape_class* pS = shape;
		while (pS->has_dad()) {
			pS = pS->get_dad();
			if (pS->get_render_flag() == 0) {
				skipF = true;
				break;
			}
			if (pS->get_render_flag() == 1) {
				skipF = false;
				break;
			}
		}
	}
	if (skipF) return true;

	// 形状により、スキップする形状を判断.
	const int type = shape->get_type();
	if (type == sxsdk::enums::part) {
		const int partType = shape->get_part().get_part_type();
		if (partType == sxsdk::enums::master_surface_part || partType == sxsdk::enums::master_image_part) skipF = true;

	} else {
		if (type == sxsdk::enums::master_surface || type == sxsdk::enums::master_image) skipF = true;

		// 光源の場合はスキップ.
		if (type == sxsdk::enums::area_light || type == sxsdk::enums::directional_light || type == sxsdk::enums::point_light || type == sxsdk::enums::spotlight) {
			skipF = true;
		}
		if (type == sxsdk::enums::line) {
			// 面光源/線光源の場合.
			sxsdk::line_class& lineC = shape->get_line();
			if (lineC.get_light_intensity() > 0.0f) skipF = true;
		}
	}

	if (type == sxsdk::enums::line) {
		// 開いた線形状で、回転体/掃引体でない場合.
		sxsdk::line_class& lineC = shape->get_line();
		if (!lineC.get_closed() && !lineC.is_extruded() && !lineC.is_revolved()) m_skip = true;
	}

	return skipF;
}

/**
 * カレント形状の処理の開始.
 */
void CUSDExporterInterface::begin (void *)
{
	m_pCurrentShape = NULL;
	m_curShapeHasSubdivision = false;
	m_begin_polymesh_count = 0;
	m_currentShapeSkinType = false;

	// カレントの形状管理クラスのポインタを取得.
	m_pCurrentShape        = m_pluginExporter->get_current_shape();
	const sxsdk::mat4 gMat = m_pluginExporter->get_transformation();
	const std::string name(m_pCurrentShape->get_name());

	//m_shapeStack.push(m_currentDepth, m_pCurrentShape, gMat);

	m_skip = m_checkSkipShape(m_pCurrentShape);

	const int type = m_pCurrentShape->get_type();

	// ローカル変換行列.
	sxsdk::mat4 tPreM = m_shapeStack.getLocalToWorldMatrix();
	sxsdk::mat4 lMat  = gMat * inv(tPreM);

	// 掃除引体、回転体の場合.
	if (m_pCurrentShape->get_type() == sxsdk::enums::line) {
		sxsdk::line_class& lineC = m_pCurrentShape->get_line();
		if (lineC.is_extruded() || lineC.is_revolved()) {
			lMat = inv(m_pCurrentShape->get_transformation()) * lMat;
		}
	}

	m_shapeStack.push(m_currentDepth, m_pCurrentShape, lMat);

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

	// リンクの場合、親を取得.
	sxsdk::shape_class* linkedParent = m_pCurrentShape->get_linked_parent();
	if (linkedParent) {
		sxsdk::shape_class* pDad = NULL;
		if (m_pCurrentShape->has_dad()) pDad = m_pCurrentShape->get_dad();

		// 1つ親の場合はリンクとしては無効.
		if (linkedParent->get_handle() == (pDad->get_handle())) linkedParent = NULL;
	}

	// マスターオブジェクトパートの場合.
	if (type == sxsdk::enums::part) {
		sxsdk::part_class& part = m_pCurrentShape->get_part();
		if (part.get_part_type() == sxsdk::enums::master_shape_part) {		// マスターオブジェクトパート.
			m_inMasterObjectPart = true;
		}
	}

	// マスターオブジェクトのみを格納する走査時.
	if (!m_skip) {
		if (m_traverseMasterObjectsMode) {
			if (!m_inMasterObjectPart) m_skip = true;
		} else {
			// マスターオブジェクト以外を格納する操作時.
			if (m_inMasterObjectPart) m_skip = true;
		}
	}

	m_currentPathName = "";
	if (!m_skip) {
		// 指定の形状と同じものがすでに格納済みの場合、そのときのパスを取得.
		const std::string samePath = m_sceneData.searchSamePath(m_pCurrentShape);

		// 形状情報を追加.
		// 戻り値は、USDのパスとしての名前。リンクを考慮したパスを作る.
		m_currentPathName = m_sceneData.appendShape(m_pCurrentShape, linkedParent);

		if (linkedParent) {
			m_linkStack.push_back(m_pCurrentShape);
		}

		if (!m_traverseMasterObjectsMode) {
			// リンク形状の場合は格納自身はスキップし、参照を保持.
			if (linkedParent) {
				m_sceneData.appendNodeReference(m_pCurrentShape, m_currentPathName);
			}
		}

		// リンク先を走査中の場合は格納は行わずスキップ.
		if (!m_linkStack.empty()) m_skip = true;

		// マスターオブジェクトは、リンク先から格納する.
		if (!m_traverseMasterObjectsMode) {
			if (m_skip) {
				if (samePath != "") m_currentPathName = samePath;

				// リンク時にまだ格納済みではない場合は、スキップしない.
				if (!m_sceneData.existStoreShape(m_pCurrentShape)) m_skip = false;
			}
		}
	}
	m_currentPathName0 = m_currentPathName;
	m_ballJointM = sxsdk::mat4::identity;

	if (!m_skip) {
		const sxsdk::mat4 lwMat = m_pCurrentShape->get_local_to_world_matrix();

		sxsdk::mat4 m = lMat;

		if (type == sxsdk::enums::polygon_mesh) {
			m = sxsdk::mat4::identity;

		} else if (Shade3DUtil::isBallJoint(*m_pCurrentShape)) {
			// ボールジョイントの場合のローカル座標での変換行列.
			m = Shade3DUtil::getBallJointMatrix(*m_pCurrentShape);
		}

		// 親がボールジョイントの場合.
		if (m_pCurrentShape->has_dad()) {
			sxsdk::shape_class* parentShape = m_pCurrentShape->get_dad();
			if (Shade3DUtil::isBallJoint(*parentShape)) {
				const sxsdk::mat4 parentLWMat = Shade3DUtil::getBallJointMatrix(*parentShape, true);
				m_ballJointM = (m * lwMat) * inv(parentLWMat);
			}
		}

		// 形状がメッシュに変換できない場合は、NULLノードとする(パート、ボーン、ボールジョイントなど).
		if (!m_sceneData.checkConvertMesh(m_pCurrentShape)) {
			// Shade3Dでのmm単位をcmに変換.
			//sxsdk::mat4 m = m_pCurrentShape->get_transformation();
			m = Shade3DUtil::convUnit_mm_to_cm(m);
			m_sceneData.appendNodeNull(m_pCurrentShape, m_currentPathName, m);
		}

		if (!m_traverseMasterObjectsMode) {
			// 未サポートのジョイントを使用している場合.
			if (Shade3DUtil::usedUnsupportedJoint(*m_pCurrentShape)) {
				m_warningCheck.appendUnsupportedJointUsedShape(m_pCurrentShape);
			}

			// 変換行列mがせん断を持つ場合.
			if (Shade3DUtil::hasShearInMatrix(m)) {
				m_warningCheck.appendShearUsedShape(m_pCurrentShape);
			}

			m_currentShapeSkinType = m_pCurrentShape->get_skin_type();
			if (m_currentShapeSkinType == 0) {		// クラシックスキン.
				m_warningCheck.appendClassicSkinUsedShape(m_pCurrentShape);
			}
		}
	}
}

/**
 * カレント形状の処理の終了.
 */
void CUSDExporterInterface::end (void *)
{
	if (!m_linkStack.empty() && m_linkStack.back() == m_pCurrentShape) {
		m_linkStack.pop_back();
	}

	// マスターオブジェクトパートから抜ける場合.
	if (m_inMasterObjectPart) {
		const int type = m_pCurrentShape->get_type();
		if (type == sxsdk::enums::part) {
			sxsdk::part_class& part = m_pCurrentShape->get_part();
			if (part.get_part_type() == sxsdk::enums::master_shape_part) {
				m_inMasterObjectPart = false;
			}
		}
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

	m_skip = m_checkSkipShape(m_pCurrentShape);
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

	// 掃引体時は、begin_polymesh - end_polymeshが複数回呼ばれる.
	// その際に、m_currentPathNameを更新する.
	if (m_begin_polymesh_count >= 1) {
		m_currentPathName = m_sceneData.appendUniquePath(m_pCurrentShape, m_currentPathName0);
	}

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

	// must_transform_skinでfalseを返した場合、スキン変形前の頂点が取得される.
	// そのため、独自変換は不要。.
	pos = pos * m_spMat;
	if (skin) {
		pos = pos * m_currentLWMatrix;		// スキン使用時はワールド座標に変換する.
	}
	pos = pos * m_ballJointM;				// 親がボールジョイントの場合の補正.

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

				// 同一のボーンを参照している場合はWeight値を0にする.
				{
					for (int j = 0; j < 4; ++j) {
						void* pV = m_sceneData.tmpMeshData.skinJointsHandle[i][j];
						for (int k = j + 1; k < 4; ++k) {
							void* pV2 = m_sceneData.tmpMeshData.skinJointsHandle[i][k];
							if (pV == pV2) {
								m_sceneData.tmpMeshData.skinWeights[i][k] = 0.0f;
							}
						}
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
		sxsdk::vec4 v4;
		for (int i = 0; i < n_list; ++i) {
			v4 = sxsdk::vec4(normals[i], 0) * m_spMat;
			m_sceneData.tmpMeshData.faceNormals.push_back(sxsdk::vec3(v4.x, v4.y, v4.z));
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
			m = m_pCurrentShape->get_transformation();
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

	m_begin_polymesh_count++;
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

/**
 * 頂点カラー情報を受け取る.
 */
void CUSDExporterInterface::polymesh_face_vertex_colors (int n_list, const int list[], const sxsdk::rgba_class* vertex_colors, int layer_index, int number_of_layers, void*)
{
	if (!m_exportParam.optOutputVertexColor) return;
	if (m_skip) return;
	if (n_list <= 2) return;

	if (layer_index == 0) {
		for (int i = 0; i < n_list; ++i) {
			sxsdk::rgba_class col = vertex_colors[i];

			// 色をリニアにする.
			USD_DATA::convColorLinear(col.red, col.green, col.blue);
			m_sceneData.tmpMeshData.faceColor0.push_back(sxsdk::vec4(col.red, col.green, col.blue, col.alpha));
		}
	}
}

//--------------------------------------------------------------------.

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
		item->set_enabled(!m_exportParam.exportAppleUSDZ);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_file_usdz));
		item->set_bool(m_exportParam.exportUSDZ);
		item->set_enabled(!m_exportParam.exportAppleUSDZ);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_file_export_apple_usdz));
		item->set_bool(m_exportParam.exportAppleUSDZ);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_file_export_output_temp_files));
		item->set_bool(m_exportParam.exportOutputTempFiles);
		item->set_enabled(m_exportParam.exportAppleUSDZ || m_exportParam.exportUSDZ);
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
		item = &(d.get_dialog_item(dlg_option_subdivision));
		item->set_bool(m_exportParam.optSubdivision);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_divide_poly_tri_quad));
		item->set_bool(m_exportParam.optDividePolyTriQuad);
		item->set_enabled(!m_exportParam.exportAppleUSDZ);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_divide_poly_tri));
		item->set_bool(m_exportParam.optDividePolyTri);
		item->set_enabled(!m_exportParam.exportAppleUSDZ && m_exportParam.optDividePolyTriQuad);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_kind));
		item->set_selection((int)(m_exportParam.optKind));
	}

	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_texture_grayscale));
		item->set_bool(m_exportParam.texOptConvGrayscale);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_bake_without_processing_textures_id));
		item->set_bool(m_exportParam.bakeWithoutProcessingTextures);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_anim_keyframe_mode));
		item->set_selection((int)m_exportParam.animKeyframeMode);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_option_anim_keyframe_step));
		item->set_int(m_exportParam.animStep);
		item->set_enabled(m_exportParam.animKeyframeMode == USD_DATA::EXPORT::ANIM_KEYFRAME_MODE::anim_keyframe_step);
	}

	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_material_shader_type));
		item->set_selection((int)m_exportParam.materialShaderType);
		item->set_enabled(!m_exportParam.exportAppleUSDZ);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_separateOpacityAndTransmission));
		item->set_bool(m_exportParam.separateOpacityAndTransmission);
		item->set_enabled(!m_exportParam.exportAppleUSDZ && m_exportParam.materialShaderType != USD_DATA::EXPORT::MATERIAL_SHADER_TYPE::material_shader_type_UsdPreviewSurface);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_grayscale_texture_colorspace));
		item->set_selection((int)m_exportParam.grayscaleTexturesColorSpace);
		item->set_enabled(!m_exportParam.exportAppleUSDZ && m_exportParam.materialShaderType != USD_DATA::EXPORT::MATERIAL_SHADER_TYPE::material_shader_type_UsdPreviewSurface);
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

	if (id == dlg_file_export_apple_usdz) {
		m_exportParam.exportAppleUSDZ = item.get_bool();
		load_dialog_data(dialog);		// UIのディム状態を更新.
		return true;
	}

	if (id == dlg_file_export_output_temp_files) {
		m_exportParam.exportOutputTempFiles = item.get_bool();
		return true;
	}

	if (id == dlg_file_export_type) {
		m_exportParam.exportFileType = (USD_DATA::EXPORT::FILE_TYPE)item.get_selection();
		return true;
	}
	if (id == dlg_file_usdz) {
		m_exportParam.exportUSDZ = item.get_bool();
		load_dialog_data(dialog);		// UIのディム状態を更新.
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
	if (id == dlg_option_subdivision) {
		m_exportParam.optSubdivision = item.get_bool();
		return true;
	}
	if (id == dlg_option_divide_poly_tri_quad) {
		m_exportParam.optDividePolyTriQuad = item.get_bool();
		load_dialog_data(dialog);
		return true;
	}
	if (id == dlg_option_divide_poly_tri) {
		m_exportParam.optDividePolyTri = item.get_bool();
		return true;
	}
	if (id == dlg_option_kind) {
		m_exportParam.optKind = (USD_DATA::EXPORT::KIND_TYPE)item.get_selection();
		return true;
	}

	if (id == dlg_option_texture_grayscale) {
		m_exportParam.texOptConvGrayscale = item.get_bool();
	}
	if (id == dlg_option_bake_without_processing_textures_id) {
		m_exportParam.bakeWithoutProcessingTextures = item.get_bool();
	}
	if (id == dlg_option_anim_keyframe_mode) {
		m_exportParam.animKeyframeMode = (USD_DATA::EXPORT::ANIM_KEYFRAME_MODE)item.get_selection();
		load_dialog_data(dialog);		// UIのディム状態を更新.
		return true;
	}
	if (id == dlg_option_anim_keyframe_step) {
		m_exportParam.animStep = std::max(1, item.get_int());
		return true;
	}

	if (id == dlg_material_shader_type) {
		m_exportParam.materialShaderType = (USD_DATA::EXPORT::MATERIAL_SHADER_TYPE)item.get_selection();
		load_dialog_data(dialog);		// UIのディム状態を更新.
		return true;
	}
	if (id == dlg_separateOpacityAndTransmission) {
		m_exportParam.separateOpacityAndTransmission = item.get_bool();
		return true;
	}

	if (id == dlg_grayscale_texture_colorspace) {
		m_exportParam.grayscaleTexturesColorSpace = (USD_DATA::EXPORT::TEXTURE_COLOR_SPACE)item.get_selection();
		return true;
	}

	return false;
}

