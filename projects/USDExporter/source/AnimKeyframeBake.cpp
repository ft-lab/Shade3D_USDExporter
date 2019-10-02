/**
 * アニメーションのキーフレーム情報を格納する。また、一定フレーム数の間隔でベイク.
 */
#include "AnimKeyframeBake.h"
#include "MathUtil.h"

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

	try {
		compointer<sxsdk::motion_interface> motion(shape->get_motion_interface());
		const int pointsCou = motion->get_number_of_motion_points();
		if (pointsCou == 0) return;

		// 既存のキーフレーム情報を保持.
		if (m_exportParam.animKeyframeMode == USD_DATA::EXPORT::ANIM_KEYFRAME_MODE::anim_keyframe_only) {
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
			return;
		}

		// 再分割する.
		const float frameStep = (float)std::max(1, m_exportParam.animStep);
		{
			std::vector<float> framePosA;
			float oldSeqPos = -1.0f;
			for (int loop = 0; loop < pointsCou; ++loop) {
				compointer<sxsdk::motion_point_interface> motionPoint(motion->get_motion_point_interface(loop));
				float seqPos = motionPoint->get_sequence();

				// 同一のフレーム位置が格納済みの場合はスキップ.
				if (oldSeqPos >= 0.0f) {
					if (MathUtil::isZero(seqPos - oldSeqPos)) continue;
				}
				oldSeqPos = seqPos;

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
