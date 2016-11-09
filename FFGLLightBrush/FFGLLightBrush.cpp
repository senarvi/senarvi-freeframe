// FFGLLightBrush.cpp - FFGL plugin for light painting
//
// Copyright 2015 - 2016 Seppo Enarvi
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cassert>
#include <vector>
#include <algorithm>
#include <GL/glew.h>
#include <FFGL.h>
#include "FFGLLightBrush.h"

#define FFPARAM_THRESHOLD (0)
#define FFPARAM_DARKENING (1)
#define FFPARAM_CLEAR (2)

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo(
	FFGLLightBrush::CreateInstance,     // Create method
	"LtBr",                             // Plugin unique ID
	"FFGLLightBrush",                   // Plugin name											
	1,                                  // API major version number 													
	000,                                // API minor version number	
	1,                                  // Plugin major version number
	000,                                // Plugin minor version number
	FF_EFFECT,                          // Plugin type
	"FFGL plugin for light painting",   // Plugin description
	"by Seppo Enarvi - users.marjaniemi.com/seppo" // About
	);

static const char * vertexShaderSource =
"void main()"
"{"
"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
"    gl_TexCoord[0] = gl_MultiTexCoord0;"
"    gl_FrontColor = gl_Color;"
"}";

// A fragment shader that updates the velocity texture.
static const char * velocityShaderSource =
"uniform sampler2D inputSampler;"
"uniform sampler2D stateSampler;"
"uniform sampler2D velocitySampler;"
"const vec4 grayScaleWeights = vec4(0.30, 0.59, 0.11, 0.0);"
"vec4 sample(sampler2D sampler, vec2 pos)"
"{"
"    ivec2 size = textureSize2D(sampler, 0);"
"    if ((pos.s < 0) || (pos.t < 0) || (pos.s >= size.s) || (pos.t >= size.t))"
"        return vec4(0.0, 0.0, 0.0, 1.0);"
"    else"
"        return texture2D(sampler, pos);"
"}"
"float luminance(vec4 color)"
"{"
"    vec4 scaledColor = color * grayScaleWeights;"
"    return scaledColor.r + scaledColor.g + scaledColor.b;"
"}"
"void main()"
"{"
"    vec2 center = gl_TexCoord[0].st;"
"    vec2 top = center + vec2(0.0, -1.0);"
"    vec2 bottom = center + vec2(0.0, 1.0);"
"    vec2 left = center + vec2(-1.0, 0.0);"
"    vec2 right = center + vec2(1.0, 0.0);"
"    vec4 inputColor = sample(inputSampler, center);"
"    float inputLuminance = luminance(inputColor);"
"    vec4 borderColor = (sample(stateSampler, top) +"
"                        sample(stateSampler, left) +"
"                        sample(stateSampler, right) +"
"                        sample(stateSampler, bottom)) / 4.0;"
"    float borderLuminance = luminance(borderColor);"
"    vec4 stateColor = sample(stateSampler, center);"
"    float stateLuminance = luminance(stateColor);"
"    vec4 velocityVec = sample(velocitySampler, center);"
"    float velocity = velocityVec.r * 0.0001;"
"    velocity += (borderLuminance - stateLuminance) * 0.0002;"
"    velocity += (inputLuminance - stateLuminance) * 0.0004;"
"    gl_FragColor = vec4(velocity, velocity, velocity, 1.0);"
"}";

