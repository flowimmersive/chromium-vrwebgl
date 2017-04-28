/*
The whole purpose of having this file and the GL calls in a different file is to
be able to compile them in differen shared libraries. Chromium is using some macros to define opengl
calls.
*/
#include "modules/vr/VRWebGL.h"

#include <android/log.h>

#define LOG_TAG "VRWebGL"
#ifdef VRWEBGL_SHOW_LOG
    #define ALOGV(...) __android_log_print( ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__ )
#else
    #define ALOGV(...)
#endif
#define ALOGE(...) __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__ )

// Not using this but left it as a reminder of the pain I went through 
// to get this done. Also, prepared for some new extensions that could be
// added in the future.

// PFNGLDRAWARRAYSINSTANCEDANGLEPROC glDrawArraysInstancedANGLE = 0;
// PFNGLDRAWELEMENTSINSTANCEDANGLEPROC glDrawElementsInstancedANGLE = 0;
// PFNGLVERTEXATTRIBDIVISORANGLEPROC glVertexAttribDivisorANGLE = 0;

void VRWebGL_initializeExtensions() 
{
	// ALOGE("VRWebGL_initializeExtensions: glDrawArraysInstancedANGLE = %p",glDrawArraysInstancedANGLE);
	// ALOGE("VRWebGL_initializeExtensions: glDrawElementsInstancedANGLE = %p",glDrawElementsInstancedANGLE);
	// ALOGE("VRWebGL_initializeExtensions: glVertexAttribDivisorANGLE = %p",glVertexAttribDivisorANGLE);


	// GLint numberOfExtensions = 0;
	// glGetIntegerv(GL_NUM_EXTENSIONS, &numberOfExtensions);
	// ALOGE("VRWebGL_initializeExtensions: numberOfExtensions = %d", numberOfExtensions);
	// for (GLint i = 0; i < numberOfExtensions; i++)
	// {
	// 	char *name = (char*)glGetStringi(GL_EXTENSIONS, i);
	// 	ALOGE("VRWebGL_initializeExtensions: %s", name);
	// }

	// char* extensionString = (char*)glGetString(GL_EXTENSIONS);
	// if (strstr(extensionString, "GL_ANGLE_instanced_arrays"))
 //  {
	// 	glDrawArraysInstancedANGLE = (PFNGLDRAWARRAYSINSTANCEDANGLEPROC)eglGetProcAddress("glDrawArraysInstancedANGLE");
	// 	glDrawElementsInstancedANGLE = (PFNGLDRAWELEMENTSINSTANCEDANGLEPROC)eglGetProcAddress("glDrawElementsInstancedANGLE");
	// 	glVertexAttribDivisorANGLE = (PFNGLVERTEXATTRIBDIVISORANGLEPROC)eglGetProcAddress("glVertexAttribDivisorANGLE");

	// 	ALOGE("VRWebGL_initializeExtensions: glDrawArraysInstancedANGLE = %p",glDrawArraysInstancedANGLE);
	// 	ALOGE("VRWebGL_initializeExtensions: glDrawElementsInstancedANGLE = %p",glDrawElementsInstancedANGLE);
	// 	ALOGE("VRWebGL_initializeExtensions: glVertexAttribDivisorANGLE = %p",glVertexAttribDivisorANGLE);
 //  }
}

void VRWebGL_glAttachShader(GLuint program, GLuint shader)
{
	glAttachShader(program, shader);
}

GLuint VRWebGL_glCreateShader(GLenum type)
{
	return glCreateShader(type);
}

void VRWebGL_glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length)
{
	glShaderSource(shader, count, string, length);
}

void VRWebGL_glCompileShader(GLuint shader)
{
	glCompileShader(shader);
}

void VRWebGL_glGetShaderiv(	GLuint shader, GLenum pname, GLint *params)
{
	glGetShaderiv(shader, pname, params);
}

GLuint VRWebGL_glCreateProgram()
{
	return glCreateProgram();
}

void VRWebGL_glLinkProgram(GLuint program)
{
	glLinkProgram(program);
}

void VRWebGL_glGetProgramiv(GLuint program, GLenum pname, GLint *params)
{
	glGetProgramiv(program, pname, params);
}

void VRWebGL_glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog)
{
	glGetShaderInfoLog(shader, maxLength, length, infoLog);
}

void VRWebGL_glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog)
{
	glGetProgramInfoLog(program, maxLength, length, infoLog);
}

void VRWebGL_glUseProgram(GLuint program)
{
	glUseProgram(program);
}

GLint VRWebGL_glGetAttribLocation(GLuint program, const GLchar *name)
{
	return glGetAttribLocation(program, name);
}

GLint VRWebGL_glGetUniformLocation(GLuint program, const GLchar *name)
{
	return glGetUniformLocation(program, name);
}

