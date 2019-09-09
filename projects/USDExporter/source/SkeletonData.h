/**
 * Skeleton情報.
 */

#ifndef _SKELETONDATA_H
#define _SKELETONDATA_H

#include "JointMotionData.h"

#include <string>
#include <vector>

//------------------------------------------------------------.
/**
 * Skeletonでのジョイントデータ.
 */
class CSkelJointData
{
public:
	std::string name;						// ジョイント名.
	std::vector<float> bindTransforms;		// 変換行列.
	std::vector<float> restTransforms;		// 元の変換行列.

	CJointMotionData jointMotion;			// ジョイントのモーションデータ.

	void *shapeHandle;						// Shade3Dでのボーン（ジョイント）に対応するsxsdk::shape_classのハンドル.

public:
	CSkelJointData ();
	~CSkelJointData ();

	CSkelJointData (const CSkelJointData& v) {
		this->name = v.name;
		this->bindTransforms = v.bindTransforms;
		this->restTransforms = v.restTransforms;
		this->jointMotion    = v.jointMotion;
		this->shapeHandle    = v.shapeHandle;
	}

    CSkelJointData& operator = (const CSkelJointData &v) {
		this->name = v.name;
		this->bindTransforms = v.bindTransforms;
		this->restTransforms = v.restTransforms;
		this->jointMotion    = v.jointMotion;
		this->shapeHandle    = v.shapeHandle;
		return (*this);
	}

	void clear ();

};

//------------------------------------------------------------.
/**
 * Skeletonのデータ.
 */
class CSkeletonData
{
public:
	std::string rootName;					// ルート名.
	std::vector<CSkelJointData> joints;		// ジョイント情報.
	void *rootShapeHandle;					// Shade3DでのrootNameでのsxsdk::shape_classのハンドル.

public:
	CSkeletonData ();
	~CSkeletonData ();

	CSkeletonData (const CSkeletonData& v) {
		this->rootName = v.rootName;
		this->joints = v.joints;
		this->rootShapeHandle = v.rootShapeHandle;
	}

    CSkeletonData& operator = (const CSkeletonData &v) {
		this->rootName = v.rootName;
		this->joints = v.joints;
		this->rootShapeHandle = v.rootShapeHandle;
		return (*this);
	}

	void clear ();

	/**
	 * ジョイントごとのキーフレームが同じになるようにした移動配列を取得.
	 * @param[out] keyframes  キーフレーム位置が返る.
	 * @param[out] data       キーフレームごとのジョイントの値が返る.
	 * @return キーフレーム数.
	 */
	int getJointsTranslationData (std::vector<float>& keyframes, std::vector< std::vector<CJointTranslationData> >& data) const;

	/**
	 * ジョイントごとのキーフレームが同じになるようにした回転配列を取得.
	 * @param[out] keyframes  キーフレーム位置が返る.
	 * @param[out] data       キーフレームごとのジョイントの値が返る.
	 * @return キーフレーム数.
	 */
	int getJointsRotationData (std::vector<float>& keyframes, std::vector< std::vector<CJointRotationData> >& data) const;

	/**
	 * ジョイントごとのキーフレームが同じになるようにしたスケール配列を取得.
	 * @param[out] keyframes  キーフレーム位置が返る.
	 * @param[out] data       キーフレームごとのジョイントの値が返る.
	 * @return キーフレーム数.
	 */
	int getJointsScaleData (std::vector<float>& keyframes, std::vector< std::vector<CJointScaleData> >& data) const;

};

#endif
