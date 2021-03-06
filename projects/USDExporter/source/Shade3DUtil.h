﻿/**
 * Shade3Dの便利関数.
 */
#ifndef _SHADE3DUTIL_H
#define _SHADE3DUTIL_H

#include "GlobalHeader.h"
#include "USDData.h"
#include "TextureTransform.h"
#include <vector>

namespace Shade3DUtil {
	/**
	 * マスターイメージパートを取得.
	 * @param[in] scene  シーンクラス.
	 */
	sxsdk::shape_class* findMasterImagePart (sxsdk::scene_interface* scene);

	/**
	 * マスターサーフェスパートを取得.
	 */
	sxsdk::shape_class* findMasteSurfacePart (sxsdk::scene_interface* scene);

	/**
	 * サーフェス情報を持つ親形状までたどる.
	 * @param[in] shape  対象の形状.
	 * @return サーフェス情報を持つ親形状。なければshapeを返す.
	 */
	sxsdk::shape_class* getHasSurfaceParentShape (sxsdk::shape_class* shape);

	/**
	 * 指定のイメージに対応するマスターイメージを取得.
	 * @param[in] scene  シーンクラス.
	 * @param[in] image  対象のイメージ.
	 * @return マスターイメージが存在する場合はそのポインタ.
	 */
	sxsdk::master_image_class* getMasterImageFromImage (sxsdk::scene_interface* scene, sxsdk::image_interface* image);

	/**
	 * Shade3Dのmm単位から、USDのcmに変換.
	 */
	void convUnit_mm_to_cm (const float x, const float y, const float z,
		float& retX, float& retY, float& retZ);
	sxsdk::vec3 convUnit_mm_to_cm (const sxsdk::vec3& v);

	/**
	 * Shade3Dのmm単位から、USDのcmに変換 (行列).
	 */
	sxsdk::mat4 convUnit_mm_to_cm (const sxsdk::mat4& m);

	/**
	 * 指定のイメージで、テクスチャの変換をを行う.
	 */
	compointer<sxsdk::image_interface> createImageWithTransform (sxsdk::image_interface* image, const USD_DATA::TEXTURE_SOURE& textureSource, const CTextureTransform& textureTrans);

	/**
	 * テクスチャサイズを2の累乗でリサイズしたサイズを求める.
	 * @param[in] size     元のサイズ.
	 * @param[in] maxSize  最大サイズ。マイナスの場合は上限なし.
	 */
	sx::vec<int,2> calcImageSizePowerOf2 (const sx::vec<int,2>& size, const int maxSize = -1);

	/**
	 * 指定のマスターイメージがAlpha情報を持つかどうか.
	 * @param[in] masterImage  マスターイメージ.
	 * @return アルファ要素が1.0でないものがある場合はtrueを返す.
	 */
	bool hasImageAlpha (sxsdk::master_image_class* masterImage);
	bool hasImageAlpha (sxsdk::image_interface* image);

	/**
	 * 画像を指定のサイズにリサイズ。アルファも考慮（image->duplicate_imageはアルファを考慮しないため）.
	 * @param[in] image  元の画像.
	 * @param[in] size   変更するサイズ.
	 */
	compointer<sxsdk::image_interface> resizeImageWithAlpha (sxsdk::scene_interface* scene, sxsdk::image_interface* image, const sx::vec<int,2>& size);

	/**
	 * compointerを使用せずにイメージをリサイズ.
	 */
	sxsdk::image_interface* resizeImageWithAlphaNotCom (sxsdk::scene_interface* scene, sxsdk::image_interface* image, const sx::vec<int,2>& size);

	/**
	 * 指定のマッピングレイヤがOcclusion用のレイヤかどうか.
	 */
	bool isOcclusionMappingLayer (sxsdk::mapping_layer_class* mappingLayer);

	/**
	 * 選択形状(active_shape)での、Occlusion用のmapping_layer_classを取得.
	 */
	sxsdk::mapping_layer_class* getActiveShapeOcclusionMappingLayer (sxsdk::scene_interface* scene);

	/**
	 * ボーンのルートを取得.
	 * @param[in]  rootShape     検索を開始するルート.
	 * @param[out] bontRootList  ルートボーンが返る.
	 * @return ルートボーン数.
	 */
	int findBoneRoot (sxsdk::shape_class* rootShape, std::vector<sxsdk::shape_class*>& boneRootList);

	/**
	 * 指定の形状がボーンかどうか.
	 */
	bool isBone (sxsdk::shape_class& shape);

	/**
	 * 指定の形状がボーンで先端かどうか.
	 */
	bool isBoneEnd (sxsdk::shape_class& shape);

	/**
	 * 行列の回転要素をクリアする.
	 */
	sxsdk::mat4 clearMatrixRotate (const sxsdk::mat4& m);

	/**
	 * 指定の形状がボールジョイントかどうか.
	 */
	bool isBallJoint (sxsdk::shape_class& shape);

	/**
	 * 指定の形状がボーン/ボールジョイントかどうか.
	 */
	bool isBoneBallJoint (sxsdk::shape_class& shape);

	/**
	 * ボーン/ボールジョイントのワールド座標での中心位置とボーンサイズを取得.
	 */
	sxsdk::vec3 getJointCenter (sxsdk::shape_class& shape, float *size);

	/**
	 * ボーン/ボールジョイントのワールド座標での中心位置とボーンサイズを取得.
	 */
	sxsdk::vec3 getBoneBallJointCenter (sxsdk::shape_class& shape, float *size);

	/**
	 * ボーン/ボールジョイントのローカル座標での中心を取得.
	 */
	sxsdk::vec3 getBoneBallJointCenterL (sxsdk::shape_class& shape);

	/**
	 * ボールジョイントの変換行列を取得.
	 * @param[in] shape  対象形状.
	 * @param[in] worldM ワールド座標での変換行列を取得する場合はtrue.
	 */
	sxsdk::mat4 getBallJointMatrix (sxsdk::shape_class& shape, const bool worldM = false);

	/**
	 * リンクの参照元を取得.
	 * @param[in]   scene       シーン.
	 * @param[out]  shapesList  マスターオブジェクトとしての形状を格納.
	 * @return マスターオブジェクト数.
	 */
	int getLinkMasterObjects (sxsdk::scene_interface* scene, std::vector<sxsdk::shape_class *>& shapesList);

	/**
	 * 指定の形状がマスターオブジェクトパート内にあるか.
	 * @param[out]  shape     対象の形状.
	 */
	bool checkInMasterObjectPart (sxsdk::shape_class& shape);

	/**
	 * 指定の変換行列でせん断要素を持つかチェック.
	 */
	bool hasShearInMatrix (sxsdk::mat4& m);

	/**
	 * エクスポート時にサポートされていないジョイントかチェック.
	 */
	bool usedUnsupportedJoint (sxsdk::shape_class& shape);
}

#endif
