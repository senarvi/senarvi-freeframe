#ifndef FFGLLIGHTBRUSH_H
#define FFGLLIGHTBRUSH_H

#include "FFGLPluginSDK.h"

class FFGLLightBrush :
	public CFreeFrameGLPlugin
{
public:
	FFGLLightBrush();
	virtual ~FFGLLightBrush();

	// factory method

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance)
	{
		*ppInstance = new FFGLLightBrush();
		if (*ppInstance != NULL) return FF_SUCCESS;
		return FF_FAIL;
	}

	// helper methods

	void compileShader();
	void initializeTexture(GLuint texture, GLuint width, GLuint height) const;
	void renderToTexture(GLuint texture) const;
	void copyFramebuffer(GLuint src, GLuint dst) const;
	void clearState();

	// FreeFrame plugin methods

	DWORD SetParameter(const SetParameterStruct* pParam);
	DWORD GetParameter(DWORD dwIndex);
	DWORD ProcessOpenGL(ProcessOpenGLStruct* pGL);
	DWORD InitGL(const FFGLViewportStruct *vp);
	DWORD DeInitGL();

protected:
	FFGLViewportStruct viewport_;
	GLuint displayList_ = 0;

	// parameters
	float threshold_;

	GLuint shaderProgram_;
	GLuint framebuffer_;
	GLuint textures_[2];
	int stateTextureIndex_;
	int outputTextureIndex_;

	// locations of the global GLSL variables
	GLint inputSamplerLocation_;
	GLint stateSamplerLocation_;
	GLint thresholdLocation_;
};


#endif
