﻿/**
 * 表面材質でAlphaModeを指定するインターフェース派生クラス.
 */

#ifndef _ALPHAMODEMATERIALINTERFACE_H
#define _ALPHAMODEMATERIALINTERFACE_H

#include "GlobalHeader.h"
#include "AlphaModeMaterialData.h"

class CAlphaModeMaterialInterface : public sxsdk::attribute_interface
{
private:
	sxsdk::shade_interface& shade;
	CAlphaModeMaterialData m_data;		// AlphaMode情報.

private:
	/**
	 * SDKのビルド番号を指定（これは固定で変更ナシ）。.
	 * ※ これはプラグインインターフェースごとに必ず必要。.
	 */
	virtual int get_shade_version () const { return SHADE_BUILD_NUMBER; }

	/**
	 * UUIDの指定（独自に定義したGUIDを指定）.
	 * ※ これはプラグインインターフェースごとに必ず必要。.
	 */
	virtual sx::uuid_class get_uuid (void * = 0) { return ALPHA_MODE_INTERFACE_ID; }

	/**
	 * 表面材質属性から呼ばれる.
	 */
	virtual void accepts_surface (bool &accept, void *aux=0) { accept = true; }

	virtual bool ask_surface (sxsdk::surface_interface *surface, void *aux=0);

	//--------------------------------------------------.
	//  ダイアログのイベント処理用.
	//--------------------------------------------------.
	/**
	 * ダイアログの初期化.
	 */
	virtual void initialize_dialog (sxsdk::dialog_interface &d, void * = 0);

	/** 
	 * ダイアログのイベントを受け取る.
	 */
	virtual bool respond (sxsdk::dialog_interface &d, sxsdk::dialog_item_class &item, int action, void * = 0);

	/**
	 * ダイアログのデータを設定する.
	 */
	virtual void load_dialog_data (sxsdk::dialog_interface &d, void * = 0);

public:
	CAlphaModeMaterialInterface (sxsdk::shade_interface& shade);
	virtual ~CAlphaModeMaterialInterface ();

	/**
	 * プラグイン名をSXUL(text.sxul)より取得.
	 */
	static const char *name (sxsdk::shade_interface *shade) { return shade->gettext("alpha_mode_title"); }
};

#endif

