/**
 * パス(/root/objects/sphere の表記)として見た際に、.
 * 同一名ではないかどうか、英数字ではないかどうか、などのチェックを行うクラス.
 */

#ifndef _FINDNAMES_H
#define _FINDNAMES_H

#include "USDData.h"

#include <vector>
#include <string>

class CFindNames {
public:

private:
	std::vector<std::string> m_namesList;

	/**
	 * 指定の名前が「xxxx_1」などの場合に、「xxxx」と「1」のように名前部と数値部に分解する.
	 */
	bool m_divideNameAndNumber (const std::string& name, std::string& retName, int& retNumber);

	/**
	 * 指定の文字列を整数値に変換.
	 * @return 変換できない場合は-1を返す.
	 */
	int m_convStringToNumber (const std::string& str);

	/**
	 * &%$# などのASCII文字を_に置き換え.
	 * @param[in] chkFilename  ファイル名として拡張子を考慮する場合はtrue.
	 */
	std::string m_convASCIIString (const std::string& str, const bool chkFilename = false);

public:
	CFindNames ();

	void clear ();

	int getCount () const { return m_namesList.size(); }

	/**
	 * 名前を格納.
	 * @param[in] name      格納する名前 (/root/objects/sphere の表記).
	 * @param[in] nodeType  ノードの種類.
	 * @param[in] chkFilename  ファイル名として拡張子を考慮する場合はtrue.
	 * @return 格納された名前 (同一名にならないように変更されたもの).
	 */
	std::string appendName (const std::string& name, const USD_DATA::NODE_TYPE& nodeType, const bool chkFilename = false);

	/**
	 * 指定の名前が存在するかチェック.
	 */
	bool existName (const std::string& name);
};

#endif
