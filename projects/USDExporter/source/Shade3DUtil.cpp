/**
 * Shade3Dの便利関数.
 */
#include "Shade3DUtil.h"
#include "MathUtil.h"

namespace {
	/**
	 * 再帰的にボーンのルートを探す.
	 */
	void findBoneRootLoop (sxsdk::shape_class* rootShape, std::vector<sxsdk::shape_class*>& boneRootList) {
		if (rootShape->has_son()) {
			sxsdk::shape_class* pS = rootShape->get_son();
			while (pS->has_bro()) {
				pS = pS->get_bro();
				if (!pS) break;
				if (pS->get_type() == sxsdk::enums::part) {
					sxsdk::part_class& part = pS->get_part();
					if (part.get_part_type() == sxsdk::enums::bone_joint || part.get_part_type() == sxsdk::enums::ball_joint) {
						boneRootList.push_back(pS);
					} else {
						findBoneRootLoop(pS, boneRootList);
					}
				}
			}
		}
	}
}

/**
 * サーフェス情報を持つ親形状までたどる.
 * @param[in] shape  対象の形状.
 * @return サーフェス情報を持つ親形状。なければshapeを返す.
 */
sxsdk::shape_class* Shade3DUtil::getHasSurfaceParentShape (sxsdk::shape_class* shape)
{
	// サーフェスを持つ親までたどる.
	sxsdk::shape_class* shape2 = shape;
	while (!shape2->get_has_surface_attributes()) {
		if (!shape2->has_dad()) {
			shape2 = shape;
			break;
		}
		shape2 = shape2->get_dad();
	}

	return shape2;
}

/**
 * マスターイメージパートを取得.
 */
sxsdk::shape_class* Shade3DUtil::findMasterImagePart (sxsdk::scene_interface* scene)
{
	try {
		sxsdk::shape_class& rootShape = scene->get_shape();
		sxsdk::shape_class* pImagePartS = NULL;
		if (rootShape.has_son()) {
			sxsdk::shape_class* pS = rootShape.get_son();
			while (pS->has_bro()) {
				pS = pS->get_bro();
				if (!pS) break;
				if (pS->get_type() == sxsdk::enums::part) {
					sxsdk::part_class& part = pS->get_part();
					if (part.get_part_type() == sxsdk::enums::master_image_part) {
						pImagePartS = pS;
						break;
					}
				}
			}
			return pImagePartS;
		}
	} catch (...) { }

	return NULL;
}

/**
 * マスターサーフェスパートを取得.
 */
sxsdk::shape_class* Shade3DUtil::findMasteSurfacePart (sxsdk::scene_interface* scene)
{
	try {
		sxsdk::shape_class& rootShape = scene->get_shape();
		sxsdk::shape_class* pRetPartS = NULL;
		if (rootShape.has_son()) {
			sxsdk::shape_class* pS = rootShape.get_son();
			while (pS->has_bro()) {
				pS = pS->get_bro();
				if (!pS) break;
				if (pS->get_type() == sxsdk::enums::part) {
					sxsdk::part_class& part = pS->get_part();
					if (part.get_part_type() == sxsdk::enums::master_surface_part) {
						pRetPartS = pS;
						break;
					}
				}
			}
			return pRetPartS;
		}
	} catch (...) { }

	return NULL;
}

/**
 * 指定のイメージに対応するマスターイメージを取得.
 * @param[in] scene  シーンクラス.
 * @param[in] image  対象のイメージ.
 * @return マスターイメージが存在する場合はそのポインタ.
 */
sxsdk::master_image_class* Shade3DUtil::getMasterImageFromImage (sxsdk::scene_interface* scene, sxsdk::image_interface* image)
{
	if (!image) return NULL;
	if (!image->has_image()) return NULL;
	const int width  = image->get_size().x;
	const int height = image->get_size().y;

	sxsdk::master_image_class* masterImageC = NULL;

	// マスターイメージパートを取得.
	sxsdk::shape_class* pMasterImagePart = findMasterImagePart(scene);
	if (!pMasterImagePart || !(pMasterImagePart->has_son())) return NULL;

	try {
		sxsdk::shape_class* pS = pMasterImagePart->get_son();
		while (pS->has_bro()) {
			pS = pS->get_bro();
			if (!pS) break;
			if (pS->get_type() != sxsdk::enums::master_image) continue;
			sxsdk::master_image_class& masterImage = pS->get_master_image();
			sxsdk::image_interface* image2 = masterImage.get_image();
			if (!image2->has_image()) continue;

			const int width2  = image2->get_size().x;
			const int height2 = image2->get_size().y;
			if (width != width2 || height != height2) continue;

			if (image->is_same_as(image2)) {
				masterImageC = &masterImage;
				break;
			}
		}
		return masterImageC;
	} catch (...) { }

	return NULL;
}

/**
 * Shade3Dのmm単位から、USDのcmに変換.
 */
void Shade3DUtil::convUnit_mm_to_cm (const float x, const float y, const float z,
	float& retX, float& retY, float& retZ)
{
	retX = x * 0.1f;
	retY = y * 0.1f;
	retZ = z * 0.1f;
}