// A fragment shader that writes the output pixels.
static const char * colorShaderSource =
"uniform sampler2D inputSampler;"
"uniform sampler2D stateSampler;"
"uniform sampler2D velocitySampler;"
"uniform float threshold;"
"uniform float darkening;"
"const vec4 grayScaleWeights = vec4(0.30, 0.59, 0.11, 0.0);"
"vec4 sample(sampler2D sampler, vec2 pos)"
"{"
"    ivec2 size = textureSize2D(sampler, 0);"
"    if ((pos.s < 0) || (pos.t < 0) || (pos.s >= size.s) || (pos.t >= size.t))"
"        return vec4(0.0, 0.0, 0.0, 1.0);"
"    else"
"        return texture2D(sampler, pos);"
"}"
"float luminance(vec4 color)"
"{"
"    vec4 scaledColor = color * grayScaleWeights;"
"    return scaledColor.r + scaledColor.g + scaledColor.b;"
"}"
"void main()"
"{"
"    vec2 center = gl_TexCoord[0].st;"
"    vec2 top = center + vec2(0.0, -1.0);"
"    vec2 bottom = center + vec2(0.0, 1.0);"
"    vec2 left = center + vec2(-1.0, 0.0);"
"    vec2 right = center + vec2(1.0, 0.0);"
"    vec4 inputColor = sample(inputSampler, center);"
"    float inputLuminance = luminance(inputColor);"
"    vec4 stateColor = sample(stateSampler, center);"
"    float stateLuminance = luminance(stateColor);"
"    vec4 velocityVec = sample(velocitySampler, center);"
"    float velocity = velocityVec.r;"
"    float outputLuminance = stateLuminance + velocity;"
"    vec4 outputColor = stateColor + vec4(velocity, velocity, velocity, 1.0);"
"    outputColor = vec4(abs(outputColor.r), abs(outputColor.g), abs(outputColor.b), 1.0);"
"    if (inputLuminance >= threshold)"
"        gl_FragColor = inputColor;"
"    else"
"        gl_FragColor = outputColor * vec4(darkening, darkening, darkening, 1.0);"
"}";

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FFGLLightBrush::FFGLLightBrush()
: CFreeFrameGLPlugin()
{
	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

	// Parameters
	threshold_ = 0.95;
	SetParamInfo(FFPARAM_THRESHOLD, "Threshold", FF_TYPE_STANDARD, threshold_);
	darkening_ = 0.95;
	SetParamInfo(FFPARAM_DARKENING, "Darkening", FF_TYPE_STANDARD, darkening_);
	SetParamInfo(FFPARAM_CLEAR, "Clear", FF_TYPE_EVENT, false);
}

FFGLLightBrush::~FFGLLightBrush()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

void FFGLLightBrush::compileShaders()
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	GLint isCompiled = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
	assert(isCompiled == GL_TRUE);

	GLuint velocityShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(velocityShader, 1, &velocityShaderSource, NULL);
	glCompileShader(velocityShader);
	glGetShaderiv(velocityShader, GL_COMPILE_STATUS, &isCompiled);
	assert(isCompiled == GL_TRUE);

	GLuint colorShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(colorShader, 1, &colorShaderSource, NULL);
	glCompileShader(colorShader);
	glGetShaderiv(colorShader, GL_COMPILE_STATUS, &isCompiled);
	assert(isCompiled == GL_TRUE);

	velocityProgram_ = glCreateProgram();
	glAttachShader(velocityProgram_, velocityShader);
	glAttachShader(velocityProgram_, vertexShader);
	glLinkProgram(velocityProgram_);
	GLint isLinked = 0;
	glGetProgramiv(velocityProgram_, GL_LINK_STATUS, &isLinked);
	assert(isLinked == GL_TRUE);
	glDetachShader(velocityProgram_, vertexShader);
	glDetachShader(velocityProgram_, velocityShader);

	colorProgram_ = glCreateProgram();
	glAttachShader(colorProgram_, colorShader);
	glAttachShader(colorProgram_, vertexShader);
	glLinkProgram(colorProgram_);
	isLinked = 0;
	glGetProgramiv(colorProgram_, GL_LINK_STATUS, &isLinked);
	assert(isLinked == GL_TRUE);
	glDetachShader(colorProgram_, vertexShader);
	glDetachShader(colorProgram_, colorShader);

	glDeleteShader(colorShader);
	glDeleteShader(velocityShader);
	glDeleteShader(vertexShader);
}

