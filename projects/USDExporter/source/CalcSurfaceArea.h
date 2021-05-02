/**
 * Meshの表面積を計算.
 */

#ifndef _CALCSURFACEAREA_H
#define _CALCSURFACEAREA_H

#include "GlobalHeader.h"
#include "MeshData.h"

namespace MathUtil
{
	/**
	 * 指定のメッシュの表面積を計算.
	 */
	double calcSurfaceArea (sxsdk::scene_interface* scene, const CNodeMeshData& meshD);
}

#endif
