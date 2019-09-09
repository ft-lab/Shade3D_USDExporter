/**
 * Skeleton情報.
 */
#include "SkeletonData.h"
#include "USDData.h"

#include <iostream>
#include <algorithm>
#include <map>

//------------------------------------------------------------.
CSkelJointData::CSkelJointData ()
{
	clear();
}

CSkelJointData::~CSkelJointData ()
{
}

void CSkelJointData::clear ()
{
	name = "";
	USD_DATA::setMatrix4x4_Identity(bindTransforms);
	USD_DATA::setMatrix4x4_Identity(restTransforms);
	jointMotion.clear();
	shapeHandle = NULL;
}

//------------------------------------------------------------.
CSkeletonData::CSkeletonData ()
{
	clear();
}

CSkeletonData::~CSkeletonData ()
{
}

void CSkeletonData::clear ()
{
	rootName = "";
	joints.clear();
	rootShapeHandle = NULL;
}

/**
 * ジョイントごとのキーフレームが同じになるようにした移動配列を取得.
 * @param[out] keyframes  キーフレーム位置が返る.
 * @param[out] data       ジョイントごとのキーフレーム値が返る.
 *                        data[ジョイント番号][キーフレーム番号].
 * @return キーフレーム数.
 */
int CSkeletonData::getJointsTranslationData (std::vector<float>& keyframes, std::vector< std::vector<CJointTranslationData> >& data) const
{
	keyframes.clear();
	data.clear();
	const size_t jointsCou = joints.size();

	// キーフレームを取得.
	for (size_t loop = 0; loop < jointsCou; ++loop) {
		const CSkelJointData& jointD = joints[loop];
		const size_t transCou = jointD.jointMotion.translations.size();
		if (transCou == 0) continue;

		// フレーム位置を取得.
		for (size_t i = 0; i < transCou; ++i) {
			keyframes.push_back(jointD.jointMotion.translations[i].frame);
		}

		// 昇順にソート.
		std::sort(keyframes.begin(), keyframes.end());

		// 重複値を削除.
		keyframes.erase(std::unique(keyframes.begin(), keyframes.end()), keyframes.end());
	}
	if (keyframes.empty()) return 0;
	const size_t keyframesCou = keyframes.size();

	// 値を取得.
	std::vector<CJointTranslationData> transList;
	transList.resize(keyframesCou);
	for (size_t loop = 0; loop < jointsCou; ++loop) {
		const CSkelJointData& jointD = joints[loop];
		const size_t transCou = jointD.jointMotion.translations.size();

		for (size_t i = 0; i < keyframesCou; ++i) transList[i].frame = -1.0f;

		for (size_t i = 0; i < transCou; ++i) {
			const float fPos = jointD.jointMotion.translations[i].frame;
			int kIndex = -1;
			for (size_t j = 0; j < keyframesCou; ++j) {
				const float fPos2 = keyframes[j];
				if (USD_DATA::isZero(fPos2 - fPos)) {
					kIndex = j;
					break;
				}
			}
			if (kIndex < 0) continue;
			transList[kIndex] = jointD.jointMotion.translations[i];
		}

		// キーフレームの値が割り当てられていない箇所を線形補間.
		for (size_t i = 0; i < keyframesCou; ++i) {
			const float curFramePos = keyframes[i];
			CJointTranslationData& curD = transList[i];

			if (curD.frame < 0.0f) {
				int minIndex = -1;
				int maxIndex = -1;
				for (size_t j = i - 1; j >= 0; --j) {
					if (transList[j].frame >= 0.0f) {
						minIndex = j;
						break;
					}
				}
				for (size_t j = i + 1; j < keyframesCou; ++j) {
					if (transList[j].frame >= 0.0f) {
						maxIndex = j;
						break;
					}
				}
				if (minIndex < 0 && maxIndex < 0) continue;
				if (minIndex < 0 && maxIndex >= 0) {
					curD = transList[maxIndex];

				} else if (minIndex >= 0 && maxIndex < 0) {
					curD = transList[minIndex];

				} else {
					const CJointTranslationData& minD = transList[minIndex];
					const CJointTranslationData& maxD = transList[maxIndex];
					const float minFramePos = keyframes[minIndex];
					const float maxFramePos = keyframes[maxIndex];
					const float fV = (curFramePos - minFramePos) / (maxFramePos - minFramePos);

					curD.x = (maxD.x - minD.x) * fV + minD.x;
					curD.y = (maxD.y - minD.y) * fV + minD.y;
					curD.z = (maxD.z - minD.z) * fV + minD.z;
				}
			}
		}

		for (size_t i = 0; i < keyframesCou; ++i) transList[i].frame = keyframes[i];
		data.push_back(transList);
	}

	return (int)keyframes.size();
}

