#ifndef VRWebGLEyeParameters_h
#define VRWebGLEyeParameters_h

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
// // #include <GLES3/gl3ext.h>

class VRWebGLEyeParameters
{
public:
	GLfloat xFOV;
	GLfloat yFOV;
	GLint width;
	GLint height;
	GLfloat interpupillaryDistance;
};

#endif