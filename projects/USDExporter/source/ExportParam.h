﻿/**
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
		 * Shaderの種類.
		 */
		enum MATERIAL_SHADER_TYPE {
			material_shader_type_UsdPreviewSurface = 0,				// デフォルト.
			material_shader_type_OmniPBR_NVIDIA_omniverse,			// OmniPBR (NVIDIA Omniverse).
			material_shader_type_OmniSurface_NVIDIA_omniverse,		// OmniSurface (NVIDIA Omniverse).
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
		 * Color Space.
		 */
		enum TEXTURE_COLOR_SPACE {
			texture_colorspace_raw = 0,				// raw.
			texture_colorspace_sRGB,				// sRGB.
		};

		/**
		 * Kind.
		 */
		enum KIND_TYPE {
			kind_none = 0,
			kind_subcomponent,
			kind_component,
			kind_assembly,
			kind_group,
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
	USD_DATA::EXPORT::MATERIAL_SHADER_TYPE materialShaderType;	// マテリアルでのShaderの種類.

	bool exportUSDZ;										// usdzを出力.
	bool exportOutputTempFiles;								// usdz出力時に作業ファイルを出力 (ver.0.0.1.1 - ).

	USD_DATA::EXPORT::TEXTURE_TYPE optTextureType;			// テクスチャ出力.
	USD_DATA::EXPORT::MAX_TEXTURE_SIZE optMaxTextureSize;	// 最大テクスチャサイズ.
	bool optOutputBoneSkin;									// ボーンとスキンを出力.
	bool optOutputVertexColor;								// 頂点カラーを出力.
	bool optOutputAnimation;								// アニメーションを出力.
	bool optSubdivision;									// Subdivisionを有効にする場合はtrue.
	bool optDividePolyTriQuad;								// 多角形を三角形/四角形に分割.
	bool optDividePolyTri;									// 三角形分割.

	USD_DATA::EXPORT::KIND_TYPE optKind;					// Kind.

	// マテリアルオプション.
	bool separateOpacityAndTransmission;					// 「不透明(Opacity)」と「透明(Transmission)」を分ける (MDL時).
	USD_DATA::EXPORT::TEXTURE_COLOR_SPACE grayscaleTexturesColorSpace;				// グレイスケールテクスチャのColor Space.

	// テクスチャオプション.
	bool texOptConvGrayscale;								// R/G/B/A要素のテクスチャがある場合に、それぞれをグレイスケール変換する.
	bool bakeWithoutProcessingTextures;						// テクスチャを加工せずにベイク.

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
		this->exportOutputTempFiles = v.exportOutputTempFiles;
		this->materialShaderType   = v.materialShaderType;
		this->separateOpacityAndTransmission = v.separateOpacityAndTransmission;

		this->optTextureType       = v.optTextureType;
		this->optMaxTextureSize    = v.optMaxTextureSize;
		this->optOutputBoneSkin    = v.optOutputBoneSkin;
		this->optOutputVertexColor = v.optOutputVertexColor;
		this->optSubdivision       = v.optSubdivision;
		this->optDividePolyTriQuad = v.optDividePolyTriQuad;
		this->optDividePolyTri     = v.optDividePolyTri;
		this->optKind              = v.optKind;

		this->texOptConvGrayscale     = v.texOptConvGrayscale;
		this->bakeWithoutProcessingTextures = v.bakeWithoutProcessingTextures;
		this->grayscaleTexturesColorSpace   = v.grayscaleTexturesColorSpace;

		this->animKeyframeMode = v.animKeyframeMode;
		this->animStep = v.animStep;
	}

    CExportParam& operator = (const CExportParam &v) {
		this->exportAppleUSDZ      = v.exportAppleUSDZ;
		this->exportFileType       = v.exportFileType;
		this->exportUSDZ           = v.exportUSDZ;
		this->exportOutputTempFiles = v.exportOutputTempFiles;
		this->materialShaderType   = v.materialShaderType;
		this->separateOpacityAndTransmission = v.separateOpacityAndTransmission;

		this->optTextureType       = v.optTextureType;
		this->optMaxTextureSize    = v.optMaxTextureSize;
		this->optOutputBoneSkin    = v.optOutputBoneSkin;
		this->optOutputVertexColor = v.optOutputVertexColor;
		this->optSubdivision       = v.optSubdivision;
		this->optDividePolyTriQuad = v.optDividePolyTriQuad;
		this->optDividePolyTri     = v.optDividePolyTri;
		this->optKind              = v.optKind;

		this->texOptConvGrayscale     = v.texOptConvGrayscale;
		this->bakeWithoutProcessingTextures = v.bakeWithoutProcessingTextures;
		this->grayscaleTexturesColorSpace   = v.grayscaleTexturesColorSpace;

		this->animKeyframeMode = v.animKeyframeMode;
		this->animStep = v.animStep;

		return (*this);
	}
	void clear ();

	/**
	 * Shaderの種類としてMDLを使用するか.
	 * これは、Macのusdz出力ではないこと、materialShaderTypeでmaterial_shader_type_OmniPBR_NVIDIA_omniverseが選択されていることが条件.
	 */
	bool useShaderMDL () const;
};

#endif