sxsdk::vec3 Shade3DUtil::convUnit_mm_to_cm (const sxsdk::vec3& v)
{
	return v * 0.1f;
}

/**
 * Shade3Dのmm単位から、USDのcmに変換 (行列).
 */
sxsdk::mat4 Shade3DUtil::convUnit_mm_to_cm (const sxsdk::mat4& m)
{
	sxsdk::vec3 scale ,shear, rotation, trans;

	// 要素ごとに分解.
	m.unmatrix(scale ,shear, rotation, trans);

	// transを0.1倍してcmにする.
	trans *= 0.1f;

	return sxsdk::mat4::scale(scale) * sxsdk::mat4::shear(shear) * sxsdk::mat4::rotate(rotation) * sxsdk::mat4::translate(trans);
}

/**
 * 行列の回転要素をクリアする.
 */
sxsdk::mat4 Shade3DUtil::clearMatrixRotate (const sxsdk::mat4& m)
{
	sxsdk::vec3 scale ,shear, rotation, trans;

	// 要素ごとに分解.
	m.unmatrix(scale ,shear, rotation, trans);

	return sxsdk::mat4::scale(scale) * sxsdk::mat4::shear(shear) * sxsdk::mat4::translate(trans);
}

/**
 * 指定のイメージで、テクスチャの変換を行う.
 */
compointer<sxsdk::image_interface> Shade3DUtil::createImageWithTransform (sxsdk::image_interface* image, const USD_DATA::TEXTURE_SOURE& textureSource,  const CTextureTransform& textureTrans)
{
	compointer<sxsdk::image_interface> retImage;
	if (!image) return retImage;

	try {
		const int width  = image->get_size().x;
		const int height = image->get_size().y;
		retImage = compointer<sxsdk::image_interface>(image->duplicate_image());
		const float weight = textureTrans.textureWeight;

		std::vector<sxsdk::rgba_class> colLines;
		colLines.resize(width);
		float fV;
		for (int y = 0; y < height; ++y) {
			image->get_pixels_rgba_float(0, y, width, 1, &(colLines[0]));
			for (int x = 0; x < width; ++x) {
				sxsdk::rgba_class& col = colLines[x];

				if (textureTrans.textureNormal) {		// 法線マップ時.
					col.red   = col.red   * weight + 0.5f * (1.0f - weight);
					col.green = col.green * weight + 0.5f * (1.0f - weight);
					col.blue  = col.blue  * weight + 1.0f * (1.0f - weight);
					continue;
				}

				if (textureTrans.flipColor) {
					col.red    = 1.0f - col.red;
					col.green  = 1.0f - col.green;
					col.blue   = 1.0f - col.blue;
					col.alpha  = 1.0f - col.alpha;
				}

				if (textureTrans.occlusion) {
					// Occlusionの場合、textureTrans.multiRが0.0に近いほど白にする.
					fV = col.red;
					switch (textureSource) {
					case USD_DATA::TEXTURE_SOURE::texture_source_r:
						fV = col.red;
						break;
					case USD_DATA::TEXTURE_SOURE::texture_source_g:
						fV = col.green;
						break;
					case USD_DATA::TEXTURE_SOURE::texture_source_b:
						fV = col.blue;
						break;
					case USD_DATA::TEXTURE_SOURE::texture_source_a:
						fV = col.alpha;
						break;
					}
					fV = fV * weight + 1.0f * (1.0f - weight);
					col.red = col.green = col.blue = fV;
					col.alpha = 1.0f;

				} else if (textureSource == USD_DATA::TEXTURE_SOURE::texture_source_rgb) {
					col.red   = col.red   * weight + textureTrans.factor[0] * (1.0f - weight);
					col.green = col.green * weight + textureTrans.factor[1] * (1.0f - weight);
					col.blue  = col.blue  * weight + textureTrans.factor[2] * (1.0f - weight);

					col.red   = col.red   * textureTrans.multiR + textureTrans.offsetR;
					col.green = col.green * textureTrans.multiG + textureTrans.offsetG;
					col.blue  = col.blue  * textureTrans.multiB + textureTrans.offsetB;

				} else if (textureSource == USD_DATA::TEXTURE_SOURE::texture_source_r) {
					fV = col.red;
					fV = fV * weight + textureTrans.factor[0] * (1.0f - weight);
					fV = fV * textureTrans.multiR + textureTrans.offsetR;
					col.red = col.green = col.blue = fV;
					col.alpha = 1.0f;

				} else if (textureSource == USD_DATA::TEXTURE_SOURE::texture_source_g) {
					fV = col.green;
					fV = fV * weight + textureTrans.factor[1] * (1.0f - weight);
					fV = fV * textureTrans.multiG + textureTrans.offsetG;
					col.red = col.green = col.blue = fV;
					col.alpha = 1.0f;

				} else if (textureSource == USD_DATA::TEXTURE_SOURE::texture_source_b) {
					fV = col.blue;
					fV = fV * weight + textureTrans.factor[2] * (1.0f - weight);
					fV = fV * textureTrans.multiB + textureTrans.offsetB;
					col.red = col.green = col.blue = fV;
					col.alpha = 1.0f;

				} else if (textureSource == USD_DATA::TEXTURE_SOURE::texture_source_a) {
					fV = col.alpha;
					fV = fV * weight + textureTrans.factor[3] * (1.0f - weight);
					fV = fV * textureTrans.multiR + textureTrans.offsetR;
					col.red = col.green = col.blue = fV;
					col.alpha = 1.0f;
				}
			}
			retImage->set_pixels_rgba_float(0, y, width, 1, &(colLines[0]));
		}
		retImage->update();

	} catch (...) { }

	return retImage;
}

