/**
 * エクスポート時のパラメータ.
 */
#ifndef _EXPORTPARAM_H
#define _EXPORTPARAM_H


namespace USD_DATA {
	namespace EXPORT {

		/**
		 * エクスポート時の出力形式.
		 */
		enum FILE_TYPE {
			file_type_usda = 0,		// usda.
			file_type_usdc,			// usdc.
		};

		/**
		 * テクスチャ出力時の種類.
		 */
		enum TEXTURE_TYPE {
			texture_type_use_image_name = 0,		// イメージ名から拡張子を参照.
			texture_type_replace_png,				// pngに置き換え.
			texture_type_replace_jpeg,				// jpegに置き換え.
		};

		/**
		 * 最大テクスチャサイズ.
		 */
		enum MAX_TEXTURE_SIZE {
			texture_size_none = 0,					// 指定なし.
			texture_size_256,						// 256.
			texture_size_512,						// 512.
			texture_size_1024,						// 1024.
			texture_size_2048,						// 2048.
			texture_size_4096,						// 4096.
		};

		/**
		 * アニメーションのキーフレーム出力の種類.
		 */
		enum ANIM_KEYFRAME_MODE {
			anim_keyframe_none = 0,					// なし.
			anim_keyframe_only,						// キーフレームのみ.
			anim_keyframe_step,						// ステップ指定.
		};

		/**
		 * 最大テクスチャサイズを数値で取得.
		 */
		int getTextureSize (const USD_DATA::EXPORT::MAX_TEXTURE_SIZE size);
	}
}

/**
 * エクスポートパラメータ.
 */
class CExportParam
{
public:
	bool exportAppleUSDZ;									// Appleのusdz互換.
	USD_DATA::EXPORT::FILE_TYPE exportFileType;				// 出力形式.
	bool exportUSDZ;										// usdzを出力.

	USD_DATA::EXPORT::TEXTURE_TYPE optTextureType;			// テクスチャ出力.
	USD_DATA::EXPORT::MAX_TEXTURE_SIZE optMaxTextureSize;	// 最大テクスチャサイズ.
	bool optOutputBoneSkin;									// ボーンとスキンを出力.
	bool optOutputVertexColor;								// 頂点カラーを出力.
	bool optOutputAnimation;								// アニメーションを出力.
	bool optSubdivision;									// Subdivisionを有効にする場合はtrue.

	// テクスチャオプション.
	bool texOptConvGrayscale;								// R/G/B/A要素のテクスチャがある場合に、それぞれをグレイスケール変換する.
	bool texOptBakeMultiTextures;							// 複数テクスチャをベイク.

	// アニメーションオプション.
	USD_DATA::EXPORT::ANIM_KEYFRAME_MODE animKeyframeMode;	// キーフレームの出力の種類.
	int animStep;											// ステップ.

public:
	CExportParam ();
	~CExportParam ();

	CExportParam (const CExportParam& v) {
		this->exportAppleUSDZ      = v.exportAppleUSDZ;
		this->exportFileType       = v.exportFileType;
		this->exportUSDZ           = v.exportUSDZ;
		this->optTextureType       = v.optTextureType;
		this->optMaxTextureSize    = v.optMaxTextureSize;
		this->optOutputBoneSkin    = v.optOutputBoneSkin;
		this->optOutputVertexColor = v.optOutputVertexColor;
		this->optSubdivision       = v.optSubdivision;

		this->texOptConvGrayscale     = v.texOptConvGrayscale;
		this->texOptBakeMultiTextures = v.texOptBakeMultiTextures;

		this->animKeyframeMode = v.animKeyframeMode;
		this->animStep = v.animStep;
	}

    CExportParam& operator = (const CExportParam &v) {
		this->exportAppleUSDZ      = v.exportAppleUSDZ;
		this->exportFileType       = v.exportFileType;
		this->exportUSDZ           = v.exportUSDZ;
		this->optTextureType       = v.optTextureType;
		this->optMaxTextureSize    = v.optMaxTextureSize;
		this->optOutputBoneSkin    = v.optOutputBoneSkin;
		this->optOutputVertexColor = v.optOutputVertexColor;
		this->optSubdivision       = v.optSubdivision;

		this->texOptConvGrayscale     = v.texOptConvGrayscale;
		this->texOptBakeMultiTextures = v.texOptBakeMultiTextures;

		this->animKeyframeMode = v.animKeyframeMode;
		this->animStep = v.animStep;

		return (*this);
	}
	void clear ();
};

#endif
