/**
 * アニメーションのキーフレーム情報を格納する。また、一定フレーム数の間隔でベイク.
 */
#include "AnimKeyframeBake.h"
#include "MathUtil.h"
#include "Shade3DUtil.h"

#include <algorithm>

CAnimKeyframeData::CAnimKeyframeData ()
{
	clear();
}

CAnimKeyframeData::~CAnimKeyframeData ()
{
}

void CAnimKeyframeData::clear ()
{
	framePos = 0.0f;
	offset   = sxsdk::vec3(0, 0 ,0);
	quat     = sxsdk::quaternion_class();
}

CAnimKeyframeBake::CAnimKeyframeBake (sxsdk::scene_interface* scene, const CExportParam& exportParam) : m_pScene(scene), m_exportParam(exportParam)
{
	clear();
	m_currentSequencePos = 0.0f;
	m_sequenceMode = false;
}

CAnimKeyframeBake::~CAnimKeyframeBake ()
{
}

void CAnimKeyframeBake::clear ()
{
	m_keyframeData.clear();
}

/**
 * 処理の開始.
 */
void CAnimKeyframeBake::begin ()
{
	m_sequenceMode       = m_pScene->get_sequence_mode();
	m_currentSequencePos = m_pScene->get_sequence_value();
}

/**
 * 処理の終了.
 */
void CAnimKeyframeBake::end ()
{
	m_pScene->set_sequence_value(m_currentSequencePos);
	m_pScene->set_selection_mode(m_sequenceMode);
}

/**
 * 指定の形状のキーフレームをベイク.
 * m_exportParam.animKeyframeMode により、ステップ数で分割する処理なども行う.
 * @param[in] shape       対象形状.
 * @param[in] startFrame  モーションの開始フレーム.
 * @param[in] endFrame    モーションの終了フレーム.
 */