void VRWebGL_glGenBuffers(GLsizei n, GLuint* buffers)
{
	glGenBuffers(n, buffers);
}

void VRWebGL_glBindBuffer(GLenum target, GLuint buffer)
{
	glBindBuffer(target, buffer);
}

void VRWebGL_glBufferData(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage)
{
	glBufferData(target, size, data, usage);
}

void VRWebGL_glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	glViewport(x, y, width, height);
}

void VRWebGL_glClear(GLbitfield mask)
{
	glClear(mask);
}

void VRWebGL_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	glClearColor(red, green, blue, alpha);
}

void VRWebGL_glClearDepthf(GLclampf depth)
{
	glClearDepthf(depth);
}

void VRWebGL_glClearStencil(GLint s)
{
	glClearStencil(s);
}

void VRWebGL_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	glColorMask(red, green, blue, alpha);
}

void VRWebGL_glEnableVertexAttribArray(GLuint index)
{
	glEnableVertexAttribArray(index);
}

void VRWebGL_glDisableVertexAttribArray(GLuint index)
{
	glDisableVertexAttribArray(index);
}

void VRWebGL_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
{
	glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void VRWebGL_glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	glUniformMatrix2fv(location, count, transpose, value);
}

void VRWebGL_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	glUniformMatrix3fv(location, count, transpose, value);
}

void VRWebGL_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	glUniformMatrix4fv(location, count, transpose, value);
}

void VRWebGL_glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	glDrawArrays(mode, first, count);
}

void VRWebGL_glGenTextures(GLsizei n, GLuint * textures)
{
	glGenTextures(n, textures);
}

void VRWebGL_glEnable(GLenum cap)
{
    glEnable(cap);
}

void VRWebGL_glDisable(GLenum cap)
{
    glDisable(cap);
}

void VRWebGL_glActiveTexture(GLenum texture)
{
	glActiveTexture(texture);
}

void VRWebGL_glBindTexture(GLenum target, GLuint texture)
{
	glBindTexture(target, texture);
}

void VRWebGL_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices)
{
	glDrawElements(mode, count, type, indices);
}

void VRWebGL_glPixelStorei(GLenum pname, GLint param)
{
	glPixelStorei(pname, param);
}

void VRWebGL_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data)
{
	glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
}

void VRWebGL_glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	glTexParameterf(target, pname, param);
}

void VRWebGL_glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	glTexParameteri(target, pname, param);
}

void VRWebGL_glGenerateMipmap(GLenum target)
{
	glGenerateMipmap(target);
}

void VRWebGL_glUniform1i(GLint location, GLint v0)
{
	glUniform1i(location, v0);
}

void VRWebGL_glUniform1iv(GLint location, GLsizei count, const GLint *value)
{
	glUniform1iv(location, count, value);
}

void VRWebGL_glUniform1f(GLint location, GLfloat v0)
{
	glUniform1f(location, v0);
}

void VRWebGL_glUniform1fv(GLint location, GLsizei count, const GLfloat *value)
{
	glUniform1fv(location, count, value);
}

void VRWebGL_glUniform2f(GLint location, GLfloat v0, GLfloat v1)
{
	glUniform2f(location, v0, v1);
}

void VRWebGL_glUniform2fv(GLint location, GLsizei count, const GLfloat *value)
{
	glUniform2fv(location, count, value);
}

void VRWebGL_glUniform2i(GLint location, GLint v0, GLint v1)
{
	glUniform2i(location, v0, v1);
}

void VRWebGL_glUniform2iv(GLint location, GLsizei count, const GLint *value)
{
	glUniform2iv(location, count, value);
}

void VRWebGL_glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
	glUniform3f(location, v0, v1, v2);
}

void VRWebGL_glUniform3fv(GLint location, GLsizei count, const GLfloat *value)
{
	glUniform3fv(location, count, value);
}

void VRWebGL_glUniform3i(GLint location, GLint v0, GLint v1, GLint v2)
{
	glUniform3i(location, v0, v1, v2);
}

void VRWebGL_glUniform3iv(GLint location, GLsizei count, const GLint *value)
{
	glUniform3iv(location, count, value);
}

void VRWebGL_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
	glUniform4f(location, v0, v1, v2, v3);
}

void VRWebGL_glUniform4fv(GLint location, GLsizei count, const GLfloat *value)
{
	glUniform4fv(location, count, value);
}

void VRWebGL_glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
	glUniform4i(location, v0, v1, v2, v3);
}

void VRWebGL_glUniform4iv(GLint location, GLsizei count, const GLint *value)
{
	glUniform4iv(location, count, value);
}

void VRWebGL_glGetBooleanv(GLenum pname, GLboolean * params)
{
	glGetBooleanv(pname, params);
}

