/**
 * ジョイントごとのモーションデータ.
 */
#include "JointMotionData.h"

//------------------------------------------------------------------.
CJointTranslationData::CJointTranslationData ()
{
	clear();
}

CJointTranslationData::~CJointTranslationData ()
{
}

void CJointTranslationData::clear ()
{
	frame = 0.0f;
	x = y = z = 0.0f;
}

//------------------------------------------------------------------.
CJointScaleData::CJointScaleData ()
{
	clear();
}

CJointScaleData::~CJointScaleData ()
{
}

void CJointScaleData::clear ()
{
	frame = 0.0f;
	x = y = z = 1.0f;
}


//------------------------------------------------------------------.
CJointRotationData::CJointRotationData ()
{
	clear();
}

CJointRotationData::~CJointRotationData ()
{
}

void CJointRotationData::clear ()
{
	frame = 0.0f;
	x = y = z = 0.0f;
	eulerX = eulerY = eulerZ = 0.0f;
	w = 1.0f;
}

//------------------------------------------------------------------.

CJointMotionData::CJointMotionData ()
{
	clear();
}

CJointMotionData::~CJointMotionData ()
{
}

void CJointMotionData::clear ()
{
	name = "";
	translations.clear();
	rotations.clear();
	scales.clear();
}

/**
 * モーション情報を持つか.
 */
bool CJointMotionData::hasMotion () const
{
	return (!translations.empty() || !rotations.empty() || !scales.empty());
}