/**
 * 指定のマッピングレイヤがOcclusion用のレイヤかどうか.
 */
bool Shade3DUtil::isOcclusionMappingLayer (sxsdk::mapping_layer_class* mappingLayer)
{
	try {
		const sx::uuid_class uuid = mappingLayer->get_pattern_uuid();
		return (uuid == OCCLUSION_SHADER_INTERFACE_ID);
	} catch (...) { }
	return false;
}

/**
 * 選択形状(active_shape)での、Occlusion用のmapping_layer_classを取得.
 */
sxsdk::mapping_layer_class* Shade3DUtil::getActiveShapeOcclusionMappingLayer (sxsdk::scene_interface* scene)
{
	try {
		sxsdk::shape_class& shape = scene->active_shape();
		if (!shape.get_has_surface_attributes()) return NULL;
		sxsdk::surface_class* surface = shape.get_surface();
		if (!surface) return NULL;
		const int layersCou = surface->get_number_of_mapping_layers();

		sxsdk::mapping_layer_class* mRetLayer = NULL;
		for (int i = 0; i < layersCou; ++i) {
			sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(i);
			if (Shade3DUtil::isOcclusionMappingLayer(&mLayer)) {
				mRetLayer = &mLayer;
				break;
			}
		}
		return mRetLayer;

	} catch (...) { }
	return NULL;
}

/**
 * ボーンのルートを取得.
 * @param[in]  rootShape     検索を開始するルート.
 * @param[out] bontRootList  ルートボーンが返る.
 * @return ルートボーン数.
 */
int Shade3DUtil::findBoneRoot (sxsdk::shape_class* rootShape, std::vector<sxsdk::shape_class*>& boneRootList)
{
	boneRootList.clear();
	::findBoneRootLoop(rootShape, boneRootList);
	return (int)boneRootList.size();
}

/**
 * 指定の形状がボーンかどうか.
 */
bool Shade3DUtil::isBone (sxsdk::shape_class& shape)
{
	if (shape.get_type() != sxsdk::enums::part) return false;
	sxsdk::part_class& part = shape.get_part();
	return (part.get_part_type() == sxsdk::enums::bone_joint);
}

/**
 * 指定の形状がボーンで先端かどうか.
 */
bool Shade3DUtil::isBoneEnd (sxsdk::shape_class& shape)
{
	if (!Shade3DUtil::isBone(shape)) return false;
	if (!shape.has_son()) return true;

	// 子要素にボーンを持つか.
	try {
		sxsdk::shape_class* pS = shape.get_son();
		bool hasChildBone = false;
		while(pS->has_bro()) {
			pS = pS->get_bro();
			if (Shade3DUtil::isBone(*pS)) {
				hasChildBone = true;
				break;
			}
		}
		return !hasChildBone;
	} catch (...) { }
	return false;
}

/**
 * 指定の形状がボールジョイントかどうか.
 */
bool Shade3DUtil::isBallJoint (sxsdk::shape_class& shape)
{
	if (shape.get_type() != sxsdk::enums::part) return false;
	sxsdk::part_class& part = shape.get_part();
	return part.is_ball_joint();
}

/**
 * ボーン/ボールジョイントのワールド座標での中心位置とボーンサイズを取得.
 */
sxsdk::vec3 Shade3DUtil::getJointCenter (sxsdk::shape_class& shape, float *size)
{
	if (size) *size = 0.0f;

	// シーケンスOff時の中心位置を取得する.
	// この場合は、bone->get_matrix()を使用.
	// shape.get_transformation() を取得すると、これはシーケンスOn時の変換行列になる.
	if (Shade3DUtil::isBone(shape)) {
		try {
			compointer<sxsdk::bone_joint_interface> bone(shape.get_bone_joint_interface());
			const sxsdk::mat4 m = bone->get_matrix();

			const sxsdk::mat4 lwMat = shape.get_local_to_world_matrix();
			const sxsdk::vec3 center = sxsdk::vec3(0, 0, 0) * m * lwMat;

			if (size) *size = bone->get_size();

			return center;
		} catch (...) { }
	}
	if (Shade3DUtil::isBallJoint(shape)) {
		try {
			compointer<sxsdk::ball_joint_interface> ball(shape.get_ball_joint_interface());
			const sxsdk::vec3 pos = ball->get_position();

			const sxsdk::mat4 lwMat = shape.get_local_to_world_matrix();
			const sxsdk::vec3 center = pos * lwMat;

			return center;
		} catch (...) { }
	}

	return sxsdk::vec3(0, 0, 0);

}

