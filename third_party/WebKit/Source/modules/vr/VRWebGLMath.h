#ifndef VRWebGLMath_h
#define VRWebGLMath_h

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
// // #include <GLES3/gl3ext.h>

void VRWebGL_multiplyMatrices4(const GLfloat* m, const GLfloat* n, GLfloat* o);
void VRWebGL_transposeMatrix4(const GLfloat* m, GLfloat* o);
void VRWebGL_inverseMatrix4(const GLfloat* m, GLfloat* o);

#endif