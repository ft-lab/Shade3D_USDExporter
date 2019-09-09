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

public:
	COcclusionShaderData () {
		clear();
	}

	void clear () {
		uvIndex = 0;
	}
};

#endif
