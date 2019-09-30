/**
 * 文字列操作関数.
 */
#include "StringUtil.h"

#if _WINDOWS
#include "windows.h"
#endif

#undef max
#undef min

#include <string>
#include <sstream>
#include <algorithm>
#include <vector>

/**
 * パスのセパレータを取得.
 */
const std::string StringUtil::getFileSeparator ()
{
#if _WINDOWS
	return "\\";
#else
	return "/";
#endif
}

/**
 * ファイルパスからファイル名のみを取得.
 * @param[in] filePath      ファイルフルパス.
 * @param[in] hasExtension  trueの場合は拡張子も付ける.
 */
const std::string StringUtil::getFileName (const std::string& filePath, const bool hasExtension)
{
	// ファイルパスからファイル名を取得.
	std::string fileNameS = filePath;
	int iPos  = filePath.find_last_of("/");
	int iPos2 = filePath.find_last_of("\\");
	if (iPos != std::string::npos || iPos2 != std::string::npos) {
		if (iPos != std::string::npos && iPos2 != std::string::npos) {
			iPos = std::max(iPos, iPos2);
		} else if (iPos == std::string::npos) iPos = iPos2;
		if (iPos != std::string::npos) fileNameS = fileNameS.substr(iPos + 1);
	}
	if (hasExtension) return fileNameS;

	iPos = fileNameS.find_last_of(".");
	if (iPos != std::string::npos) {
		fileNameS = fileNameS.substr(0, iPos);
	}
	return fileNameS;
}

/**
 * ファイル名として使用できない文字('/'など)を"_"に置き換え.
 */
const std::string StringUtil::convAsFileName (const std::string& fileName)
{
	 std::string name = fileName;
	 name = replaceString(name, "/", "_");
	 name = replaceString(name, "#", "_");
	 name = replaceString(name, "*", "_");
	 name = replaceString(name, "<", "_");
	 name = replaceString(name, ">", "_");
	 return name;
}

/**
 * ファイル名を除いたディレクトリを取得.
 * @param[in] filePath      ファイルフルパス.
 */
const std::string StringUtil::getFileDir (const std::string& filePath)
{
	int iPos  = filePath.find_last_of("/");
	int iPos2 = filePath.find_last_of("\\");
	if (iPos == std::string::npos && iPos2 == std::string::npos) return filePath;
	if (iPos != std::string::npos && iPos2 != std::string::npos) {
		iPos = std::max(iPos, iPos2);
	} else if (iPos == std::string::npos) iPos = iPos2;
	if (iPos == std::string::npos) return filePath;

	return filePath.substr(0, iPos);
}

/**
 * ファイルパスから拡張子を取得.
 * @param[in] filePath      ファイルフルパス.
 */
const std::string StringUtil::getFileExtension (const std::string& filePath)
{
	if (filePath == "") return "";
	std::string fileNameS = getFileName(filePath);
	const int iPos = fileNameS.find_last_of(".");
	if (iPos == std::string::npos) return "";

	std::transform(fileNameS.begin(), fileNameS.end(), fileNameS.begin(), ::tolower);

	return fileNameS.substr(iPos + 1);
}

/**
 * 文字列内の全置換.
 * @param[in] targetStr   対象の文字列.
 * @param[in] srcStr      置き換え前の文字列.
 * @param[in] dstStr      置き換え後の文字列.
 * @return 変換された文字列.
 */
std::string StringUtil::replaceString (const std::string& targetStr, const std::string& srcStr, const std::string& dstStr)
{
	 std::string str = targetStr;

	 std::string::size_type pos = 0;
	 while ((pos = str.find(srcStr, pos)) != std::string::npos) {
		str.replace(pos, srcStr.length(), dstStr);
		pos += dstStr.length();
	 }
	 return str;
}

/**
 * ASCII文字列で、0-9、a-Z、A-Z、以外は置き換え.
 * @param[in] targetStr   対象の文字列.
 * @param[in] dstChar     置き換え後の文字.
 * @param[in] useSeparator  '/'をそのままにする場合はtrue.
 * @return 変換された文字列.
 */
