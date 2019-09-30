/**
 * 文字列操作関数.
 */

#ifndef _STRINGUTIL_H
#define _STRINGUTIL_H

#include <string>

namespace StringUtil
{
	/**
	 * ファイルパスからファイル名のみを取得.
	 * @param[in] filePath      ファイルフルパス.
	 * @param[in] hasExtension  trueの場合は拡張子も付ける.
	 */
	const std::string getFileName (const std::string& filePath, const bool hasExtension = true);

	/**
	 * ファイル名として使用できない文字('/'など)を"_"に置き換え.
	 */
	const std::string convAsFileName (const std::string& fileName);

	/**
	 * ファイル名を除いたディレクトリを取得.
	 * @param[in] filePath      ファイルフルパス.
	 */
	const std::string getFileDir (const std::string& filePath);

	/**
	 * ファイルパスから拡張子を取得.
	 * @param[in] filePath      ファイルフルパス.
	 */
	const std::string getFileExtension (const std::string& filePath);

	/**
	 * パスのセパレータを取得.
	 */
	const std::string getFileSeparator ();

	/**
	 * すべてがASCII文字列かどうか.
	 */
	bool checkASCII (const std::string& name);

	/**
	 * 画像ファイル名に拡張子(pngまたはjpg)がない場合は拡張子をつける.
	 * @param[in] filePath      ファイルフルパス.
	 * @param[in] extension     拡張子(pngまたはjpg).
	 * @param[in] forceF        強制的に指定の拡張子に置き換え.
	 */
	const std::string SetFileImageExtension (const std::string& filePath, const std::string& extension, const bool forceF = false);

	/**
	 * 文字列内の全置換.
	 * @param[in] targetStr   対象の文字列.
	 * @param[in] srcStr      置き換え前の文字列.
	 * @param[in] dstStr      置き換え後の文字列.
	 * @return 変換された文字列.
	 */
	std::string replaceString (const std::string& targetStr, const std::string& srcStr, const std::string& dstStr);

	/**
	 * ASCII文字列で、0-9、a-Z、A-Z、以外は置き換え.
	 * @param[in] targetStr   対象の文字列.
	 * @param[in] dstChar     置き換え後の文字.
	 * @param[in] useSeparator  '/'をそのままにする場合はtrue.
	 * @return 変換された文字列.
	 */
	std::string replaceASCIIStringOtherThanAlphabetAndNumber (const std::string& targetStr, char* dstChar, const bool useSeparator = true);

	/**
	 * テキストをHTML用に変換.
	 *  & ==> &amp;  < ==> &lt; など.
	 */
	std::string convHTMLEncode (const std::string& str);

	/**
	 * HTML用のテキストを元に戻す.
	 *  &amp; ==> &   &lt; ==> < など.
	 */
	std::string convHTMLDecode (const std::string& str);

	/**
	 * UTF-8の文字列をSJISに変換.
	 */
	int convUTF8ToSJIS (const std::string& utf8Str, std::string& sjisStr);

}

#endif
