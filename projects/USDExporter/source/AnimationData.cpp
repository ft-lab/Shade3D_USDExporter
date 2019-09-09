/**
 * アニメーション情報.
 */
#include "AnimationData.h"

CAnimationData::CAnimationData ()
{
	clear();
}

CAnimationData::~CAnimationData ()
{
}

void CAnimationData::clear ()
{
	startFrame = 0.0f;
	endFrame   = 100.0f;
	frameRate  = 30.0f;
}
