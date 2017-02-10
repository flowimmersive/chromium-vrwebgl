#ifndef VRWebGLPose_h
#define VRWebGLPose_h

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
// // #include <GLES3/gl3ext.h>

class VRWebGLPose
{
public:
	GLfloat timeStamp;
	GLfloat orientation[4];
	GLfloat position[3];
	GLfloat linearVelocity[3];
	GLfloat linearAcceleration[3];
	GLfloat angularVelocity[3];
	GLfloat angularAcceleration[3];
};

#endif