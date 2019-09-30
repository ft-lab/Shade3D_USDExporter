/**
 * パス(/root/objects/sphere の表記)として見た際に、.
 * 同一名ではないかどうか、英数字ではないかどうか、などのチェックを行うクラス.
 */
#include "FindNames.h"
#include "StringUtil.h"

#include <algorithm>
#include <iostream>

CFindNames::CFindNames ()
{
	clear();
}

void CFindNames::clear ()
{
	m_namesList.clear();
}

/**
 * 名前を格納.
 * @param[in] name      格納する名前 (/root/objects/sphere の表記).
 * @param[in] nodeType  ノードの種類.
 * @param[in] chkFilename  ファイル名として拡張子を考慮する場合はtrue.
 * @return 格納された名前 (同一名にならないように変更されたもの).
 */
std::string CFindNames::appendName (const std::string& name, const USD_DATA::NODE_TYPE& nodeType, const bool chkFilename)
{
	std::string name2 = name;

	// ASCII文字列で構成されているかチェック.
	if (!StringUtil::checkASCII(name2)) {
		const int iPos = name2.find_last_of("/");
		const std::string nameOnly = (iPos != std::string::npos) ? name2.substr(iPos + 1) : name2;

		if (iPos != std::string::npos) {
			name2 = name2.substr(0, iPos);
		} else {
			name2 = "";
		}

		name2 += std::string("/");

		if (nameOnly == u8"球") {
			name2 += std::string("sphere");
		} else if (nameOnly == u8"パート") {
			name2 += std::string("part");
		} else if (nameOnly == u8"自由曲面") {
			name2 += std::string("curved_surface");
		} else if (nameOnly == u8"円") {
			name2 += std::string("disk");
		} else {
			switch (nodeType) {
			case USD_DATA::NODE_TYPE::null_node:
				name2 += std::string("node");
				break;

			case USD_DATA::NODE_TYPE::mesh_node:
				name2 += std::string("mesh");
				break;

			case USD_DATA::NODE_TYPE::bone_node:
				name2 += std::string("bone");
				break;

			case USD_DATA::NODE_TYPE::material_node:
				name2 += std::string("material");
				break;

			case USD_DATA::NODE_TYPE::texture_node:
				name2 += std::string("texture");
				break;
			}
		}
	}

	// 指定不可の文字がある場合にアンダーバーに置き換え.
	// また、ファイル時に一番はじめの文字が数値の場合、"_"を先頭に入れる.
	name2 = m_convASCIIString(name2, chkFilename);

	// 同じ名前が存在するかチェック.
	while (existName(name2)) {
		int num;
		std::string name3;

		std::string extStr = "";

		if (chkFilename) {
			// 拡張子部と分ける.
			const int iPos = name2.find_last_of(".");
			if (iPos != std::string::npos) {
				extStr = name2.substr(iPos + 1);
				name2  = name2.substr(0, iPos);
			}
		}

		// 名前部と連番部を分け、連番部分を+1する.
		if (m_divideNameAndNumber(name2, name3, num)) {
			name2 = name3 + std::string("_") + std::to_string(num + 1);
		} else {
			name2 = name2 + std::string("_1");
		}
		if (extStr != "") {
			name2 = name2 + std::string(".") + extStr;
		}
	}

	m_namesList.push_back(name2);
	return name2;
}

/**
 * 指定の名前が存在するかチェック.
 */
bool CFindNames::existName (const std::string& name)
{
	if (std::find(m_namesList.begin(), m_namesList.end(), name) != m_namesList.end()) return true;
	return false;
}

/**
 * 指定の文字列を整数値に変換.
 * @return 変換できない場合は-1を返す.
 */
int CFindNames::m_convStringToNumber (const std::string& str)
{
	if (str == "") return -1;
	const unsigned char *strP = (const unsigned char *)str.c_str();

	int i = 0;
	bool chkF = false;
	while (strP[i]) {
		if (strP[i] >= '0' && strP[i] <= '9') {
			i++;
			continue;
		}
		chkF = true;
		break;
	}
	if (chkF) return -1;

	return std::stoi(str);
}


/**
 * 指定の名前が「xxxx_1」などの場合に、「xxxx」と「1」のように名前部と数値部に分解する.
 */
bool CFindNames::m_divideNameAndNumber (const std::string& name, std::string& retName, int& retNumber)
{
	const int iPos = name.find_last_of("_");
	if (iPos == std::string::npos) {
		retName = name;
		retNumber = -1;
		return false;
	}

	const std::string name2  = name.substr(0, iPos);
	const std::string numStr = name.substr(iPos + 1);

	const int num = m_convStringToNumber(numStr);
	if (num < 0) {
		retName = name;
		retNumber = -1;
		return false;
	}

	retName = name2;
	retNumber = num;
	return true;
}

/**
 * &%$# などのASCII文字を_に置き換え.
 * @param[in] chkFilename  ファイル名として拡張子を考慮する場合はtrue.
 */
std::string CFindNames::m_convASCIIString (const std::string& str, const bool chkFilename)
{
	std::string str2 = str;
	if (str2 == "") return str2;

	// 0-1、a-z、A-Z、/ でない場合は、'_'に置き換え.
	str2 = StringUtil::replaceASCIIStringOtherThanAlphabetAndNumber(str2, "_", true, chkFilename);

	if (chkFilename) {
		str2 = StringUtil::replaceString(str2, "/", "_");
	}
	
	// 先頭の文字が英字でない場合は、"_"を入れる.
	if (chkFilename) {
		const char* pStr = str2.c_str();
		if (!((pStr[0] >= 'a' && pStr[0] <= 'z') || (pStr[0] >= 'A' && pStr[0] <= 'Z'))) {
			str2 = std::string("_") + str2;
		}
	}

	return str2;
}
