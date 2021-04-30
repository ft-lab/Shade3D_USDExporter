/**
 * streamへの入出力.
 */
#include "StreamCtrl.h"

#include <string>
#include <algorithm>
#include <iostream>

/**
 * Exportダイアログボックスの情報を保存.
 */
void StreamCtrl::saveExportDialogParam (sxsdk::shade_interface& shade, const CExportParam& data)
{
	try {
		compointer<sxsdk::preference_interface> preference(shade.get_preference_interface());
		if (!preference) return;
		compointer<sxsdk::stream_interface> stream(preference->create_attribute_stream_interface_with_uuid(USD_EXPORTER_INTERFACE_ID));
		if (!stream) return;
		stream->set_pointer(0);
		stream->set_size(0);

		int iDat;
		int iVersion = USD_EXPORTER_DLG_STREAM_VERSION;
		stream->write_int(iVersion);

		{
			iDat = (int)data.exportFileType;
			stream->write_int(iDat);
		}
		{
			iDat = data.exportUSDZ ? 1 : 0;
			stream->write_int(iDat);
		}
		{
			iDat = (int)data.optTextureType;
			stream->write_int(iDat);
		}
		{
			iDat = (int)data.optMaxTextureSize;
			stream->write_int(iDat);
		}
		{
			iDat = data.optOutputBoneSkin ? 1 : 0;
			stream->write_int(iDat);
		}
		{
			iDat = data.optOutputVertexColor ? 1 : 0;
			stream->write_int(iDat);
		}
		{
			iDat = data.optSubdivision ? 1 : 0;
			stream->write_int(iDat);
		}
		{
			iDat = data.exportAppleUSDZ ? 1 : 0;
			stream->write_int(iDat);
		}
		{
			iDat = data.texOptConvGrayscale ? 1 : 0;
			stream->write_int(iDat);
		}
		{
			iDat = 0;		// dummy.
			stream->write_int(iDat);
		}
		{
			iDat = (int)data.animKeyframeMode;
			stream->write_int(iDat);
		}
		{
			stream->write_int(data.animStep);
		}

		// ver.101 - 
		{
			iDat = data.exportOutputTempFiles ? 1 : 0;
			stream->write_int(iDat);
		}

		// ver.102 - 
		{
			iDat = data.optDividePolyTriQuad ? 1 : 0;
			stream->write_int(iDat);
		}

		// ver.103 - 
		{
			iDat = data.bakeWithoutProcessingTextures ? 1 : 0;
			stream->write_int(iDat);
		}

		// ver.104 -
		{
			iDat = data.optDividePolyTri ? 1 : 0;
			stream->write_int(iDat);
		}

		// ver.105 - 
		{
			iDat = (int)data.materialShaderType;
			stream->write_int(iDat);

			iDat = data.separateOpacityAndTransmission ? 1 : 0;
			stream->write_int(iDat);
		}

	} catch (...) { }
}

/**
 * Exportダイアログボックスの情報を読み込み.
 */
void StreamCtrl::loadExportDialogParam (sxsdk::shade_interface& shade, CExportParam& data)
{
	data.clear();
	try {
		compointer<sxsdk::preference_interface> preference(shade.get_preference_interface());
		if (!preference) return;
		compointer<sxsdk::stream_interface> stream(preference->get_attribute_stream_interface_with_uuid(USD_EXPORTER_INTERFACE_ID));
		if (!stream) return;
		stream->set_pointer(0);

		int iDat;
		int iVersion;
		stream->read_int(iVersion);

		{
			stream->read_int(iDat);
			data.exportFileType = (USD_DATA::EXPORT::FILE_TYPE)iDat;
		}
		{
			stream->read_int(iDat);
			data.exportUSDZ = iDat ? true : false;
		}
		{
			stream->read_int(iDat);
			data.optTextureType = (USD_DATA::EXPORT::TEXTURE_TYPE)iDat;
		}
		{
			stream->read_int(iDat);
			data.optMaxTextureSize = (USD_DATA::EXPORT::MAX_TEXTURE_SIZE)iDat;
		}
		{
			stream->read_int(iDat);
			data.optOutputBoneSkin = iDat ? true: false;
		}
		{
			stream->read_int(iDat);
			data.optOutputVertexColor = iDat ? true: false;
		}
		{
			stream->read_int(iDat);
			data.optSubdivision = iDat ? true: false;
		}
		{
			stream->read_int(iDat);
			data.exportAppleUSDZ = iDat ? true: false;
		}
		{
			stream->read_int(iDat);
			data.texOptConvGrayscale = iDat ? true: false;
		}
		{
			stream->read_int(iDat);		// dummy.
		}
		{
			stream->read_int(iDat);
			data.animKeyframeMode = (USD_DATA::EXPORT::ANIM_KEYFRAME_MODE)iDat;
		}
		{
			stream->read_int(iDat);
			data.animStep = iDat;
		}

		// ver.101 - 
		if (iVersion >= USD_EXPORTER_DLG_STREAM_VERSION_101) {
			stream->read_int(iDat);
			data.exportOutputTempFiles = iDat ? true : false;
		}

		// ver.102 - 
		if (iVersion >= USD_EXPORTER_DLG_STREAM_VERSION_102) {
			stream->read_int(iDat);
			data.optDividePolyTriQuad = iDat ? true : false;
		}

		// ver.103 - 
		if (iVersion >= USD_EXPORTER_DLG_STREAM_VERSION_103) {
			stream->read_int(iDat);
			data.bakeWithoutProcessingTextures = iDat ? true : false;
		}

		// ver.104 - 
		if (iVersion >= USD_EXPORTER_DLG_STREAM_VERSION_104) {
			stream->read_int(iDat);
			data.optDividePolyTri = iDat ? true : false;
		}

		// ver.105 - 
		if (iVersion >= USD_EXPORTER_DLG_STREAM_VERSION_105) {
			stream->read_int(iDat);
			data.materialShaderType = (USD_DATA::EXPORT::MATERIAL_SHADER_TYPE)iDat;

			stream->read_int(iDat);
			data.separateOpacityAndTransmission = iDat ? true : false;
		}
	} catch (...) { }
}


