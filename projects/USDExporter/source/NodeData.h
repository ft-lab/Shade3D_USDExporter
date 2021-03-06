﻿/**
 * ノード情報.
 */
#ifndef _NODEDATA_H
#define _NODEDATA_H

#include "GlobalHeader.h"
#include "USDData.h"
#include "JointMotionData.h"

#include <string>

//------------------------------------------------------------------.
/**
 * ノードのベース情報.
 */
class CNodeBaseData
{
public:
	std::string name;				// ノード名.
	sxsdk::mat4 matrix;				// 変換行列.
	USD_DATA::NODE_TYPE nodeType;	// ノードの種類.

public:
	CNodeBaseData () {
		clear();
	}
	~CNodeBaseData () {
	}

	CNodeBaseData (const CNodeBaseData& v) {
		this->name     = v.name;
		this->matrix   = v.matrix;
		this->nodeType = v.nodeType;
	}

    CNodeBaseData& operator = (const CNodeBaseData &v) {
		this->name     = v.name;
		this->matrix   = v.matrix;
		this->nodeType = v.nodeType;
		return (*this);
	}

	void clear () {
		name = "";
		matrix = sxsdk::mat4::identity;
		nodeType = USD_DATA::NODE_TYPE::null_node;
	}
};

//------------------------------------------------------------------.
/**
 * NULLノードの情報.
 */
class CNodeNullData : public CNodeBaseData
{
public:
	void *shapeHandle;			// sxsdk::shape_classのハンドル.
	CJointMotionData jointMotion;		// ジョイントとしてのモーション情報.
	bool hasFaceGroup;					// 子としてFace groupのMeshを持つ場合.
public:
	CNodeNullData () {
		clear();
	}
	virtual ~CNodeNullData () {
	}

	CNodeNullData (const CNodeNullData& v) {
		this->name     = v.name;
		this->matrix   = v.matrix;
		this->nodeType = v.nodeType;
		this->shapeHandle = v.shapeHandle;
		this->jointMotion = v.jointMotion;
		this->hasFaceGroup = v.hasFaceGroup;
	}

    CNodeNullData& operator = (const CNodeNullData &v) {
		this->name     = v.name;
		this->matrix   = v.matrix;
		this->nodeType = v.nodeType;
		this->shapeHandle = v.shapeHandle;
		this->jointMotion = v.jointMotion;
		this->hasFaceGroup = v.hasFaceGroup;
		return (*this);
	}

	void clear () {
		name = "";
		matrix = sxsdk::mat4::identity;
		nodeType = USD_DATA::NODE_TYPE::null_node;
		shapeHandle = NULL;
		jointMotion.clear();
		hasFaceGroup = false;
	}
};

//------------------------------------------------------------------.
/**
 * 参照用のノード.
 */
class CNodeRefData : public CNodeBaseData
{
public:
	void *shapeHandle;			// sxsdk::shape_classのハンドル.

public:
	CNodeRefData () {
		clear();
	}
	virtual ~CNodeRefData () {
	}

	CNodeRefData (const CNodeRefData& v) {
		this->name     = v.name;
		this->matrix   = v.matrix;
		this->nodeType = v.nodeType;
		this->shapeHandle = v.shapeHandle;
	}

    CNodeRefData& operator = (const CNodeRefData &v) {
		this->name     = v.name;
		this->matrix   = v.matrix;
		this->nodeType = v.nodeType;
		this->shapeHandle = v.shapeHandle;
		return (*this);
	}

	void clear () {
		name = "";
		matrix = sxsdk::mat4::identity;
		nodeType = USD_DATA::NODE_TYPE::ref_node;
		shapeHandle = NULL;
	}
};
#endif