/**
 * ジョイントごとのキーフレームが同じになるようにしたスケール配列を取得.
 * @param[out] keyframes  キーフレーム位置が返る.
 * @param[out] data       キーフレームごとのジョイントの値が返る.
 * @return キーフレーム数.
 */
int CSkeletonData::getJointsScaleData (std::vector<float>& keyframes, std::vector< std::vector<CJointScaleData> >& data) const
{
	keyframes.clear();
	data.clear();
	const size_t jointsCou = joints.size();

	// キーフレームを取得.
	for (size_t loop = 0; loop < jointsCou; ++loop) {
		const CSkelJointData& jointD = joints[loop];
		const size_t scalesCou = jointD.jointMotion.scales.size();
		if (scalesCou == 0) continue;

		// フレーム位置を取得.
		for (size_t i = 0; i < scalesCou; ++i) {
			keyframes.push_back(jointD.jointMotion.scales[i].frame);
		}

		// 昇順にソート.
		std::sort(keyframes.begin(), keyframes.end());

		// 重複値を削除.
		keyframes.erase(std::unique(keyframes.begin(), keyframes.end()), keyframes.end());
	}
	if (keyframes.empty()) return 0;
	const size_t keyframesCou = keyframes.size();

	// 値を取得.
	std::vector<CJointScaleData> scalesList;
	scalesList.resize(keyframesCou);
	for (size_t loop = 0; loop < jointsCou; ++loop) {
		const CSkelJointData& jointD = joints[loop];
		const size_t scalesCou = jointD.jointMotion.scales.size();

		for (size_t i = 0; i < keyframesCou; ++i) scalesList[i].frame = -1.0f;

		for (size_t i = 0; i < scalesCou; ++i) {
			const float fPos = jointD.jointMotion.scales[i].frame;
			int kIndex = -1;
			for (size_t j = 0; j < keyframesCou; ++j) {
				const float fPos2 = keyframes[j];
				if (USD_DATA::isZero(fPos2 - fPos)) {
					kIndex = j;
					break;
				}
			}
			if (kIndex < 0) continue;
			scalesList[kIndex] = jointD.jointMotion.scales[i];
		}

		// キーフレームの値が割り当てられていない箇所を線形補間.
		for (size_t i = 0; i < keyframesCou; ++i) {
			const float curFramePos = keyframes[i];
			CJointScaleData& curD = scalesList[i];

			if (curD.frame < 0.0f) {
				int minIndex = -1;
				int maxIndex = -1;
				for (size_t j = i - 1; j >= 0; --j) {
					if (scalesList[j].frame >= 0.0f) {
						minIndex = j;
						break;
					}
				}
				for (size_t j = i + 1; j < keyframesCou; ++j) {
					if (scalesList[j].frame >= 0.0f) {
						maxIndex = j;
						break;
					}
				}
				if (minIndex < 0 && maxIndex < 0) continue;
				if (minIndex < 0 && maxIndex >= 0) {
					curD = scalesList[maxIndex];

				} else if (minIndex >= 0 && maxIndex < 0) {
					curD = scalesList[minIndex];

				} else {
					const CJointScaleData& minD = scalesList[minIndex];
					const CJointScaleData& maxD = scalesList[maxIndex];
					const float minFramePos = keyframes[minIndex];
					const float maxFramePos = keyframes[maxIndex];
					const float fV = (curFramePos - minFramePos) / (maxFramePos - minFramePos);

					curD.x = (maxD.x - minD.x) * fV + minD.x;
					curD.y = (maxD.y - minD.y) * fV + minD.y;
					curD.z = (maxD.z - minD.z) * fV + minD.z;
				}
			}
		}

		for (size_t i = 0; i < keyframesCou; ++i) scalesList[i].frame = keyframes[i];
		data.push_back(scalesList);
	}

	return (int)keyframes.size();
}

