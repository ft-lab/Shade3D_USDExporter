/**
 * アニメーション情報.
 */

#ifndef _ANIMATIONDATA_H
#define _ANIMATIONDATA_H

#include <string>
#include <vector>

class CAnimationData
{
public:
	float startFrame;			// 開始フレーム.
	float endFrame;				// 終了フレーム.
	float frameRate;			// フレームレート.

public:
	CAnimationData ();
	~CAnimationData ();

	CAnimationData (const CAnimationData& v) {
		this->startFrame = v.startFrame;
		this->endFrame   = v.endFrame;
		this->frameRate  = v.frameRate;
	}

    CAnimationData& operator = (const CAnimationData &v) {
		this->startFrame = v.startFrame;
		this->endFrame   = v.endFrame;
		this->frameRate  = v.frameRate;
		return (*this);
	}

	void clear ();
};

#endif
