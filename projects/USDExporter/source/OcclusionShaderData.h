/**
 * Occlusion Shader情報.
 */

#ifndef _OCCLUSIONSHADERDATA_H
#define _OCCLUSIONSHADERDATA_H

/**
 * OcclusionのShader情報.
 */
class COcclusionShaderData
{
public:
	int uvIndex;			// UV層番号 (0 or 1).
	int channelMix;			// チャンネル合成（0:Red、1:Green、2:Blue、3:Alpha）.

public:
	COcclusionShaderData () {
		clear();
	}

	void clear () {
		uvIndex = 0;
		channelMix = 0;
	}
};

#endif
