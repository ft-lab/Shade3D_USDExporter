/**
 * ジョイントごとのモーションデータ.
 */

#ifndef _JOINTMOTIONDATA_H
#define _JOINTMOTIONDATA_H

#include "USDData.h"

#include <string>
#include <vector>

//------------------------------------------------------------------.
/**
 * 移動情報.
 */
class CJointTranslationData {
public:
	float frame;		// フレーム位置.
	float x, y, z;		// 位置.

public:
	CJointTranslationData ();
	~CJointTranslationData ();

	CJointTranslationData (const CJointTranslationData& v) {
		this->frame = v.frame;
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
	}

    CJointTranslationData& operator = (const CJointTranslationData &v) {
		this->frame = v.frame;
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		return (*this);
	}

	void clear ();
};

/**
 * スケール情報.
 */
class CJointScaleData {
public:
	float frame;		// フレーム位置.
	float x, y, z;		// スケール値.

public:
	CJointScaleData ();
	~CJointScaleData ();

	CJointScaleData (const CJointScaleData& v) {
		this->frame = v.frame;
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
	}

    CJointScaleData& operator = (const CJointScaleData &v) {
		this->frame = v.frame;
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		return (*this);
	}

	void clear ();
};

/**
 * 回転情報（クォータニオン）.
 */
class CJointRotationData {
public:
	float frame;		// フレーム位置.
	float x, y, z, w;	// クォータニオン値.

public:
	CJointRotationData ();
	~CJointRotationData ();

	CJointRotationData (const CJointRotationData& v) {
		this->frame = v.frame;
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = v.w;
	}

    CJointRotationData& operator = (const CJointRotationData &v) {
		this->frame = v.frame;
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = v.w;
		return (*this);
	}

	void clear ();
};

//------------------------------------------------------------------.
/**
 * モーション情報.
 */
class CJointMotionData
{
public:
	std::string name;			// ジョイント名 (USDでのパス名).

	std::vector<CJointTranslationData> translations;	// キーフレームごとの移動情報.
	std::vector<CJointRotationData> rotations;			// キーフレームごとの回転情報.
	std::vector<CJointScaleData> scales;				// キーフレームごとのスケール情報.

public:
	CJointMotionData ();
	~CJointMotionData ();

	CJointMotionData (const CJointMotionData& v) {
		this->name         = v.name;
		this->translations = v.translations;
		this->rotations    = v.rotations;
		this->scales       = v.scales;
	}

    CJointMotionData& operator = (const CJointMotionData &v) {
		this->name         = v.name;
		this->translations = v.translations;
		this->rotations    = v.rotations;
		this->scales       = v.scales;
		return (*this);
	}

	void clear ();
};

#endif