void CAnimKeyframeBake::storeKeyframes (sxsdk::shape_class* shape, const float startFrame, const float endFrame)
{
	clear();
	if (!shape) return;
	if (!(shape->has_motion())) return;
	if (m_exportParam.animKeyframeMode == USD_DATA::EXPORT::ANIM_KEYFRAME_MODE::anim_keyframe_none) return;

	const float frameRate = (float)(m_pScene->get_frame_rate());
	const int totalFrames = m_pScene->get_total_frames();

	try {
		compointer<sxsdk::motion_interface> motion(shape->get_motion_interface());
		const int pointsCou = motion->get_number_of_motion_points();
		if (pointsCou == 0) return;

		// 既存のキーフレーム情報を保持.
		if (m_exportParam.animKeyframeMode == USD_DATA::EXPORT::ANIM_KEYFRAME_MODE::anim_keyframe_only) {

			const sxsdk::mat4 lwMat = shape->get_local_to_world_matrix();
			sxsdk::vec3 boneLCenter = Shade3DUtil::getBoneBallJointCenterL(*shape);

			// ボーンの場合.
			sxsdk::mat4 boneLMat = sxsdk::mat4::identity;
			bool isBoneJoint = false;
			if (Shade3DUtil::isBone(*shape)) {
				isBoneJoint = true;
				sxsdk::part_class& part = shape->get_part();
				compointer<sxsdk::bone_joint_interface> bone(shape->get_bone_joint_interface());
				boneLMat = bone->get_matrix();
				boneLCenter = sxsdk::vec3(0, 0, 0);
			}

			std::vector<float> tmpKeyframes;
			std::vector<sxsdk::vec3> tmpOffsets;
			std::vector<sxsdk::vec3> tmpRotations;
			std::vector<bool> tmpLinears;

			float prevRotatorVal = 0.0f;
			sxsdk::vec3 eularV;
			float oldSeqPos = -1.0f;
			for (int i = 0; i < pointsCou; ++i) {
				compointer<sxsdk::motion_point_interface> motionPoint(motion->get_motion_point_interface(i));
				float seqPos = motionPoint->get_sequence();
				if (seqPos < startFrame || seqPos > endFrame) continue;

				// 同一のフレーム位置が格納済みの場合はスキップ.
				if (MathUtil::isZero(seqPos - oldSeqPos)) continue;
				if (oldSeqPos < 0.0f) oldSeqPos = seqPos;

				// ボーンまたはボールジョイント時.
				const sxsdk::vec3 offset  = motionPoint->get_offset();
				sxsdk::vec3 offset2       = offset + boneLCenter;
				sxsdk::quaternion_class q = motionPoint->get_rotation();
				q.get_euler(eularV);

				if (isBoneJoint) {
					// ボーンの変換行列は、回転 * ローカル変換行列 * オフセット.
					sxsdk::mat4 m = sxsdk::mat4::rotate(eularV) * boneLMat * sxsdk::mat4::translate(offset2);
					sxsdk::vec3 scale, shear, rotate, trans;
					m.unmatrix(scale, shear, rotate, trans);
					offset2 = trans;
					eularV  = rotate;
				}

				tmpKeyframes.push_back(seqPos);
				tmpOffsets.push_back(offset2);
				tmpRotations.push_back(eularV);

				oldSeqPos = seqPos;
			}

			if (tmpKeyframes.empty()) return;

			for (size_t i = 0; i < tmpKeyframes.size(); ++i) {
				//const float seqPos = tmpKeyframes[i] / frameRate;		// 秒単位に変換.

				CAnimKeyframeData keyframeD;
				keyframeD.framePos = tmpKeyframes[i];
				keyframeD.offset   = tmpOffsets[i];
				keyframeD.quat     = sxsdk::quaternion_class(tmpRotations[i]);
				m_keyframeData.push_back(keyframeD);
			}
/*

			// ---------------------------------.
			float oldSeqPos = -1.0f;
			for (int loop = 0; loop < pointsCou; ++loop) {
				compointer<sxsdk::motion_point_interface> motionPoint(motion->get_motion_point_interface(loop));
				float seqPos = motionPoint->get_sequence();

				// 同一のフレーム位置が格納済みの場合はスキップ.
				if (oldSeqPos >= 0.0f) {
					if (MathUtil::isZero(seqPos - oldSeqPos)) continue;
				}
				if (oldSeqPos < 0.0f) oldSeqPos = seqPos;
				oldSeqPos = seqPos;

				const sxsdk::vec3 offset        = motionPoint->get_offset();
				const sxsdk::quaternion_class q = motionPoint->get_rotation();

				CAnimKeyframeData keyframeD;
				keyframeD.framePos = seqPos;
				keyframeD.offset   = offset;
				keyframeD.quat     = q;
				m_keyframeData.push_back(keyframeD);
			}
			*/
			return;
		}

		const float frameStep = (float)std::max(1, m_exportParam.animStep);
		{
			// キーフレーム + 再分割した位置のフレーム位置のリストを作成.
			std::vector<float> framePosA;
			framePosA.push_back(startFrame);
			framePosA.push_back(endFrame);
			float oldSeqPos = -1.0f;
			for (int loop = 0; loop < pointsCou; ++loop) {
				compointer<sxsdk::motion_point_interface> motionPoint(motion->get_motion_point_interface(loop));
				float seqPos = motionPoint->get_sequence();

				// 同一のフレーム位置が格納済みの場合はスキップ.
				if (oldSeqPos >= 0.0f) {
					if (MathUtil::isZero(seqPos - oldSeqPos)) continue;
				}
				oldSeqPos = seqPos;

				if (std::find(framePosA.begin(), framePosA.end(), seqPos) == framePosA.end()) {
					framePosA.push_back(seqPos);
				}
			}

			{
				float seqPos = startFrame + frameStep;
				while (seqPos < endFrame) {
					if (std::find(framePosA.begin(), framePosA.end(), seqPos) == framePosA.end()) {
						framePosA.push_back(seqPos);
					}
					seqPos += frameStep;
				}
			}
			std::sort(framePosA.begin(), framePosA.end());

			// framePosA[]に格納したフレーム位置でのジョイント値を保持.
			for (size_t i = 0; i < framePosA.size(); ++i) {
				const float seqPos = framePosA[i];
				const sxsdk::vec3 offset        = motion->get_joint_offset(seqPos);
				const sxsdk::quaternion_class q = motion->get_joint_rotation(seqPos);

				CAnimKeyframeData keyframeD;
				keyframeD.framePos = seqPos;
				keyframeD.offset   = offset;
				keyframeD.quat     = q;
				m_keyframeData.push_back(keyframeD);
			}
		}

	} catch (...) { }
}

/**
 * ベイクしたキーフレーム情報を取得.
 */
const std::vector<CAnimKeyframeData>& CAnimKeyframeBake::getKeyframes () const
{
	return m_keyframeData;
}