void VRWebGL_glGetFloatv(GLenum pname, GLfloat * params)
{
	glGetFloatv(pname, params);
}

void VRWebGL_glGetIntegerv(GLenum pname, GLint * params)
{
	glGetIntegerv(pname, params);
}

void VRWebGL_glGetShaderPrecisionFormat(GLenum shaderType, GLenum precisionType, GLint *range, GLint *precision)
{
	glGetShaderPrecisionFormat(shaderType, precisionType, range, precision);
}

void VRWebGL_glDepthFunc(GLenum func)
{
	glDepthFunc(func);
}

void VRWebGL_glDepthMask(GLboolean flag)
{
	glDepthMask(flag);
}

void VRWebGL_glDepthRangef(GLclampf nearVal, GLclampf farVal)
{
	glDepthRangef(nearVal, farVal);
}

void VRWebGL_glDetachShader(GLuint program, GLuint shader)
{
	glDetachShader(program, shader);
}

void VRWebGL_glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	glBlendColor(red, green, blue, alpha);
}

void VRWebGL_glBlendEquation(GLenum mode)
{
	glBlendEquation(mode);
}

void VRWebGL_glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
	glBlendEquationSeparate(modeRGB, modeAlpha);
}

void VRWebGL_glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	glBlendFunc(sfactor, dfactor);
}

void VRWebGL_glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void VRWebGL_glDeleteBuffers(GLsizei n, const GLuint * buffers)
{
	glDeleteBuffers(n, buffers);
}

void VRWebGL_glDeleteFramebuffers(GLsizei n, const GLuint * framebuffers)
{
	glDeleteFramebuffers(n, framebuffers);
}

void VRWebGL_glDeleteRenderbuffers(GLsizei n, const GLuint * renderbuffers)
{
	glDeleteRenderbuffers(n, renderbuffers);
}

void VRWebGL_glDeleteTextures(GLsizei n, const GLuint * textures)
{
	glDeleteTextures(n, textures);
}

void VRWebGL_glDeleteProgram(GLuint program)
{
	glDeleteProgram(program);
}	

void VRWebGL_glDeleteShader(GLuint shader)
{
	glDeleteShader(shader);
}

void VRWebGL_glCullFace(GLenum mode)
{
	glCullFace(mode);
}

void VRWebGL_glFrontFace(GLenum mode)
{
	glFrontFace(mode);
}

void VRWebGL_glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
	glGetActiveAttrib(program, index, bufSize, length, size, type, name);
}

void VRWebGL_glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
	glGetActiveUniform(program, index, bufSize, length, size, type, name);
}

void VRWebGL_glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	glScissor(x, y, width, height);
}

void VRWebGL_glBindAttribLocation(GLuint program, GLuint index, const GLchar *name)
{
	glBindAttribLocation(program, index, name);
}

void VRWebGL_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data)
{
	glBufferSubData(target, offset, size, data);
}

void VRWebGL_glGenFramebuffers(GLsizei n, GLuint * framebuffers)
{
	glGenFramebuffers(n, framebuffers);
}

void VRWebGL_glLineWidth(GLfloat width)
{
	glLineWidth(width);
}

void VRWebGL_glBindFramebuffer(GLenum target, GLuint framebuffer)
{
	glBindFramebuffer(target, framebuffer);
}

void VRWebGL_glGenRenderbuffers(GLsizei n, GLuint * renderbuffers)
{
	glGenRenderbuffers(n, renderbuffers);
}

void VRWebGL_glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
	glBindRenderbuffer(target, renderbuffer);
}

void VRWebGL_glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	glRenderbufferStorage(target, internalformat, width, height);
}

void VRWebGL_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void VRWebGL_glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

const GLubyte* VRWebGL_glGetString(GLenum name)
{
	return glGetString(name);
}

GLenum VRWebGL_glGetError(void)
{
	return glGetError();
}

GLenum VRWebGL_glCheckFramebufferStatus(GLenum target)
{
	return glCheckFramebufferStatus(target);
}

void VRWebGL_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * data)
{
	glReadPixels(x, y, width, height, format, type, data);
}

void VRWebGL_glDrawArraysInstancedANGLE(GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
	glDrawArraysInstanced(mode, first, count, primcount);
	// glDrawArraysInstancedANGLE(mode, first, count, primcount);
}

void VRWebGL_glDrawElementsInstancedANGLE(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount)
{
	glDrawElementsInstanced(mode, count, type, indices, primcount);
	// glDrawElementsInstancedANGLE(mode, count, type, indices, primcount);
}

void VRWebGL_glVertexAttribDivisorANGLE(GLuint index, GLuint divisor)
{
	glVertexAttribDivisor(index, divisor);	
	// glVertexAttribDivisorANGLE(index, divisor);	
}
