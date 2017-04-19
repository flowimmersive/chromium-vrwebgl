#ifndef VRWebGL_h
#define VRWebGL_h

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
// // #include <GLES3/gl3ext.h>

#include "modules/vr/VRWebGLCommand.h"

#include <memory>

namespace blink
{
    class VRWebGLUniformLocation;
    class VRWebGLProgram;
    class VRWebGLShader;
    class VRWebGLTexture;
    class VRWebGLBuffer;
    class VRWebGLActiveInfo;
    class VRWebGLFramebuffer;
    class VRWebGLRenderbuffer;
}

class VRWebGLCommand;

using blink::VRWebGLUniformLocation;
using blink::VRWebGLProgram;
using blink::VRWebGLShader;
using blink::VRWebGLTexture;
using blink::VRWebGLBuffer;
using blink::VRWebGLActiveInfo;
using blink::VRWebGLFramebuffer;
using blink::VRWebGLRenderbuffer;

// VRWebGL OpenGL functions. They are all implemented in the VRWebGL_gl.cpp file.
GLuint VRWebGL_glCreateShader(GLenum type);
void VRWebGL_glAttachShader(GLuint program, GLuint shader);
void VRWebGL_glCompileShader(GLuint shader);
void VRWebGL_glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
void VRWebGL_glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
GLuint VRWebGL_glCreateProgram();   
void VRWebGL_glLinkProgram(GLuint program);
void VRWebGL_glGetProgramiv(GLuint program, GLenum pname, GLint *params);
void VRWebGL_glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
void VRWebGL_glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void VRWebGL_glUseProgram(GLuint program);
GLint VRWebGL_glGetAttribLocation(GLuint program, const GLchar *name);
GLint VRWebGL_glGetUniformLocation(GLuint program, const GLchar *name);
void VRWebGL_glGenBuffers(GLsizei n, GLuint* buffers);
void VRWebGL_glBindBuffer(GLenum target, GLuint buffer);
void VRWebGL_glBufferData(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage); 
void VRWebGL_glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void VRWebGL_glClear(GLbitfield mask);
void VRWebGL_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void VRWebGL_glClearDepthf(GLclampf depth);
void VRWebGL_glClearStencil(GLint s);
void VRWebGL_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void VRWebGL_glEnableVertexAttribArray(GLuint index);
void VRWebGL_glDisableVertexAttribArray(GLuint index);
void VRWebGL_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);
void VRWebGL_glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void VRWebGL_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void VRWebGL_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void VRWebGL_glDrawArrays(GLenum mode, GLint first, GLsizei count);
void VRWebGL_glGenTextures(GLsizei n, GLuint * textures);
void VRWebGL_glEnable(GLenum cap);
void VRWebGL_glDisable(GLenum cap);
void VRWebGL_glActiveTexture(GLenum texture);
void VRWebGL_glBindTexture(GLenum target, GLuint texture);
void VRWebGL_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices);
void VRWebGL_glPixelStorei(GLenum pname, GLint param);
void VRWebGL_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data);
void VRWebGL_glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void VRWebGL_glTexParameteri(GLenum target, GLenum pname, GLint param);
void VRWebGL_glGenerateMipmap(GLenum target);
void VRWebGL_glUniform1i(GLint location, GLint v0);
void VRWebGL_glUniform1iv(GLint location, GLsizei count, const GLint *value);
void VRWebGL_glUniform1f(GLint location, GLfloat v0);
void VRWebGL_glUniform1fv(GLint location, GLsizei count, const GLfloat *value);
void VRWebGL_glUniform2f(GLint location, GLfloat v0, GLfloat v1);
void VRWebGL_glUniform2fv(GLint location, GLsizei count, const GLfloat *value);
void VRWebGL_glUniform2i(GLint location, GLint v0, GLint v1);
void VRWebGL_glUniform2iv(GLint location, GLsizei count, const GLint *value);
void VRWebGL_glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void VRWebGL_glUniform3fv(GLint location, GLsizei count, const GLfloat *value);
void VRWebGL_glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
void VRWebGL_glUniform3iv(GLint location, GLsizei count, const GLint *value);
void VRWebGL_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void VRWebGL_glUniform4fv(GLint location, GLsizei count, const GLfloat *value);
void VRWebGL_glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
void VRWebGL_glUniform4iv(GLint location, GLsizei count, const GLint *value);
void VRWebGL_glGetBooleanv(GLenum pname, GLboolean * params);
void VRWebGL_glGetFloatv(GLenum pname, GLfloat * params);
void VRWebGL_glGetIntegerv(GLenum pname, GLint * params);
void VRWebGL_glGetShaderPrecisionFormat(GLenum shaderType, GLenum precisionType, GLint *range, GLint *precision);
void VRWebGL_glDepthFunc(GLenum func);
void VRWebGL_glDepthMask(GLboolean flag);
void VRWebGL_glDepthRangef(GLclampf nearVal, GLclampf farVal);
void VRWebGL_glDetachShader(GLuint program, GLuint shader);
void VRWebGL_glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void VRWebGL_glBlendEquation(GLenum mode);
void VRWebGL_glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
void VRWebGL_glBlendFunc(GLenum sfactor, GLenum dfactor);
void VRWebGL_glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void VRWebGL_glDeleteBuffers(GLsizei n, const GLuint * buffers);
void VRWebGL_glDeleteFramebuffers(GLsizei n, const GLuint * framebuffers);
void VRWebGL_glDeleteRenderbuffers(GLsizei n, const GLuint * renderbuffers);
void VRWebGL_glDeleteTextures(GLsizei n,  const GLuint * textures);
void VRWebGL_glDeleteProgram(GLuint program);
void VRWebGL_glDeleteShader(GLuint shader);
void VRWebGL_glCullFace(GLenum mode);
void VRWebGL_glFrontFace(GLenum mode);
void VRWebGL_glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
void VRWebGL_glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
void VRWebGL_glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void VRWebGL_glBindAttribLocation(GLuint program, GLuint index, const GLchar *name);
void VRWebGL_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data);
void VRWebGL_glGenFramebuffers(GLsizei n, GLuint * framebuffers);
void VRWebGL_glLineWidth(GLfloat width);
void VRWebGL_glBindFramebuffer(GLenum target, GLuint framebuffer);
void VRWebGL_glGenRenderbuffers(GLsizei n, GLuint * renderbuffers);
void VRWebGL_glBindRenderbuffer(GLenum target, GLuint renderbuffer);
void VRWebGL_glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void VRWebGL_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void VRWebGL_glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
const GLubyte* VRWebGL_glGetString(GLenum name);
GLenum VRWebGL_glGetError(void);
GLenum VRWebGL_glCheckFramebufferStatus(GLenum target);
void VRWebGL_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * data);

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_activeTexture final: public VRWebGLCommand
{
private:
    GLenum m_texture;

    VRWebGLCommand_activeTexture(GLenum texture);
    
public:
    static std::shared_ptr<VRWebGLCommand_activeTexture> newInstance(GLenum texture);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_attachShader final: public VRWebGLCommand
{
private:
    VRWebGLProgram* m_program;
    VRWebGLShader* m_shader;
    bool m_processed = false;

    VRWebGLCommand_attachShader(VRWebGLProgram* program, VRWebGLShader* shader);

public:
    static std::shared_ptr<VRWebGLCommand_attachShader> newInstance(VRWebGLProgram* program, VRWebGLShader* shader);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_bindAttribLocation final: public VRWebGLCommand
{
private:
    VRWebGLProgram* m_program;
    GLuint m_index;
    std::string m_name;
    
    VRWebGLCommand_bindAttribLocation(VRWebGLProgram* program, GLuint index, const std::string& name);
    
public:
    static std::shared_ptr<VRWebGLCommand_bindAttribLocation> newInstance(VRWebGLProgram* program, GLuint index, const std::string& name);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_bindBuffer final: public VRWebGLCommand
{
private:
    GLenum m_target;
    VRWebGLBuffer* m_buffer;

    VRWebGLCommand_bindBuffer(GLenum target, VRWebGLBuffer* buffer);
    
public:
    static std::shared_ptr<VRWebGLCommand_bindBuffer> newInstance(GLenum target, VRWebGLBuffer* buffer);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_bindFramebuffer final: public VRWebGLCommand
{
private:
    GLenum m_target;
    VRWebGLFramebuffer* m_framebuffer;

    VRWebGLCommand_bindFramebuffer(GLenum target, VRWebGLFramebuffer* framebuffer);
    
public:
    static std::shared_ptr<VRWebGLCommand_bindFramebuffer> newInstance(GLenum target, VRWebGLFramebuffer* framebuffer);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_bindRenderbuffer final: public VRWebGLCommand
{
private:
    GLenum m_target;
    VRWebGLRenderbuffer* m_renderbuffer;

    VRWebGLCommand_bindRenderbuffer(GLenum target, VRWebGLRenderbuffer* renderbuffer);
    
public:
    static std::shared_ptr<VRWebGLCommand_bindRenderbuffer> newInstance(GLenum target, VRWebGLRenderbuffer* renderbuffer);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_bindTexture final: public VRWebGLCommand
{
private:
    GLenum m_target;
    VRWebGLTexture* m_texture;

    VRWebGLCommand_bindTexture(GLenum target, VRWebGLTexture* texture);
    
public:
    static std::shared_ptr<VRWebGLCommand_bindTexture> newInstance(GLenum target, VRWebGLTexture* texture);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_blendColor final: public VRWebGLCommand
{
private:
    GLfloat m_red, m_green, m_blue, m_alpha;

    VRWebGLCommand_blendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    
public:
    static std::shared_ptr<VRWebGLCommand_blendColor> newInstance(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_blendEquation final: public VRWebGLCommand
{
private:
    GLenum m_mode;

    VRWebGLCommand_blendEquation(GLenum mode);
    
public:
    static std::shared_ptr<VRWebGLCommand_blendEquation> newInstance(GLenum mode);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_blendEquationSeparate final: public VRWebGLCommand
{
private:
    GLenum m_modeRGB;
    GLenum m_modeAlpha;

    VRWebGLCommand_blendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
    
public:
    static std::shared_ptr<VRWebGLCommand_blendEquationSeparate> newInstance(GLenum modeRGB, GLenum modeAlpha);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_blendFunc final: public VRWebGLCommand
{
private:
    GLenum m_sfactor;
    GLenum m_dfactor;

    VRWebGLCommand_blendFunc(GLenum sfactor, GLenum dfactor);
    
public:
    static std::shared_ptr<VRWebGLCommand_blendFunc> newInstance(GLenum sfactor, GLenum dfactor);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_blendFuncSeparate final: public VRWebGLCommand
{
private:
    GLenum m_srcRGB;
    GLenum m_dstRGB;
    GLenum m_srcAlpha;
    GLenum m_dstAlpha;

    VRWebGLCommand_blendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    
public:
    static std::shared_ptr<VRWebGLCommand_blendFuncSeparate> newInstance(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_bufferData final: public VRWebGLCommand
{
private:
    GLenum m_target;
    GLsizeiptr m_size;
    GLbyte* m_data;
    GLenum m_usage;
    bool m_processed = false;

    VRWebGLCommand_bufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
    
public:
    static std::shared_ptr<VRWebGLCommand_bufferData> newInstance(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);

    virtual ~VRWebGLCommand_bufferData();
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_bufferSubData final: public VRWebGLCommand
{
private:
    GLenum m_target;
    GLintptr m_offset;
    GLsizeiptr m_size;
    GLbyte* m_data;

    VRWebGLCommand_bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data);
    
public:
    static std::shared_ptr<VRWebGLCommand_bufferSubData> newInstance(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data);

    virtual ~VRWebGLCommand_bufferSubData();
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_checkFramebufferStatus final: public VRWebGLCommand
{
private:
    GLenum m_target;
    GLenum m_status;

    VRWebGLCommand_checkFramebufferStatus(GLenum target);
    
public:
    static std::shared_ptr<VRWebGLCommand_checkFramebufferStatus> newInstance(GLenum target);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_clear final: public VRWebGLCommand
{
private:
    GLbitfield m_mask;
    bool m_processed = false;

    VRWebGLCommand_clear(GLbitfield mask);
    
public:
    static std::shared_ptr<VRWebGLCommand_clear> newInstance(GLbitfield mask);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_clearColor final: public VRWebGLCommand
{
private:
    GLfloat m_red, m_green, m_blue, m_alpha;
    bool m_processed = false;

    VRWebGLCommand_clearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    
public:
    static std::shared_ptr<VRWebGLCommand_clearColor> newInstance(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_clearDepthf final: public VRWebGLCommand
{
private:
    GLclampf m_depth;

    VRWebGLCommand_clearDepthf(GLclampf depth);
    
public:
    static std::shared_ptr<VRWebGLCommand_clearDepthf> newInstance(GLclampf depth);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_clearStencil final: public VRWebGLCommand
{
private:
    GLint m_s;

    VRWebGLCommand_clearStencil(GLint s);
    
public:
    static std::shared_ptr<VRWebGLCommand_clearStencil> newInstance(GLint s);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_colorMask final: public VRWebGLCommand
{
private:
    GLfloat m_red, m_green, m_blue, m_alpha;

    VRWebGLCommand_colorMask(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    
public:
    static std::shared_ptr<VRWebGLCommand_colorMask> newInstance(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_compileShader final: public VRWebGLCommand
{
private:
    VRWebGLShader* m_shader;
    bool m_processed = false;

    VRWebGLCommand_compileShader(VRWebGLShader* shader);

public:
    static std::shared_ptr<VRWebGLCommand_compileShader> newInstance(VRWebGLShader* shader);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_createBuffer final: public VRWebGLCommand
{
private:
    VRWebGLBuffer* m_buffer;
    bool m_processed = false;

    VRWebGLCommand_createBuffer(VRWebGLBuffer* buffer);
    
public:
    static std::shared_ptr<VRWebGLCommand_createBuffer> newInstance(VRWebGLBuffer* buffer);
    
    virtual bool isSynchronous() const override;

    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_createFramebuffer final: public VRWebGLCommand
{
private:
    VRWebGLFramebuffer* m_framebuffer;
    bool m_processed = false;

    VRWebGLCommand_createFramebuffer(VRWebGLFramebuffer* framebuffer);
    
public:
    static std::shared_ptr<VRWebGLCommand_createFramebuffer> newInstance(VRWebGLFramebuffer* framebuffer);
    
    virtual bool isSynchronous() const override;

    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_createProgram final: public VRWebGLCommand
{
private:
    VRWebGLProgram* m_program;
    bool m_processed = false;

    VRWebGLCommand_createProgram(VRWebGLProgram* program);
    
public:
    static std::shared_ptr<VRWebGLCommand_createProgram> newInstance(VRWebGLProgram* program);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
       
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_createRenderbuffer final: public VRWebGLCommand
{
private:
    VRWebGLRenderbuffer* m_renderbuffer;
    bool m_processed = false;

    VRWebGLCommand_createRenderbuffer(VRWebGLRenderbuffer* renderbuffer);
    
public:
    static std::shared_ptr<VRWebGLCommand_createRenderbuffer> newInstance(VRWebGLRenderbuffer* renderbuffer);
    
    virtual bool isSynchronous() const override;

    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_createShader final: public VRWebGLCommand
{
private:
    VRWebGLShader* m_shader = 0;
    GLenum m_type;
    bool m_processed = false;

    VRWebGLCommand_createShader(VRWebGLShader* shader, GLenum type);

public:
    static std::shared_ptr<VRWebGLCommand_createShader> newInstance(VRWebGLShader* shader, GLenum type);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_createTexture final: public VRWebGLCommand
{
private:
    VRWebGLTexture* m_texture = 0;
    bool m_processed = false;

    VRWebGLCommand_createTexture(VRWebGLTexture* texture);

public:
    static std::shared_ptr<VRWebGLCommand_createTexture> newInstance(VRWebGLTexture* texture);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_cullFace final: public VRWebGLCommand
{
private:
    GLenum m_mode;

    VRWebGLCommand_cullFace(GLenum mode);
    
public:
    static std::shared_ptr<VRWebGLCommand_cullFace> newInstance(GLenum mode);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_deleteBuffer final: public VRWebGLCommand
{
private:
    VRWebGLBuffer* m_buffer;
    bool m_processed = false;
    
    VRWebGLCommand_deleteBuffer(VRWebGLBuffer* buffer);
    
public:
    static std::shared_ptr<VRWebGLCommand_deleteBuffer> newInstance(VRWebGLBuffer* buffer);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_deleteFramebuffer final: public VRWebGLCommand
{
private:
    VRWebGLFramebuffer* m_framebuffer;
    bool m_processed = false;
    
    VRWebGLCommand_deleteFramebuffer(VRWebGLFramebuffer* framebuffer);
    
public:
    static std::shared_ptr<VRWebGLCommand_deleteFramebuffer> newInstance(VRWebGLFramebuffer* framebuffer);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_deleteProgram final: public VRWebGLCommand
{
private:
    VRWebGLProgram* m_program;
    bool m_processed = false;
    
    VRWebGLCommand_deleteProgram(VRWebGLProgram* program);
    
public:
    static std::shared_ptr<VRWebGLCommand_deleteProgram> newInstance(VRWebGLProgram* program);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_deleteRenderbuffer final: public VRWebGLCommand
{
private:
    VRWebGLRenderbuffer* m_renderbuffer;
    bool m_processed = false;
    
    VRWebGLCommand_deleteRenderbuffer(VRWebGLRenderbuffer* renderbuffer);
    
public:
    static std::shared_ptr<VRWebGLCommand_deleteRenderbuffer> newInstance(VRWebGLRenderbuffer* renderbuffer);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_deleteShader final: public VRWebGLCommand
{
private:
    VRWebGLShader* m_shader;
    bool m_processed = false;
    
    VRWebGLCommand_deleteShader(VRWebGLShader* shader);
    
public:
    static std::shared_ptr<VRWebGLCommand_deleteShader> newInstance(VRWebGLShader* shader);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_deleteTexture final: public VRWebGLCommand
{
private:
    VRWebGLTexture* m_texture;
    bool m_processed = false;
    
    VRWebGLCommand_deleteTexture(VRWebGLTexture* texture);
    
public:
    static std::shared_ptr<VRWebGLCommand_deleteTexture> newInstance(VRWebGLTexture* texture);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_depthFunc final: public VRWebGLCommand
{
private:
    GLenum m_func;

    VRWebGLCommand_depthFunc(GLenum func);
    
public:
    static std::shared_ptr<VRWebGLCommand_depthFunc> newInstance(GLenum func);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_depthMask final: public VRWebGLCommand
{
private:
    GLboolean m_flag;

    VRWebGLCommand_depthMask(GLboolean flag);
    
public:
    static std::shared_ptr<VRWebGLCommand_depthMask> newInstance(GLboolean flag);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_depthRangef final: public VRWebGLCommand
{
private:
    GLclampf m_nearVal;
    GLclampf m_farVal;

    VRWebGLCommand_depthRangef(GLclampf nearVal, GLclampf farVal);
    
public:
    static std::shared_ptr<VRWebGLCommand_depthRangef> newInstance(GLclampf nearVal, GLclampf farVal);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_detachShader final: public VRWebGLCommand
{
private:
    VRWebGLProgram* m_program;
    VRWebGLShader* m_shader;

    VRWebGLCommand_detachShader(VRWebGLProgram* program, VRWebGLShader* shader);

public:
    static std::shared_ptr<VRWebGLCommand_detachShader> newInstance(VRWebGLProgram* program, VRWebGLShader* shader);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_disable final: public VRWebGLCommand
{
private:
    GLenum m_cap;

    VRWebGLCommand_disable(GLenum cap);
    
public:
    static std::shared_ptr<VRWebGLCommand_disable> newInstance(GLenum cap);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_disableVertexAttribArray final: public VRWebGLCommand
{
private:
    GLuint m_index;

    VRWebGLCommand_disableVertexAttribArray(GLuint index);
    
public:
    static std::shared_ptr<VRWebGLCommand_disableVertexAttribArray> newInstance(GLuint index);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_drawArrays final: public VRWebGLCommand
{
private:
    GLenum m_mode;
    GLint m_first;
    GLsizei m_count;

    VRWebGLCommand_drawArrays(GLenum mode, GLint first, GLsizei count);
    
public:
    static std::shared_ptr<VRWebGLCommand_drawArrays> newInstance(GLenum mode, GLint first, GLsizei count);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_drawElements final: public VRWebGLCommand
{
private:
    GLenum m_mode;
    GLsizei m_count;
    GLenum m_type;
    long long m_offset;

    VRWebGLCommand_drawElements(GLenum mode, GLsizei count, GLenum type, long long offset);
    
public:
    static std::shared_ptr<VRWebGLCommand_drawElements> newInstance(GLenum mode, GLsizei count, GLenum type, long long offset);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_enable final: public VRWebGLCommand
{
private:
    GLenum m_cap;

    VRWebGLCommand_enable(GLenum cap);
    
public:
    static std::shared_ptr<VRWebGLCommand_enable> newInstance(GLenum cap);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_enableVertexAttribArray final: public VRWebGLCommand
{
private:
    GLuint m_index;

    VRWebGLCommand_enableVertexAttribArray(GLuint index);
    
public:
    static std::shared_ptr<VRWebGLCommand_enableVertexAttribArray> newInstance(GLuint index);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_framebufferRenderbuffer final: public VRWebGLCommand
{
private:
    GLenum m_target;
    GLenum m_attachment;
    GLenum m_renderbuffertarget;
    VRWebGLRenderbuffer* m_renderbuffer;

    VRWebGLCommand_framebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, VRWebGLRenderbuffer* renderbuffer);
    
public:
    static std::shared_ptr<VRWebGLCommand_framebufferRenderbuffer> newInstance(GLenum target, GLenum attachment, GLenum renderbuffertarget, VRWebGLRenderbuffer* renderbuffer);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_framebufferTexture2D final: public VRWebGLCommand
{
private:
    GLenum m_target;
    GLenum m_attachment;
    GLenum m_textarget;
    VRWebGLTexture* m_texture;
    GLint m_level;

    VRWebGLCommand_framebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, VRWebGLTexture* texture, GLint level);
    
public:
    static std::shared_ptr<VRWebGLCommand_framebufferTexture2D> newInstance(GLenum target, GLenum attachment, GLenum textarget, VRWebGLTexture* texture, GLint level);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_frontFace final: public VRWebGLCommand
{
private:
    GLenum m_mode;

    VRWebGLCommand_frontFace(GLenum mode);
    
public:
    static std::shared_ptr<VRWebGLCommand_frontFace> newInstance(GLenum mode);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_generateMipmap final: public VRWebGLCommand
{
private:
    GLenum m_target;
    bool m_processed = false;

    VRWebGLCommand_generateMipmap(GLenum target);
    
public:
    static std::shared_ptr<VRWebGLCommand_generateMipmap> newInstance(GLenum target);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getActiveAttrib final: public VRWebGLCommand
{
private:
    VRWebGLProgram* m_program;
    GLuint m_index;
    GLsizei m_bufSize;
    GLsizei* m_length;
    GLint* m_size;
    GLenum* m_type;
    GLchar* m_name;

    VRWebGLCommand_getActiveAttrib(VRWebGLProgram* program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
    
public:
    static std::shared_ptr<VRWebGLCommand_getActiveAttrib> newInstance(VRWebGLProgram* program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getActiveUniform final: public VRWebGLCommand
{
private:
    VRWebGLProgram* m_program;
    GLuint m_index;
    GLsizei m_bufSize;
    GLsizei* m_length;
    GLint* m_size;
    GLenum* m_type;
    GLchar* m_name;
    
    VRWebGLCommand_getActiveUniform(VRWebGLProgram* program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
    
public:
    static std::shared_ptr<VRWebGLCommand_getActiveUniform> newInstance(VRWebGLProgram* program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getAttribLocation final: public VRWebGLCommand
{
private:
    VRWebGLProgram* m_program;
    std::string m_name;
    GLint m_attribLocation = 0;
    
    VRWebGLCommand_getAttribLocation(VRWebGLProgram* program, const std::string& name);
    
public:
    static std::shared_ptr<VRWebGLCommand_getAttribLocation> newInstance(VRWebGLProgram* program, const std::string& name);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getError final: public VRWebGLCommand
{
private:
    GLenum m_error;

    VRWebGLCommand_getError();
    
public:
    static std::shared_ptr<VRWebGLCommand_getError> newInstance();
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getBooleanv final: public VRWebGLCommand
{
private:
    GLenum m_pname;
    GLboolean m_value[4];

    VRWebGLCommand_getBooleanv(GLenum pname);

public:
    static std::shared_ptr<VRWebGLCommand_getBooleanv> newInstance(GLenum pname);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getFloatv final: public VRWebGLCommand
{
private:
    GLenum m_pname;
    GLfloat m_value[4];

    VRWebGLCommand_getFloatv(GLenum pname);

public:
    static std::shared_ptr<VRWebGLCommand_getFloatv> newInstance(GLenum pname);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getIntegerv final: public VRWebGLCommand
{
private:
    GLenum m_pname;
    GLint m_value[4];

    VRWebGLCommand_getIntegerv(GLenum pname);

public:
    static std::shared_ptr<VRWebGLCommand_getIntegerv> newInstance(GLenum pname);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getProgramiv final: public VRWebGLCommand
{
private:
    VRWebGLProgram* m_program;
    GLenum m_pname;
    GLint m_programParameter = 0;
    
    VRWebGLCommand_getProgramiv(VRWebGLProgram* program, GLenum pname);
    
public:
    static std::shared_ptr<VRWebGLCommand_getProgramiv> newInstance(VRWebGLProgram* program, GLenum pname);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getProgramInfoLog final: public VRWebGLCommand
{
private:
    VRWebGLProgram* m_program;
    std::string m_programInfoLog;
    
    VRWebGLCommand_getProgramInfoLog(VRWebGLProgram* program);
    
public:
    static std::shared_ptr<VRWebGLCommand_getProgramInfoLog> newInstance(VRWebGLProgram* program);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getShaderiv final: public VRWebGLCommand
{
private:
    VRWebGLShader* m_shader;
    GLenum m_pname;
    GLint m_shaderParameter;
    
    VRWebGLCommand_getShaderiv(VRWebGLShader* shader, GLenum pname);
    
public:
    static std::shared_ptr<VRWebGLCommand_getShaderiv> newInstance(VRWebGLShader* shader, GLenum pname);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getShaderInfoLog final: public VRWebGLCommand
{
private:
    VRWebGLShader* m_shader;
    std::string m_shaderInfoLog;
    
    VRWebGLCommand_getShaderInfoLog(VRWebGLShader* shader);
    
public:
    static std::shared_ptr<VRWebGLCommand_getShaderInfoLog> newInstance(VRWebGLShader* shader);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getShaderPrecisionFormat final: public VRWebGLCommand
{
private:
    GLenum m_shaderType;
    GLenum m_precisionType;
    GLint m_rangeAndPrecision[3];

    VRWebGLCommand_getShaderPrecisionFormat(GLenum shaderType, GLenum precisionType);
    
public:
    static std::shared_ptr<VRWebGLCommand_getShaderPrecisionFormat> newInstance(GLenum shaderType, GLenum precisionType);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getUniformLocation final: public VRWebGLCommand
{
private:
    VRWebGLUniformLocation* m_uniformLocation;
    VRWebGLProgram* m_program;
    std::string m_name;

    VRWebGLCommand_getUniformLocation(VRWebGLUniformLocation* uniformLocation, VRWebGLProgram* program, const std::string& name);
    
public:
    static std::shared_ptr<VRWebGLCommand_getUniformLocation> newInstance(VRWebGLUniformLocation* uniformLocation, VRWebGLProgram* program, const std::string& name);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_lineWidth final: public VRWebGLCommand
{
public:
    GLfloat m_width;

    VRWebGLCommand_lineWidth(GLfloat width);
    
public:
    static std::shared_ptr<VRWebGLCommand_lineWidth> newInstance(GLfloat width);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_linkProgram final: public VRWebGLCommand
{
public:
    VRWebGLProgram* m_program;
    bool m_processed = false;

    VRWebGLCommand_linkProgram(VRWebGLProgram* program);
    
public:
    static std::shared_ptr<VRWebGLCommand_linkProgram> newInstance(VRWebGLProgram* program);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_pixelStorei final: public VRWebGLCommand
{
public:
    GLenum m_pname;
    GLint m_param;

    VRWebGLCommand_pixelStorei(GLenum pname, GLint param);
    
public:
    static std::shared_ptr<VRWebGLCommand_pixelStorei> newInstance(GLenum pname, GLint param);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_readPixels: public VRWebGLCommand
{
private:
    GLint m_x;
    GLint m_y;
    GLsizei m_width;
    GLsizei m_height;
    GLenum m_format;
    GLenum m_type;
    GLvoid* m_data;

    VRWebGLCommand_readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * data);
    
public:
    static std::shared_ptr<VRWebGLCommand_readPixels> newInstance(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * data);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_renderbufferStorage: public VRWebGLCommand
{
private:
    GLenum m_target;
    GLenum m_internalformat;
    GLsizei m_width;
    GLsizei m_height;

    VRWebGLCommand_renderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    
public:
    static std::shared_ptr<VRWebGLCommand_renderbufferStorage> newInstance(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_scissor: public VRWebGLCommand
{
private:
    GLint m_x;
    GLint m_y;
    GLsizei m_width;
    GLsizei m_height;

    VRWebGLCommand_scissor(GLint x, GLint y, GLsizei width, GLsizei height);
    
public:
    static std::shared_ptr<VRWebGLCommand_scissor> newInstance(GLint x, GLint y, GLsizei width, GLsizei height);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_shaderSource final: public VRWebGLCommand
{
private:
    VRWebGLShader* m_shader;
    GLchar* m_source;
    GLint m_length;
    bool m_processed = false;

    VRWebGLCommand_shaderSource(VRWebGLShader* shader, const GLchar* source, GLint length);

public:
    static std::shared_ptr<VRWebGLCommand_shaderSource> newInstance(VRWebGLShader* shader, const GLchar* source, GLint length);
    
    virtual ~VRWebGLCommand_shaderSource();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_texParameteri final: public VRWebGLCommand
{
private:
    GLenum m_target;
    GLenum m_pname;
    GLint m_param;
    // This attribute should not be needed but added to fix external texture handling for filters.
    VRWebGLTexture* m_texture;

    VRWebGLCommand_texParameteri(GLenum target, GLenum pname,GLint param, VRWebGLTexture* texture);

public:
    static std::shared_ptr<VRWebGLCommand_texParameteri> newInstance(GLenum target, GLenum pname, GLint param, VRWebGLTexture* texture);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_texParameterf final: public VRWebGLCommand
{
private:
    GLenum m_target;
    GLenum m_pname;
    GLfloat m_param;

    VRWebGLCommand_texParameterf(GLenum target, GLenum pname, GLfloat param);

public:
    static std::shared_ptr<VRWebGLCommand_texParameterf> newInstance(GLenum target, GLenum pname, GLfloat param);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};
        
// ======================================================================================
// ======================================================================================

class VRWebGLCommand_texImage2D final: public VRWebGLCommand
{
public:
    GLenum m_target;
    GLint m_level;
    GLint m_internalformat;
    GLsizei m_width;
    GLsizei m_height;
    GLint m_border;
    GLenum m_format;
    GLenum m_type;
    GLbyte* m_data;
    bool m_processed = false;

    VRWebGLCommand_texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data);
    
public:
    static std::shared_ptr<VRWebGLCommand_texImage2D> newInstance(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data);
    
    virtual ~VRWebGLCommand_texImage2D();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform1i: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLint m_x;

    VRWebGLCommand_uniform1i(const VRWebGLUniformLocation* location, GLint x);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform1i> newInstance(const VRWebGLUniformLocation* location, GLint x);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform1iv: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLsizei m_count;
    GLint* m_value;

    VRWebGLCommand_uniform1iv(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform1iv> newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value);
    
    virtual ~VRWebGLCommand_uniform1iv();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform1f: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLfloat m_x;

    VRWebGLCommand_uniform1f(const VRWebGLUniformLocation* location, GLfloat x);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform1f> newInstance(const VRWebGLUniformLocation* location, GLfloat x);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform1fv: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLsizei m_count;
    GLfloat* m_value;

    VRWebGLCommand_uniform1fv(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform1fv> newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value);
    
    virtual ~VRWebGLCommand_uniform1fv();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform2f: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLfloat m_x;
    GLfloat m_y;

    VRWebGLCommand_uniform2f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform2f> newInstance(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform2fv: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLsizei m_count;
    GLfloat* m_value;

    VRWebGLCommand_uniform2fv(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform2fv> newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value);
    
    virtual ~VRWebGLCommand_uniform2fv();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform2i: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLint m_x;
    GLint m_y;

    VRWebGLCommand_uniform2i(const VRWebGLUniformLocation* location, GLint x, GLint y);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform2i> newInstance(const VRWebGLUniformLocation* location, GLint x, GLint y);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform2iv: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLsizei m_count;
    GLint* m_value;

    VRWebGLCommand_uniform2iv(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform2iv> newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value);
    
    virtual ~VRWebGLCommand_uniform2iv();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform3f: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLfloat m_x;
    GLfloat m_y;
    GLfloat m_z;

    VRWebGLCommand_uniform3f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform3f> newInstance(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform3fv: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLsizei m_count;
    GLfloat* m_value;

    VRWebGLCommand_uniform3fv(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform3fv> newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value);
    
    virtual ~VRWebGLCommand_uniform3fv();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform3i: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLint m_x;
    GLint m_y;
    GLint m_z;

    VRWebGLCommand_uniform3i(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform3i> newInstance(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform3iv: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLsizei m_count;
    GLint* m_value;

    VRWebGLCommand_uniform3iv(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform3iv> newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value);
    
    virtual ~VRWebGLCommand_uniform3iv();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform4f: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLfloat m_x;
    GLfloat m_y;
    GLfloat m_z;
    GLfloat m_w;

    VRWebGLCommand_uniform4f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform4f> newInstance(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform4fv: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLsizei m_count;
    GLfloat* m_value;

    VRWebGLCommand_uniform4fv(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform4fv> newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value);
    
    virtual ~VRWebGLCommand_uniform4fv();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform4i: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLint m_x;
    GLint m_y;
    GLint m_z;
    GLint m_w;

    VRWebGLCommand_uniform4i(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z, GLint w);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform4i> newInstance(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z, GLint w);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniform4iv: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLsizei m_count;
    GLint* m_value;

    VRWebGLCommand_uniform4iv(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniform4iv> newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value);
    
    virtual ~VRWebGLCommand_uniform4iv();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniformMatrix2fv: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLsizei m_count;
    GLboolean m_transpose;
    GLfloat* m_value;

    VRWebGLCommand_uniformMatrix2fv(const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniformMatrix2fv> newInstance(const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value);
    
    virtual ~VRWebGLCommand_uniformMatrix2fv();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniformMatrix3fv: public VRWebGLCommand
{
private:
    const VRWebGLUniformLocation* m_location;
    GLsizei m_count;
    GLboolean m_transpose;
    GLfloat* m_value;

    VRWebGLCommand_uniformMatrix3fv(const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniformMatrix3fv> newInstance(const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value);
    
    virtual ~VRWebGLCommand_uniformMatrix3fv();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_uniformMatrix4fv: public VRWebGLCommand
{
private:
    const VRWebGLProgram* m_program;
    const VRWebGLUniformLocation* m_location;
    GLsizei m_count;
    GLboolean m_transpose;
    GLfloat* m_value;

    VRWebGLCommand_uniformMatrix4fv(const VRWebGLProgram* program, const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value);
    
public:
    static std::shared_ptr<VRWebGLCommand_uniformMatrix4fv> newInstance(const VRWebGLProgram* program, const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value);
    
    virtual ~VRWebGLCommand_uniformMatrix4fv();

    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_useProgram: public VRWebGLCommand
{
private:
    VRWebGLProgram* m_program;

    VRWebGLCommand_useProgram(VRWebGLProgram* program);
    
public:
    static std::shared_ptr<VRWebGLCommand_useProgram> newInstance(VRWebGLProgram* program);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_vertexAttribPointer: public VRWebGLCommand
{
private:
    GLuint m_index;
    GLint m_size;
    GLenum m_type;
    GLboolean m_normalized;
    GLsizei m_stride;
    long long m_offset;

    VRWebGLCommand_vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, long long offset);
    
public:
    static std::shared_ptr<VRWebGLCommand_vertexAttribPointer> newInstance(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, long long offset);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_viewport: public VRWebGLCommand
{
private:
    GLint m_x;
    GLint m_y;
    GLsizei m_width;
    GLsizei m_height;
    bool m_useViewportFromCommandProcessor;

    VRWebGLCommand_viewport(GLint x, GLint y, GLsizei width, GLsizei height, bool useViewportFromCommandProcessor);
    
public:
    static std::shared_ptr<VRWebGLCommand_viewport> newInstance(GLint x, GLint y, GLsizei width, GLsizei height, bool useViewportFromCommandProcessor);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

class VRWebGLCommand_getString final: public VRWebGLCommand
{
private:
    GLenum m_pname;

    VRWebGLCommand_getString(GLenum pname);
    
public:
    static std::shared_ptr<VRWebGLCommand_getString> newInstance(GLenum pname);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

// This is not a WebGL/OpenGL command per se. We use it to set the camera world matrix in the right order among other VRWebGLCommands
class VRWebGLCommand_setCameraWorldMatrix final: public VRWebGLCommand
{
private:
    GLfloat m_matrix[16];
    bool m_processed = false;

    VRWebGLCommand_setCameraWorldMatrix(const GLfloat* matrix);
    
public:
    static std::shared_ptr<VRWebGLCommand_setCameraWorldMatrix> newInstance(const GLfloat* matrix);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// ======================================================================================
// ======================================================================================

// This is not a WebGL/OpenGL command per se. We use it to set the camera world matrix in the right order among other VRWebGLCommands
class VRWebGLCommand_updateSurfaceTexture: public VRWebGLCommand
{
private:
    const GLuint m_textureId;
    bool m_processed = false;

    VRWebGLCommand_updateSurfaceTexture(GLuint textureId);
    
public:
    static std::shared_ptr<VRWebGLCommand_updateSurfaceTexture> newInstance(GLuint textureId);
    
    virtual bool isSynchronous() const override;
    
    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
        
    virtual std::string name() const override;
};
#endif // VRWebGL_h