/**
 * Occlusion Shader(マッピングレイヤ)情報を保存.
 */
void StreamCtrl::saveOcclusionParam (sxsdk::stream_interface* stream, const COcclusionShaderData& data)
{
	try {
		if (!stream) return;
		stream->set_pointer(0);
		stream->set_size(0);

		int iVersion = OCCLUSION_PARAM_DLG_STREAM_VERSION;
		stream->write_int(iVersion);

		stream->write_int(data.uvIndex);
		stream->write_int(data.channelMix);

	} catch (...) { }
}

void StreamCtrl::saveOcclusionParam (sxsdk::mapping_layer_class& mappingLayer, const COcclusionShaderData& data)
{
	try {
		compointer<sxsdk::stream_interface> stream(mappingLayer.create_attribute_stream_interface_with_uuid(OCCLUSION_SHADER_INTERFACE_ID));
		if (!stream) return;
		saveOcclusionParam(stream, data);
	} catch (...) { }
}

/**
 * Occlusion Shader(マッピングレイヤ)情報を取得.
 */
bool StreamCtrl::loadOcclusionParam (sxsdk::stream_interface* stream, COcclusionShaderData& data)
{
	data.clear();
	try {
		if (!stream) return false;
		stream->set_pointer(0);

		int iVersion;
		stream->read_int(iVersion);

		stream->read_int(data.uvIndex);

		if (iVersion >= OCCLUSION_PARAM_DLG_STREAM_VERSION_101) {
			stream->read_int(data.channelMix);
		}

		return true;
	} catch (...) { }
	return false;

}

bool StreamCtrl::loadOcclusionParam (sxsdk::mapping_layer_class& mappingLayer, COcclusionShaderData& data)
{
	data.clear();
	try {
		compointer<sxsdk::stream_interface> stream(mappingLayer.get_attribute_stream_interface_with_uuid(OCCLUSION_SHADER_INTERFACE_ID));
		if (!stream) return false;
		return loadOcclusionParam(stream, data);
	} catch (...) { }
	return false;
}

/**
 * AlphaModeのマテリアル情報を保存.
 */
void StreamCtrl::saveAlphaModeMaterialParam (sxsdk::stream_interface* stream, const CAlphaModeMaterialData& data)
{
	try {
		if (!stream) return;
		stream->set_pointer(0);
		stream->set_size(0);

		int iDat;
		int iVersion = ALPHA_MODE_DLG_STREAM_VERSION;
		stream->write_int(iVersion);

		iDat = (int)data.alphaModeType;
		stream->write_int(iDat);

		stream->write_float(data.alphaCutoff);

	} catch (...) { }
}

void StreamCtrl::saveAlphaModeMaterialParam (sxsdk::surface_interface* surface, const CAlphaModeMaterialData& data)
{
	try {
		compointer<sxsdk::stream_interface> stream(surface->create_attribute_stream_interface_with_uuid(ALPHA_MODE_INTERFACE_ID));
		if (!stream) return;
		saveAlphaModeMaterialParam(stream, data);
	} catch (...) { }
}

void StreamCtrl::saveAlphaModeMaterialParam (sxsdk::surface_class* surface, const CAlphaModeMaterialData& data)
{
	try {
		compointer<sxsdk::stream_interface> stream(surface->create_attribute_stream_interface_with_uuid(ALPHA_MODE_INTERFACE_ID));
		if (!stream) return;
		saveAlphaModeMaterialParam(stream, data);
	} catch (...) { }
}

/**
 * AlphaModeのマテリアル情報を取得.
 */
bool StreamCtrl::loadAlphaModeMaterialParam (sxsdk::stream_interface* stream, CAlphaModeMaterialData& data)
{
	data.clear();
	try {
		if (!stream) return false;
		stream->set_pointer(0);

		int iVersion;
		int iDat;
		stream->read_int(iVersion);

		stream->read_int(iDat);
		data.alphaModeType = (CommonParam::alpha_mode_type)iDat;

		stream->read_float(data.alphaCutoff);

		return true;
	} catch (...) { }
	return false;
}

bool StreamCtrl::loadAlphaModeMaterialParam (sxsdk::surface_interface* surface, CAlphaModeMaterialData& data)
{
	data.clear();
	try {
		compointer<sxsdk::stream_interface> stream(surface->get_attribute_stream_interface_with_uuid(ALPHA_MODE_INTERFACE_ID));
		if (!stream) return false;
		return loadAlphaModeMaterialParam(stream, data);
	} catch (...) { }
	return false;
}

bool StreamCtrl::loadAlphaModeMaterialParam (sxsdk::surface_class* surface, CAlphaModeMaterialData& data)
{
	data.clear();
	try {
		compointer<sxsdk::stream_interface> stream(surface->get_attribute_stream_interface_with_uuid(ALPHA_MODE_INTERFACE_ID));
		if (!stream) return false;
		return loadAlphaModeMaterialParam(stream, data);
	} catch (...) { }
	return false;
}