/**
 * ジョイントごとのキーフレームが同じになるようにした回転配列を取得.
 * @param[out] keyframes  キーフレーム位置が返る.
 * @param[out] data       キーフレームごとのジョイントの値が返る.
 * @return キーフレーム数.
 */
int CSkeletonData::getJointsRotationData (std::vector<float>& keyframes, std::vector< std::vector<CJointRotationData> >& data) const
{
	keyframes.clear();
	data.clear();
	const size_t jointsCou = joints.size();

	// キーフレームを取得.
	for (size_t loop = 0; loop < jointsCou; ++loop) {
		const CSkelJointData& jointD = joints[loop];
		const size_t rotationCou = jointD.jointMotion.rotations.size();
		if (rotationCou == 0) continue;

		// フレーム位置を取得.
		for (size_t i = 0; i < rotationCou; ++i) {
			keyframes.push_back(jointD.jointMotion.rotations[i].frame);
		}

		// 昇順にソート.
		std::sort(keyframes.begin(), keyframes.end());

		// 重複値を削除.
		keyframes.erase(std::unique(keyframes.begin(), keyframes.end()), keyframes.end());
	}
	if (keyframes.empty()) return 0;
	const size_t keyframesCou = keyframes.size();

	// 値を取得.
	std::vector<CJointRotationData> rotationsList;
	rotationsList.resize(keyframesCou);
	for (size_t loop = 0; loop < jointsCou; ++loop) {
		const CSkelJointData& jointD = joints[loop];
		const size_t rotationsCou = jointD.jointMotion.rotations.size();

		for (size_t i = 0; i < keyframesCou; ++i) rotationsList[i].frame = -1.0f;

		for (size_t i = 0; i < rotationsCou; ++i) {
			const float fPos = jointD.jointMotion.rotations[i].frame;
			int kIndex = -1;
			for (size_t j = 0; j < keyframesCou; ++j) {
				const float fPos2 = keyframes[j];
				if (USD_DATA::isZero(fPos2 - fPos)) {
					kIndex = j;
					break;
				}
			}
			if (kIndex < 0) continue;
			rotationsList[kIndex] = jointD.jointMotion.rotations[i];
		}

		// キーフレームの値が割り当てられていない箇所を線形補間.
		for (size_t i = 0; i < keyframesCou; ++i) {
			const float curFramePos = keyframes[i];
			CJointRotationData& curD = rotationsList[i];

			if (curD.frame < 0.0f) {
				int minIndex = -1;
				int maxIndex = -1;
				for (size_t j = i - 1; j >= 0; --j) {
					if (rotationsList[j].frame >= 0.0f) {
						minIndex = j;
						break;
					}
				}
				for (size_t j = i + 1; j < keyframesCou; ++j) {
					if (rotationsList[j].frame >= 0.0f) {
						maxIndex = j;
						break;
					}
				}
				if (minIndex < 0 && maxIndex < 0) continue;
				if (minIndex < 0 && maxIndex >= 0) {
					curD = rotationsList[maxIndex];

				} else if (minIndex >= 0 && maxIndex < 0) {
					curD = rotationsList[minIndex];

				} else {
					const CJointRotationData& minD = rotationsList[minIndex];
					const CJointRotationData& maxD = rotationsList[maxIndex];
					const float minFramePos = keyframes[minIndex];
					const float maxFramePos = keyframes[maxIndex];
					const float fV = (curFramePos - minFramePos) / (maxFramePos - minFramePos);

					curD.x = (maxD.x - minD.x) * fV + minD.x;
					curD.y = (maxD.y - minD.y) * fV + minD.y;
					curD.z = (maxD.z - minD.z) * fV + minD.z;
					curD.w = (maxD.w - minD.w) * fV + minD.w;
				}
			}
		}

		for (size_t i = 0; i < keyframesCou; ++i) rotationsList[i].frame = keyframes[i];
		data.push_back(rotationsList);
	}

	return (int)keyframes.size();
}