std::string StringUtil::replaceASCIIStringOtherThanAlphabetAndNumber (const std::string& targetStr, char* dstChar, const bool useSeparator)
{
	std::string retStr = targetStr;

	for (size_t i = 0; i < targetStr.length(); ++i) {
		const char chD = targetStr[i];
		if ((chD >= '0' && chD <= '9') || (chD >= 'a' && chD <= 'z') || (chD >= 'A' && chD <= 'Z')) continue;
		if (useSeparator) {
			if (chD == '/') continue;
		}

		retStr[i] = dstChar[0];
	}
	return retStr;
}

/**
 * テキストをHTML用に変換.
 *  & ==> &amp;  < ==> &lt; など.
 */
std::string StringUtil::convHTMLEncode (const std::string& str)
{
	 std::string str2 = str;
	 str2 = replaceString(str2, "&", "&amp;");
	 str2 = replaceString(str2, "\"", "&quot;");
	 str2 = replaceString(str2, "<", "&lt;");
	 str2 = replaceString(str2, ">", "&gt;");
	 return str2;
}

/**
 * HTML用のテキストを元に戻す.
 *  &amp; ==> &   &lt; ==> < など.
 */
std::string StringUtil::convHTMLDecode (const std::string& str)
{
	 std::string str2 = str;
	 str2 = replaceString(str2, "&gt;", ">");
	 str2 = replaceString(str2, "&lt;", "<");
	 str2 = replaceString(str2, "&quot;", "\"");
	 str2 = replaceString(str2, "&amp;", "&");
	 return str2;
}

/**
 * UTF-8の文字列をSJISに変換.
 */
int StringUtil::convUTF8ToSJIS (const std::string& utf8Str, std::string& sjisStr) {
#if _WINDOWS
		// UTF-8の文字列サイズを取得.
		const int n1 = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, 0, 0);
		if (n1 <= 0) return 0;

		const int size = n1 << 2;

		// WCHAR形式に変換(UTF-8 ==> WCHAR).
		std::vector<WCHAR> wideCharStr;
		wideCharStr.resize(size, 0);
		MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &(wideCharStr[0]), n1);

		// SJIS形式に変換.
		std::vector<char> strBuff;
		strBuff.resize(size, 0);
		const int n2 = WideCharToMultiByte(CP_OEMCP, 0, &(wideCharStr[0]), -1, &(strBuff[0]), size, 0, 0);
		strBuff[n2] = '\0';
		sjisStr = std::string(&(strBuff[0]));
		if (n2 <= 0) return 0;
		return n2;
#else
		sjisStr = utf8Str;
		return sjisStr.length();
#endif
}

/**
 * すべてがASCII文字列かどうか.
 */
bool StringUtil::checkASCII (const std::string& name)
{
	const unsigned char *nameP = (const unsigned char *)name.c_str();
	bool chkF = true;

	int i = 0;
	while (nameP[i]) {
		if (nameP[i] >= 0x20 && nameP[i] <= 0x7e) {
			i++;
			continue;
		}
		chkF = false;
		break;
	}
	return chkF;
}

/**
 * 画像ファイル名に拡張子(pngまたはjpg)がない場合は拡張子をつける.
 * @param[in] filePath      ファイルフルパス.
 * @param[in] extension     拡張子(pngまたはjpg).
 * @param[in] forceF        強制的に指定の拡張子に置き換え.
 */
const std::string StringUtil::SetFileImageExtension (const std::string& filePath, const std::string& extension, const bool forceF)
{
	std::string retFilePath = filePath;

	if (!forceF) {
		const std::string extStr = getFileExtension(filePath);
		if (extStr == "jpeg" || extStr == "jpg" || extStr == "png") return retFilePath;
	}

	{
		// 拡張子を除外.
		const int iPos = retFilePath.find_last_of(".");
		if (iPos != std::string::npos) {
			retFilePath = retFilePath.substr(0, iPos);
		}
	}

	retFilePath += std::string(".") + extension;
	return retFilePath;
}
