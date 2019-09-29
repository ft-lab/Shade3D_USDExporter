/**
 * アニメーションのキーフレーム情報を格納する。また、一定フレーム数の間隔でベイク.
 */
#include "AnimKeyframeBake.h"

CAnimKeyframeData::CAnimKeyframeData ()
{
	clear();
}

CAnimKeyframeData::~CAnimKeyframeData ()
{
}

void CAnimKeyframeData::clear ()
{
	framePos = 0.0f;
	offset   = sxsdk::vec3(0, 0 ,0);
	quat     = sxsdk::quaternion_class();
}

CAnimKeyframeBake::CAnimKeyframeBake (sxsdk::scene_interface* scene, const CExportParam& exportParam) : m_pScene(scene), m_exportParam(exportParam)
{
	clear();
}

CAnimKeyframeBake::~CAnimKeyframeBake ()
{
}

void CAnimKeyframeBake::clear ()
{
	m_keyframeData.clear();
}

