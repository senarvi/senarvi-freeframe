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
	void render() const;
	void copyFramebuffer(GLuint src, GLuint dst) const;

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

	// locations of the global GLSL variables
	GLint inputTextureLocation_;
	GLint stateTextureLocation_;
	GLint thresholdLocation_;
};


#endif