void FFGLLightBrush::initializeTexture(GLuint texture,
									   GLuint width,
									   GLuint height,
									   bool color) const
{
	glBindTexture(GL_TEXTURE_2D, texture);
	vector<GLubyte> data(width * height * 4, 0x00);
	if (color)
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			width, height,
			0,
			GL_BGRA,
			GL_UNSIGNED_INT_8_8_8_8_REV,
			&data[0]);
	else
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_R32F,
			width, height,
			0,
			GL_RED,
			GL_FLOAT,
			&data[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void FFGLLightBrush::renderToTexture(GLuint texture) const
{
	// Attach the texture to the framebuffer object.
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D,
		texture,
		0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);
	glViewport(viewport_.x, viewport_.y, viewport_.width, viewport_.height);
	glCallList(displayList_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void FFGLLightBrush::copyTexture(GLuint texture, GLuint dst) const
{
	// Attach the texture to the framebuffer object.
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D,
		texture,
		0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst);
	glBlitFramebuffer(
		0, 0, viewport_.width, viewport_.height,
		0, 0, viewport_.width, viewport_.height,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void FFGLLightBrush::clearState()
{
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);

	// Bind velocity state texture to the framebuffer object and clear it.
	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D,
		textures_[velocityStateTextureIndex_],
		0);
	glClear(GL_COLOR_BUFFER_BIT);

	// Bind color state texture to the framebuffer object and clear it.
	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D,
		textures_[colorStateTextureIndex_],
		0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.0, 0.0, 0.0, 0.0);
}

DWORD FFGLLightBrush::InitGL(const FFGLViewportStruct *vp)
{
	// GLEW helps to load dynamically some extensions.
	glewInit();
	assert(GLEW_VERSION_2_0);

	viewport_.x = 0;
	viewport_.y = 0;
	viewport_.width = vp->width;
	viewport_.height = vp->height;

	// Generate name for one framebuffer and four textures: the state and output
	// texture for pixel color and velocity.
	glGenFramebuffers(1, &framebuffer_);
	glGenTextures(4, textures_);

	// Compile the GLSL shaders.
	compileShaders();

	// To pass data to the shader, we need the location of the uniforms (global
	// variables defined in the shader code), and then call one of the
	// glUniform* methods.
	velocityShaderInputSampler_ =
		glGetUniformLocation(velocityProgram_, "inputSampler");
	assert(velocityShaderInputSampler_ != -1);
	velocityShaderStateSampler_ =
		glGetUniformLocation(velocityProgram_, "stateSampler");
	assert(velocityShaderStateSampler_ != -1);
	velocityShaderVelocitySampler_ =
		glGetUniformLocation(velocityProgram_, "velocitySampler");
	assert(velocityShaderVelocitySampler_ != -1);
	colorShaderInputSampler_ =
		glGetUniformLocation(colorProgram_, "inputSampler");
	assert(colorShaderInputSampler_ != -1);
	colorShaderStateSampler_ =
		glGetUniformLocation(colorProgram_, "stateSampler");
	assert(colorShaderStateSampler_ != -1);
	colorShaderVelocitySampler_ =
		glGetUniformLocation(colorProgram_, "velocitySampler");
	assert(colorShaderVelocitySampler_ != -1);
	colorShaderThreshold_ =
		glGetUniformLocation(colorProgram_, "threshold");
	assert(colorShaderThreshold_ != -1);
	colorShaderDarkening_ =
		glGetUniformLocation(colorProgram_, "darkening");
	assert(colorShaderDarkening_ != -1);

	// The input, state, and velocity textures are always bound to the same
	// texture units.
	glUseProgram(velocityProgram_);
	glUniform1i(velocityShaderInputSampler_, 0);
	glUniform1i(velocityShaderStateSampler_, 1);
	glUniform1i(velocityShaderVelocitySampler_, 2);
	glUseProgram(0);
	glUseProgram(colorProgram_);
	glUniform1i(colorShaderInputSampler_, 0);
	glUniform1i(colorShaderStateSampler_, 1);
	glUniform1i(colorShaderVelocitySampler_, 2);
	glUseProgram(0);

	// Initialize the textures with correct size. The velocity textyres are luminosity only.
	initializeTexture(textures_[0], viewport_.width, viewport_.height);
	initializeTexture(textures_[1], viewport_.width, viewport_.height);
	initializeTexture(textures_[2], viewport_.width, viewport_.height, false);
	initializeTexture(textures_[3], viewport_.width, viewport_.height, false);

	// Start with textures 0 and 2 being the state, and 1 and 3 being the
	// output, then switch.
	colorStateTextureIndex_ = 0;
	colorOutputTextureIndex_ = 1;
	velocityStateTextureIndex_ = 2;
	velocityOutputTextureIndex_ = 3;

	// Create a list of operations for painting a texture on a quad that fills
	// the entire viewport.
	if (displayList_ == 0) {
		displayList_ = glGenLists(1);
		glNewList(displayList_, GL_COMPILE);
		glColor4f(1.f, 1.f, 1.f, 1.f);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glBegin(GL_QUADS);
		//lower left
		glTexCoord2d(0.0, 0.0);
		glVertex2f(-1, -1);
		//upper left
		glTexCoord2d(0.0, 1.0);
		glVertex2f(-1, 1);
		//upper right
		glTexCoord2d(1.0, 1.0);
		glVertex2f(1, 1);
		//lower right
		glTexCoord2d(1.0, 0.0);
		glVertex2f(1, -1);
		glEnd();
		glEndList();
	}

	return FF_SUCCESS;
}

