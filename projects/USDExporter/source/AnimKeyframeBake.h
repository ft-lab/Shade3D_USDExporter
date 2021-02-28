/**
 * アニメーションのキーフレーム情報を格納する。また、一定フレーム数の間隔でベイク.
 */

#ifndef _ANIMKEYFRAMEBAKE_H
#define _ANIMKEYFRAMEBAKE_H

#include <string>
#include <vector>

#include "GlobalHeader.h"
#include "ExportParam.h"

// ------------------------------------------------------------.
// モーションのキーフレームデータ.
// ボールジョイントとボーンで使用する.
// ------------------------------------------------------------.
class CAnimKeyframeData
{
public:
	float framePos;						// フレーム位置.
	sxsdk::vec3 offset;					// オフセット値.
	sxsdk::quaternion_class quat;		// 回転情報.
	sxsdk::vec3 scale;					// スケール (マイナススケール指定もありうるため).

public:
	CAnimKeyframeData ();
	~CAnimKeyframeData ();

	CAnimKeyframeData (const CAnimKeyframeData& v) {
		this->framePos = v.framePos;
		this->offset   = v.offset;
		this->quat     = v.quat;
		this->scale    = v.scale;
	}

    CAnimKeyframeData& operator = (const CAnimKeyframeData &v) {
		this->framePos = v.framePos;
		this->offset   = v.offset;
		this->quat     = v.quat;
		this->scale    = v.scale;
		return (*this);
	}

	void clear ();
};

// ------------------------------------------------------------.
// キーフレームのベイククラス.
// ------------------------------------------------------------.
class CAnimKeyframeBake
{
private:
	sxsdk::scene_interface* m_pScene;		// sceneクラス.
	CExportParam m_exportParam;				// エクスポートのデータ.

	std::vector<CAnimKeyframeData> m_keyframeData;	// キーフレームデータ.

	float m_currentSequencePos;				// シーケンス位置の保持用.
	bool m_sequenceMode;					// シーケンスモードの保持用.

public:
	CAnimKeyframeBake (sxsdk::scene_interface* scene, const CExportParam& exportParam);
	~CAnimKeyframeBake ();

	void clear ();

	/**
	 * 処理の開始.
	 */
	void begin ();

	/**
	 * 処理の終了.
	 */
	void end ();

	/**
	 * 指定の形状のキーフレームをベイク.
	 * m_exportParam.animKeyframeMode により、ステップ数で分割する処理なども行う.
	 * @param[in] shape       対象形状.
	 * @param[in] startFrame  モーションの開始フレーム.
	 * @param[in] endFrame    モーションの終了フレーム.
	 */
	void storeKeyframes (sxsdk::shape_class* shape, const float startFrame, const float endFrame);

	/**
	 * ベイクしたキーフレーム情報を取得.
	 */
	const std::vector<CAnimKeyframeData>& getKeyframes () const;
};

#endif