DWORD FFGLLightBrush::DeInitGL()
{
	glDeleteFramebuffers(1, &framebuffer_);
	glDeleteTextures(2, textures_);
	glDeleteProgram(colorProgram_);
	glDeleteProgram(velocityProgram_);
	return FF_SUCCESS;
}

DWORD FFGLLightBrush::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
	if (pGL->numInputTextures < 1)
		return FF_FAIL;
	if (pGL->inputTextures[0] == NULL)
		return FF_FAIL;
	FFGLTextureStruct &inputTexture = *(pGL->inputTextures[0]);

	glUseProgram(velocityProgram_);

	// Bind input texture to texture unit 0.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexture.Handle);

	// Bind color state texture to texture unit 1.
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures_[colorStateTextureIndex_]);

	// Bind velocity state texture to texture unit 2.
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, textures_[velocityStateTextureIndex_]);

	// Write to velocity output texture.
	renderToTexture(textures_[velocityOutputTextureIndex_]);

	glUseProgram(colorProgram_);

	// Pass the current parameter values to the shader program.
	glUniform1f(colorShaderThreshold_, threshold_);
	glUniform1f(colorShaderDarkening_, darkening_);

	// Bind input texture to texture unit 0.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexture.Handle);

	// Bind color state texture to texture unit 1.
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures_[colorStateTextureIndex_]);

	// Bind velocity output texture to texture unit 2.
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, textures_[velocityOutputTextureIndex_]);

	// Write to color output texture.
	renderToTexture(textures_[colorOutputTextureIndex_]);

	glUseProgram(0);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Copy the color output texture to the host framebuffer object.
	copyTexture(textures_[colorOutputTextureIndex_], pGL->HostFBO);

	swap(velocityStateTextureIndex_, velocityOutputTextureIndex_);
	swap(colorStateTextureIndex_, colorOutputTextureIndex_);

	return FF_SUCCESS;
}

DWORD FFGLLightBrush::GetParameter(DWORD dwIndex)
{
	DWORD dwRet;

	switch (dwIndex) {
	case FFPARAM_THRESHOLD:
		//sizeof(DWORD) must == sizeof(float)
		*((float *)(unsigned)(&dwRet)) = threshold_;
		return dwRet;

	case FFPARAM_DARKENING:
		//sizeof(DWORD) must == sizeof(float)
		*((float *)(unsigned)(&dwRet)) = darkening_;
		return dwRet;

	default:
		return FF_FAIL;
	}
}

DWORD FFGLLightBrush::SetParameter(const SetParameterStruct* pParam)
{
	if (pParam != NULL) {
		switch (pParam->ParameterNumber) {
		case FFPARAM_THRESHOLD:
			//sizeof(DWORD) must == sizeof(float)
			threshold_ = *((float *)(unsigned)&(pParam->NewParameterValue));
			break;

		case FFPARAM_DARKENING:
			//sizeof(DWORD) must == sizeof(float)
			darkening_ = *((float *)(unsigned)&(pParam->NewParameterValue));
			break;

		case FFPARAM_CLEAR:
			if (pParam->NewParameterValue) {
				clearState();
			}
			break;

		default:
			return FF_FAIL;
		}

		return FF_SUCCESS;
	}

	return FF_FAIL;
}
