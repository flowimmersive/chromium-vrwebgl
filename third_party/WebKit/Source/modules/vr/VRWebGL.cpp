/**
IMPORTANT: This file includes all the implementation of VRWebGLCommands except from the part related to GL calls that is available in
the VRWebGL_gl.cpp file. These 2 files need to be compiled in different libraries to avoid references to the blink IDL elements.
The VR SDK parts do not need to know anything about IDL stuff, just the GL calls and actually, the IDL side is quite complicated to be
built if it makes any references to GL calls. Of course, during linkage, they will both know each other to be able to correctly link.
*/

#include "modules/vr/VRWebGL.h"

#include "modules/vr/VRWebGLCommandProcessor.h"

#include "modules/vr/VRWebGLUniformLocation.h"
#include "modules/vr/VRWebGLProgram.h"
#include "modules/vr/VRWebGLShader.h"
#include "modules/vr/VRWebGLTexture.h"
#include "modules/vr/VRWebGLBuffer.h"
#include "modules/vr/VRWebGLActiveInfo.h"
#include "modules/vr/VRWebGLFramebuffer.h"
#include "modules/vr/VRWebGLRenderbuffer.h"
#include "modules/vr/VRWebGLMath.h"

#include <cstring>

// ======================================================================================
// ======================================================================================

VRWebGLCommand_activeTexture::VRWebGLCommand_activeTexture(GLenum texture): m_texture(texture)
{
}
    
std::shared_ptr<VRWebGLCommand_activeTexture> VRWebGLCommand_activeTexture::newInstance(GLenum texture)
{
    return std::shared_ptr<VRWebGLCommand_activeTexture>(new VRWebGLCommand_activeTexture(texture));
}

bool VRWebGLCommand_activeTexture::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_activeTexture::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_activeTexture::process() 
{
    VRWebGL_glActiveTexture(m_texture);
#ifdef VRWEBGL_SHOW_LOG    
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " texture = << " << m_texture;
#endif
    return 0;
}

std::string VRWebGLCommand_activeTexture::name() const 
{
    return "activeTexture";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_attachShader::VRWebGLCommand_attachShader(VRWebGLProgram* program, VRWebGLShader* shader): m_program(program), m_shader(shader)
{
}

std::shared_ptr<VRWebGLCommand_attachShader> VRWebGLCommand_attachShader::newInstance(VRWebGLProgram* program, VRWebGLShader* shader)
{
    return std::shared_ptr<VRWebGLCommand_attachShader>(new VRWebGLCommand_attachShader(program, shader));
}

bool VRWebGLCommand_attachShader::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_attachShader::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_attachShader::process() 
{
    if (!m_processed)
    {
        GLuint program = m_program->id();
        GLuint shader = m_shader->id();
        VRWebGL_glAttachShader(program, shader);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program << " shader = " << shader;
#endif
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_attachShader::name() const 
{
    return "attachShader";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_bindAttribLocation::VRWebGLCommand_bindAttribLocation(VRWebGLProgram* program, GLuint index, const std::string& name): m_program(program), m_index(index), m_name(name)
{
}

std::shared_ptr<VRWebGLCommand_bindAttribLocation> VRWebGLCommand_bindAttribLocation::newInstance(VRWebGLProgram* program, GLuint index, const std::string& name)
{
    return std::shared_ptr<VRWebGLCommand_bindAttribLocation>(new VRWebGLCommand_bindAttribLocation(program, index, name));
}

bool VRWebGLCommand_bindAttribLocation::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_bindAttribLocation::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_bindAttribLocation::process() 
{
    GLuint program = m_program->id();
    VRWebGL_glBindAttribLocation(program, m_index, m_name.c_str());
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program << " index = " << m_index << " name = " << m_name.c_str();
#endif  
    return 0;
}

std::string VRWebGLCommand_bindAttribLocation::name() const 
{
    return "bindAttribLocation";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_bindBuffer::VRWebGLCommand_bindBuffer(GLenum target, VRWebGLBuffer* buffer): m_target(target), m_buffer(buffer)
{
}
    
std::shared_ptr<VRWebGLCommand_bindBuffer> VRWebGLCommand_bindBuffer::newInstance(GLenum target, VRWebGLBuffer* buffer)
{
    return std::shared_ptr<VRWebGLCommand_bindBuffer>(new VRWebGLCommand_bindBuffer(target, buffer));
}

bool VRWebGLCommand_bindBuffer::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_bindBuffer::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_bindBuffer::process() 
{
    GLuint buffer = m_buffer != 0 ? m_buffer->id() : 0;
    VRWebGL_glBindBuffer(m_target, buffer);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " buffer = " << buffer;
#endif
    return 0;
}

std::string VRWebGLCommand_bindBuffer::name() const 
{
    return "bindBuffer";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_bindFramebuffer::VRWebGLCommand_bindFramebuffer(GLenum target, VRWebGLFramebuffer* framebuffer): m_target(target), m_framebuffer(framebuffer)
{
}
    
std::shared_ptr<VRWebGLCommand_bindFramebuffer> VRWebGLCommand_bindFramebuffer::newInstance(GLenum target, VRWebGLFramebuffer* framebuffer)
{
    return std::shared_ptr<VRWebGLCommand_bindFramebuffer>(new VRWebGLCommand_bindFramebuffer(target, framebuffer));
}

bool VRWebGLCommand_bindFramebuffer::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_bindFramebuffer::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_bindFramebuffer::process() 
{
    GLuint framebuffer = m_framebuffer != 0 ? m_framebuffer->id() : VRWebGLCommandProcessor::getInstance()->getFramebuffer();
    VRWebGL_glBindFramebuffer(m_target, framebuffer);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " framebuffer = " << framebuffer;
#endif
    return 0;
}

std::string VRWebGLCommand_bindFramebuffer::name() const 
{
    return "bindFramebuffer";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_bindRenderbuffer::VRWebGLCommand_bindRenderbuffer(GLenum target, VRWebGLRenderbuffer* renderbuffer): m_target(target), m_renderbuffer(renderbuffer)
{
}
    
std::shared_ptr<VRWebGLCommand_bindRenderbuffer> VRWebGLCommand_bindRenderbuffer::newInstance(GLenum target, VRWebGLRenderbuffer* renderbuffer)
{
    return std::shared_ptr<VRWebGLCommand_bindRenderbuffer>(new VRWebGLCommand_bindRenderbuffer(target, renderbuffer));
}

bool VRWebGLCommand_bindRenderbuffer::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_bindRenderbuffer::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_bindRenderbuffer::process() 
{
    GLuint renderbuffer = m_renderbuffer != 0 ? m_renderbuffer->id() : 0;
    VRWebGL_glBindRenderbuffer(m_target, renderbuffer);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " renderbuffer = " << renderbuffer;
#endif
    return 0;
}

std::string VRWebGLCommand_bindRenderbuffer::name() const 
{
    return "bindRenderbuffer";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_bindTexture::VRWebGLCommand_bindTexture(GLenum target, VRWebGLTexture* texture): m_target(target), m_texture(texture)
{
}

std::shared_ptr<VRWebGLCommand_bindTexture> VRWebGLCommand_bindTexture::newInstance(GLenum target, VRWebGLTexture* texture)
{
    return std::shared_ptr<VRWebGLCommand_bindTexture>(new VRWebGLCommand_bindTexture(target, texture));
}

bool VRWebGLCommand_bindTexture::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_bindTexture::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_bindTexture::process() 
{
    GLuint externalTextureId = m_texture != 0 ? m_texture->externalTextureId() : 0;
    GLuint textureId = m_texture != 0 ? externalTextureId != 0 ? externalTextureId : m_texture->id() : 0;
    GLenum target = externalTextureId != 0 ? GL_TEXTURE_EXTERNAL_OES : m_target;
    VRWebGL_glBindTexture(target, textureId);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = << " << target << " texture = " << texture;
#endif
    return 0;
}

std::string VRWebGLCommand_bindTexture::name() const 
{
    return "bindTexture";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_blendColor::VRWebGLCommand_blendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha): m_red(red), m_green(green), m_blue(blue), m_alpha(alpha)
{
}

std::shared_ptr<VRWebGLCommand_blendColor> VRWebGLCommand_blendColor::newInstance(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    return std::shared_ptr<VRWebGLCommand_blendColor>(new VRWebGLCommand_blendColor(red, green, blue, alpha));
}

bool VRWebGLCommand_blendColor::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_blendColor::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_blendColor::process() 
{
    VRWebGL_glBlendColor(m_red, m_green, m_blue, m_alpha);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " red = " << m_red << " green = " << m_green << " blue = " << m_blue << " alpha = " << m_alpha;
#endif
    return 0;
}

std::string VRWebGLCommand_blendColor::name() const 
{
    return "blendColor";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_blendEquation::VRWebGLCommand_blendEquation(GLenum mode): m_mode(mode)
{
}

std::shared_ptr<VRWebGLCommand_blendEquation> VRWebGLCommand_blendEquation::newInstance(GLenum mode)
{
    return std::shared_ptr<VRWebGLCommand_blendEquation>(new VRWebGLCommand_blendEquation(mode));
}

bool VRWebGLCommand_blendEquation::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_blendEquation::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_blendEquation::process() 
{
    VRWebGL_glBlendEquation(m_mode);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " mode = " << m_mode;
#endif    
    return 0;
}

std::string VRWebGLCommand_blendEquation::name() const 
{
    return "blendEquation";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_blendEquationSeparate::VRWebGLCommand_blendEquationSeparate(GLenum modeRGB, GLenum modeAlpha): m_modeRGB(modeRGB), m_modeAlpha(modeAlpha)
{
}

std::shared_ptr<VRWebGLCommand_blendEquationSeparate> VRWebGLCommand_blendEquationSeparate::newInstance(GLenum modeRGB, GLenum modeAlpha)
{
    return std::shared_ptr<VRWebGLCommand_blendEquationSeparate>(new VRWebGLCommand_blendEquationSeparate(modeRGB, modeAlpha));
}

bool VRWebGLCommand_blendEquationSeparate::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_blendEquationSeparate::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_blendEquationSeparate::process() 
{
    VRWebGL_glBlendEquationSeparate(m_modeRGB, m_modeAlpha);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " modeRGB = " << m_modeRGB << " modeAlpha = " << m_modeAlpha;
#endif    
    return 0;
}

std::string VRWebGLCommand_blendEquationSeparate::name() const 
{
    return "blendEquationSeparate";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_blendFunc::VRWebGLCommand_blendFunc(GLenum sfactor, GLenum dfactor): m_sfactor(sfactor), m_dfactor(dfactor)
{
}

std::shared_ptr<VRWebGLCommand_blendFunc> VRWebGLCommand_blendFunc::newInstance(GLenum sfactor, GLenum dfactor)
{
    return std::shared_ptr<VRWebGLCommand_blendFunc>(new VRWebGLCommand_blendFunc(sfactor, dfactor));
}

bool VRWebGLCommand_blendFunc::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_blendFunc::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_blendFunc::process() 
{
    VRWebGL_glBlendFunc(m_sfactor, m_dfactor);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " sfactor = " << m_sfactor <<" dfactor = " << m_dfactor;
#endif    
    return 0;
}

std::string VRWebGLCommand_blendFunc::name() const 
{
    return "blendFunc";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_blendFuncSeparate::VRWebGLCommand_blendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha): m_srcRGB(srcRGB), m_dstRGB(dstRGB), m_srcAlpha(srcAlpha), m_dstAlpha(dstAlpha)
{
}

std::shared_ptr<VRWebGLCommand_blendFuncSeparate> VRWebGLCommand_blendFuncSeparate::newInstance(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    return std::shared_ptr<VRWebGLCommand_blendFuncSeparate>(new VRWebGLCommand_blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha));
}

bool VRWebGLCommand_blendFuncSeparate::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_blendFuncSeparate::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_blendFuncSeparate::process() 
{
    VRWebGL_glBlendFuncSeparate(m_srcRGB, m_dstRGB, m_srcAlpha, m_dstAlpha);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " srcRGB = " << m_srcRGB << " dstRGB = " << m_dstRGB << " srcAlpha = " << m_srcAlpha << " dstAlpha = " << m_dstAlpha;
#endif    
    return 0;
}

std::string VRWebGLCommand_blendFuncSeparate::name() const 
{
    return "blendFuncSeparate";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_bufferData::VRWebGLCommand_bufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage): m_target(target), m_size(size), m_usage(usage)
{
    m_data = new GLbyte[size];
    memcpy(m_data, data, size);
}
    
std::shared_ptr<VRWebGLCommand_bufferData> VRWebGLCommand_bufferData::newInstance(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    return std::shared_ptr<VRWebGLCommand_bufferData>(new VRWebGLCommand_bufferData(target, size, data, usage));
}

VRWebGLCommand_bufferData::~VRWebGLCommand_bufferData()
{
    delete [] m_data;
    m_data = 0;
}

bool VRWebGLCommand_bufferData::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_bufferData::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_bufferData::process() 
{
    if (!m_processed)
    {
        VRWebGL_glBufferData(m_target, m_size, (GLvoid*)m_data, m_usage);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " size = " << m_size << " usage = " << m_usage;
#endif  
        m_processed = true;
    }  
    return 0;
}

std::string VRWebGLCommand_bufferData::name() const 
{
    return "bufferData";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_bufferSubData::VRWebGLCommand_bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data): m_target(target), m_offset(offset), m_size(size)
{
    m_data = new GLbyte[size];
    memcpy(m_data, data, size);
}
    
std::shared_ptr<VRWebGLCommand_bufferSubData> VRWebGLCommand_bufferSubData::newInstance(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data)
{
    return std::shared_ptr<VRWebGLCommand_bufferSubData>(new VRWebGLCommand_bufferSubData(target, offset, size, data));
}

VRWebGLCommand_bufferSubData::~VRWebGLCommand_bufferSubData()
{
    delete [] m_data;
    m_data = 0;
}

bool VRWebGLCommand_bufferSubData::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_bufferSubData::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_bufferSubData::process() 
{
    VRWebGL_glBufferSubData(m_target, m_offset, m_size, (GLvoid*)m_data);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " offset = " << m_offset  << " size = " << m_size;
#endif    
    return 0;
}

std::string VRWebGLCommand_bufferSubData::name() const 
{
    return "bufferSubData";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_checkFramebufferStatus::VRWebGLCommand_checkFramebufferStatus(GLenum target): m_target(target)
{
}

std::shared_ptr<VRWebGLCommand_checkFramebufferStatus> VRWebGLCommand_checkFramebufferStatus::newInstance(GLenum target)
{
    return std::shared_ptr<VRWebGLCommand_checkFramebufferStatus>(new VRWebGLCommand_checkFramebufferStatus(target));
}

bool VRWebGLCommand_checkFramebufferStatus::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_checkFramebufferStatus::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_checkFramebufferStatus::process() 
{
    m_status = VRWebGL_glCheckFramebufferStatus(m_target);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " status " << m_status;
#endif    
    return &m_status;
}

std::string VRWebGLCommand_checkFramebufferStatus::name() const 
{
    return "checkFramebufferStatus";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_clear::VRWebGLCommand_clear(GLbitfield mask): m_mask(mask)
{
}
    
std::shared_ptr<VRWebGLCommand_clear> VRWebGLCommand_clear::newInstance(GLbitfield mask)
{
    return std::shared_ptr<VRWebGLCommand_clear>(new VRWebGLCommand_clear(mask));
}

bool VRWebGLCommand_clear::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_clear::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_clear::process() 
{
    if (!m_processed)
    {
        VRWebGL_glClear(m_mask);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " mask = " << m_mask;
#endif    
        m_processed = true;
    }
    else 
    {
        m_processed = false;
    }
    return 0;
}

std::string VRWebGLCommand_clear::name() const 
{
    return "clear";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_clearColor::VRWebGLCommand_clearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha): m_red(red), m_green(green), m_blue(blue), m_alpha(alpha)
{
}

std::shared_ptr<VRWebGLCommand_clearColor> VRWebGLCommand_clearColor::newInstance(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    return std::shared_ptr<VRWebGLCommand_clearColor>(new VRWebGLCommand_clearColor(red, green, blue, alpha));
}

bool VRWebGLCommand_clearColor::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_clearColor::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_clearColor::process() 
{
    if (!m_processed)
    {
        VRWebGL_glClearColor(m_red, m_green, m_blue, m_alpha);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " red = " << m_red << " green = " << m_green << " blue = " << m_blue << " alpha = " << m_alpha;
#endif
        m_processed = true;    
    }
    return 0;
}

std::string VRWebGLCommand_clearColor::name() const 
{
    return "clearColor";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_clearDepthf::VRWebGLCommand_clearDepthf(GLclampf depth): m_depth(depth)
{
}

std::shared_ptr<VRWebGLCommand_clearDepthf> VRWebGLCommand_clearDepthf::newInstance(GLclampf depth)
{
    return std::shared_ptr<VRWebGLCommand_clearDepthf>(new VRWebGLCommand_clearDepthf(depth));
}

bool VRWebGLCommand_clearDepthf::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_clearDepthf::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_clearDepthf::process() 
{
    VRWebGL_glClearDepthf(m_depth);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " depth = " << m_depth;
#endif    
    return 0;
}

std::string VRWebGLCommand_clearDepthf::name() const 
{
    return "clearDepthf";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_clearStencil::VRWebGLCommand_clearStencil(GLint s): m_s(s)
{
}

std::shared_ptr<VRWebGLCommand_clearStencil> VRWebGLCommand_clearStencil::newInstance(GLint s)
{
    return std::shared_ptr<VRWebGLCommand_clearStencil>(new VRWebGLCommand_clearStencil(s));
}

bool VRWebGLCommand_clearStencil::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_clearStencil::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_clearStencil::process() 
{
    VRWebGL_glClearStencil(m_s);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " s = " << m_s;
#endif    
    return 0;
}

std::string VRWebGLCommand_clearStencil::name() const 
{
    return "clearStencil";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_colorMask::VRWebGLCommand_colorMask(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha): m_red(red), m_green(green), m_blue(blue), m_alpha(alpha)
{
}

std::shared_ptr<VRWebGLCommand_colorMask> VRWebGLCommand_colorMask::newInstance(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    return std::shared_ptr<VRWebGLCommand_colorMask>(new VRWebGLCommand_colorMask(red, green, blue, alpha));
}

bool VRWebGLCommand_colorMask::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_colorMask::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_colorMask::process() 
{
    VRWebGL_glColorMask(m_red, m_green, m_blue, m_alpha);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " red = " << m_red << " green = " << m_green << " blue = " << m_blue << " alpha = " << m_alpha;
#endif    
    return 0;
}

std::string VRWebGLCommand_colorMask::name() const 
{
    return "colorMask";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_compileShader::VRWebGLCommand_compileShader(VRWebGLShader* shader): m_shader(shader)
{
}

std::shared_ptr<VRWebGLCommand_compileShader> VRWebGLCommand_compileShader::newInstance(VRWebGLShader* shader)
{
    return std::shared_ptr<VRWebGLCommand_compileShader>(new VRWebGLCommand_compileShader(shader));
}

bool VRWebGLCommand_compileShader::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_compileShader::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_compileShader::process() 
{
    if (!m_processed)
    {
        GLuint shader = m_shader->id();
        VRWebGL_glCompileShader(shader);
#ifdef VRWEBGL_USE_CACHE
        GLint compileStatus;
        VRWebGL_glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
        m_shader->setCompileStatus(compileStatus);
        GLint infoLogLength;
        VRWebGL_glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0)
        {
            GLchar* infoLog = new GLchar[infoLogLength];
            VRWebGL_glGetShaderInfoLog(shader, infoLogLength, &infoLogLength, infoLog);
            m_shader->setInfoLog(infoLog);
            delete [] infoLog;
            infoLog = 0;
        }
        m_shader->cached = true;
#endif
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " shader = " << shader;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_compileShader::name() const 
{
    return "compileShader";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_createBuffer::VRWebGLCommand_createBuffer(VRWebGLBuffer* buffer): m_buffer(buffer)
{
}

std::shared_ptr<VRWebGLCommand_createBuffer> VRWebGLCommand_createBuffer::newInstance(VRWebGLBuffer* buffer)
{
    return std::shared_ptr<VRWebGLCommand_createBuffer>(new VRWebGLCommand_createBuffer(buffer));
}

bool VRWebGLCommand_createBuffer::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_createBuffer::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_createBuffer::process() 
{
    if (!m_processed)
    {
        GLuint buffer;
        VRWebGL_glGenBuffers(1, &buffer);
        m_buffer->setId(buffer);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " buffer = " << buffer;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_createBuffer::name() const 
{
    return "createBuffer";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_createFramebuffer::VRWebGLCommand_createFramebuffer(VRWebGLFramebuffer* framebuffer): m_framebuffer(framebuffer)
{
}

std::shared_ptr<VRWebGLCommand_createFramebuffer> VRWebGLCommand_createFramebuffer::newInstance(VRWebGLFramebuffer* framebuffer)
{
    return std::shared_ptr<VRWebGLCommand_createFramebuffer>(new VRWebGLCommand_createFramebuffer(framebuffer));
}

bool VRWebGLCommand_createFramebuffer::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_createFramebuffer::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_createFramebuffer::process() 
{
    if (!m_processed)
    {
        GLuint framebuffer;
        VRWebGL_glGenFramebuffers(1, &framebuffer);
        m_framebuffer->setId(framebuffer);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " framebuffer = " << framebuffer;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_createFramebuffer::name() const 
{
    return "createFramebuffer";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_createProgram::VRWebGLCommand_createProgram(VRWebGLProgram* program): m_program(program)
{
}

std::shared_ptr<VRWebGLCommand_createProgram> VRWebGLCommand_createProgram::newInstance(VRWebGLProgram* program)
{
    return std::shared_ptr<VRWebGLCommand_createProgram>(new VRWebGLCommand_createProgram(program));
}

bool VRWebGLCommand_createProgram::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_createProgram::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_createProgram::process() 
{
    if (!m_processed)
    {
        GLuint program = VRWebGL_glCreateProgram();
        m_program->setId(program);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_createProgram::name() const 
{
    return "createProgram";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_createRenderbuffer::VRWebGLCommand_createRenderbuffer(VRWebGLRenderbuffer* renderbuffer): m_renderbuffer(renderbuffer)
{
}

std::shared_ptr<VRWebGLCommand_createRenderbuffer> VRWebGLCommand_createRenderbuffer::newInstance(VRWebGLRenderbuffer* renderbuffer)
{
    return std::shared_ptr<VRWebGLCommand_createRenderbuffer>(new VRWebGLCommand_createRenderbuffer(renderbuffer));
}

bool VRWebGLCommand_createRenderbuffer::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_createRenderbuffer::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_createRenderbuffer::process() 
{
    if (!m_processed)
    {
        GLuint renderbuffer;
        VRWebGL_glGenFramebuffers(1, &renderbuffer);
        m_renderbuffer->setId(renderbuffer);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " renderbuffer = " << renderbuffer;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_createRenderbuffer::name() const 
{
    return "createRenderbuffer";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_createShader::VRWebGLCommand_createShader(VRWebGLShader* shader, GLenum type): m_shader(shader), m_type(type)
{
}

std::shared_ptr<VRWebGLCommand_createShader> VRWebGLCommand_createShader::newInstance(VRWebGLShader* shader, GLenum type)
{
    return std::shared_ptr<VRWebGLCommand_createShader>(new VRWebGLCommand_createShader(shader, type));
}

bool VRWebGLCommand_createShader::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_createShader::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_createShader::process() 
{
    if (!m_processed)
    {
        GLuint id = VRWebGL_glCreateShader(m_type);
        m_shader->setId(id);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " type = " << m_type << " shader = " << id;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_createShader::name() const 
{
    return "createShader";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_createTexture::VRWebGLCommand_createTexture(VRWebGLTexture* texture): m_texture(texture)
{
}

std::shared_ptr<VRWebGLCommand_createTexture> VRWebGLCommand_createTexture::newInstance(VRWebGLTexture* texture)
{
    return std::shared_ptr<VRWebGLCommand_createTexture>(new VRWebGLCommand_createTexture(texture));
}

bool VRWebGLCommand_createTexture::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_createTexture::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_createTexture::process() 
{
    if (!m_processed)
    {
        GLuint texture;
        VRWebGL_glGenTextures(1, &texture);
        m_texture->setId(texture);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " texture = " << texture;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_createTexture::name() const 
{
    return "createTexture";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_cullFace::VRWebGLCommand_cullFace(GLenum mode): m_mode(mode)
{
}

std::shared_ptr<VRWebGLCommand_cullFace> VRWebGLCommand_cullFace::newInstance(GLenum mode)
{
    return std::shared_ptr<VRWebGLCommand_cullFace>(new VRWebGLCommand_cullFace(mode));
}

bool VRWebGLCommand_cullFace::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_cullFace::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_cullFace::process() 
{
    VRWebGL_glCullFace(m_mode);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " mode = " << m_mode;
#endif    
    return 0;
}

std::string VRWebGLCommand_cullFace::name() const 
{
    return "cullFace";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_deleteBuffer::VRWebGLCommand_deleteBuffer(VRWebGLBuffer* buffer): m_buffer(buffer)
{
}

std::shared_ptr<VRWebGLCommand_deleteBuffer> VRWebGLCommand_deleteBuffer::newInstance(VRWebGLBuffer* buffer)
{
    return std::shared_ptr<VRWebGLCommand_deleteBuffer>(new VRWebGLCommand_deleteBuffer(buffer));
}

bool VRWebGLCommand_deleteBuffer::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_deleteBuffer::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_deleteBuffer::process() 
{
    if (!m_processed)
    {
        GLuint buffer = m_buffer->id();
        VRWebGL_glDeleteBuffers(1, &buffer);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " buffer = " << buffer;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_deleteBuffer::name() const 
{
    return "deleteBuffer";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_deleteFramebuffer::VRWebGLCommand_deleteFramebuffer(VRWebGLFramebuffer* framebuffer): m_framebuffer(framebuffer)
{
}

std::shared_ptr<VRWebGLCommand_deleteFramebuffer> VRWebGLCommand_deleteFramebuffer::newInstance(VRWebGLFramebuffer* framebuffer)
{
    return std::shared_ptr<VRWebGLCommand_deleteFramebuffer>(new VRWebGLCommand_deleteFramebuffer(framebuffer));
}

bool VRWebGLCommand_deleteFramebuffer::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_deleteFramebuffer::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_deleteFramebuffer::process() 
{
    if (!m_processed)
    {
        GLuint framebuffer = m_framebuffer->id();
        VRWebGL_glDeleteFramebuffers(1, &framebuffer);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " framebuffer = " << framebuffer;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_deleteFramebuffer::name() const 
{
    return "deleteFramebuffer";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_deleteProgram::VRWebGLCommand_deleteProgram(VRWebGLProgram* program): m_program(program)
{
}

std::shared_ptr<VRWebGLCommand_deleteProgram> VRWebGLCommand_deleteProgram::newInstance(VRWebGLProgram* program)
{
    return std::shared_ptr<VRWebGLCommand_deleteProgram>(new VRWebGLCommand_deleteProgram(program));
}

bool VRWebGLCommand_deleteProgram::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_deleteProgram::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_deleteProgram::process() 
{
    if (!m_processed)
    {
        GLuint program = m_program->id();
        VRWebGL_glDeleteProgram(program);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_deleteProgram::name() const 
{
    return "deleteProgram";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_deleteRenderbuffer::VRWebGLCommand_deleteRenderbuffer(VRWebGLRenderbuffer* renderbuffer): m_renderbuffer(renderbuffer)
{
}

std::shared_ptr<VRWebGLCommand_deleteRenderbuffer> VRWebGLCommand_deleteRenderbuffer::newInstance(VRWebGLRenderbuffer* renderbuffer)
{
    return std::shared_ptr<VRWebGLCommand_deleteRenderbuffer>(new VRWebGLCommand_deleteRenderbuffer(renderbuffer));
}

bool VRWebGLCommand_deleteRenderbuffer::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_deleteRenderbuffer::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_deleteRenderbuffer::process() 
{
    if (!m_processed)
    {
        GLuint renderbuffer = m_renderbuffer->id();
        VRWebGL_glDeleteRenderbuffers(1, &renderbuffer);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " renderbuffer = " << renderbuffer;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_deleteRenderbuffer::name() const 
{
    return "deleteRenderbuffer";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_deleteShader::VRWebGLCommand_deleteShader(VRWebGLShader* shader): m_shader(shader)
{
}

std::shared_ptr<VRWebGLCommand_deleteShader> VRWebGLCommand_deleteShader::newInstance(VRWebGLShader* shader)
{
    return std::shared_ptr<VRWebGLCommand_deleteShader>(new VRWebGLCommand_deleteShader(shader));
}

bool VRWebGLCommand_deleteShader::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_deleteShader::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_deleteShader::process() 
{
    if (!m_processed)
    {
        GLuint shader = m_shader->id();
        VRWebGL_glDeleteShader(shader);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " shader = " << shader;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_deleteShader::name() const 
{
    return "deleteShader";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_deleteTexture::VRWebGLCommand_deleteTexture(VRWebGLTexture* texture): m_texture(texture)
{
}

std::shared_ptr<VRWebGLCommand_deleteTexture> VRWebGLCommand_deleteTexture::newInstance(VRWebGLTexture* texture)
{
    return std::shared_ptr<VRWebGLCommand_deleteTexture>(new VRWebGLCommand_deleteTexture(texture));
}

bool VRWebGLCommand_deleteTexture::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_deleteTexture::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_deleteTexture::process() 
{
    if (!m_processed)
    {
        GLuint texture = m_texture->id();
        VRWebGL_glDeleteTextures(1, &texture);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " texture = " << texture;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_deleteTexture::name() const 
{
    return "deleteTexture";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_depthFunc::VRWebGLCommand_depthFunc(GLenum func): m_func(func)
{
}

std::shared_ptr<VRWebGLCommand_depthFunc> VRWebGLCommand_depthFunc::newInstance(GLenum func)
{
    return std::shared_ptr<VRWebGLCommand_depthFunc>(new VRWebGLCommand_depthFunc(func));
}

bool VRWebGLCommand_depthFunc::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_depthFunc::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_depthFunc::process() 
{
    VRWebGL_glDepthFunc(m_func);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " func = " << m_func;
#endif    
    return 0;
}

std::string VRWebGLCommand_depthFunc::name() const 
{
    return "depthFunc";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_depthMask::VRWebGLCommand_depthMask(GLboolean flag): m_flag(flag)
{
}

std::shared_ptr<VRWebGLCommand_depthMask> VRWebGLCommand_depthMask::newInstance(GLboolean flag)
{
    return std::shared_ptr<VRWebGLCommand_depthMask>(new VRWebGLCommand_depthMask(flag));
}

bool VRWebGLCommand_depthMask::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_depthMask::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_depthMask::process() 
{
    VRWebGL_glDepthMask(m_flag);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " flag = " << (m_flag ? "true" : "false");
#endif    
    return 0;
}

std::string VRWebGLCommand_depthMask::name() const 
{
    return "depthMask";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_depthRangef::VRWebGLCommand_depthRangef(GLclampf nearVal, GLclampf farVal): m_nearVal(nearVal), m_farVal(farVal)
{
}

std::shared_ptr<VRWebGLCommand_depthRangef> VRWebGLCommand_depthRangef::newInstance(GLclampf nearVal, GLclampf farVal)
{
    return std::shared_ptr<VRWebGLCommand_depthRangef>(new VRWebGLCommand_depthRangef(nearVal, farVal));
}

bool VRWebGLCommand_depthRangef::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_depthRangef::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_depthRangef::process() 
{
    VRWebGL_glDepthRangef(m_nearVal, m_farVal);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " near = " << m_nearVal << " far = " << m_farVal;
#endif    
    return 0;
}

std::string VRWebGLCommand_depthRangef::name() const 
{
    return "depthRangef";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_detachShader::VRWebGLCommand_detachShader(VRWebGLProgram* program, VRWebGLShader* shader): m_program(program), m_shader(shader)
{
}

std::shared_ptr<VRWebGLCommand_detachShader> VRWebGLCommand_detachShader::newInstance(VRWebGLProgram* program, VRWebGLShader* shader)
{
    return std::shared_ptr<VRWebGLCommand_detachShader>(new VRWebGLCommand_detachShader(program, shader));
}

bool VRWebGLCommand_detachShader::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_detachShader::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_detachShader::process() 
{
    GLuint program = m_program->id();
    GLuint shader = m_shader->id();
    VRWebGL_glDetachShader(program, shader);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program << " shader = " << shader;
#endif    
    return 0;
}

std::string VRWebGLCommand_detachShader::name() const 
{
    return "detachShader";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_disable::VRWebGLCommand_disable(GLenum cap): m_cap(cap)
{
}

std::shared_ptr<VRWebGLCommand_disable> VRWebGLCommand_disable::newInstance(GLenum cap)
{
    return std::shared_ptr<VRWebGLCommand_disable>(new VRWebGLCommand_disable(cap));
}

bool VRWebGLCommand_disable::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_disable::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_disable::process() 
{
    VRWebGL_glDisable(m_cap);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " cap = " << m_cap;
#endif    
    return 0;
}

std::string VRWebGLCommand_disable::name() const 
{
    return "disable";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_disableVertexAttribArray::VRWebGLCommand_disableVertexAttribArray(GLuint index): m_index(index)
{
}

std::shared_ptr<VRWebGLCommand_disableVertexAttribArray> VRWebGLCommand_disableVertexAttribArray::newInstance(GLuint index)
{
    return std::shared_ptr<VRWebGLCommand_disableVertexAttribArray>(new VRWebGLCommand_disableVertexAttribArray(index));
}

bool VRWebGLCommand_disableVertexAttribArray::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_disableVertexAttribArray::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_disableVertexAttribArray::process() 
{
    VRWebGL_glDisableVertexAttribArray(m_index);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " index = " << m_index;
#endif    
    return 0;
}

std::string VRWebGLCommand_disableVertexAttribArray::name() const 
{
    return "disableVertexAttribArray";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_drawArrays::VRWebGLCommand_drawArrays(GLenum mode, GLint first, GLsizei count): m_mode(mode), m_first(first), m_count(count)
{
}

std::shared_ptr<VRWebGLCommand_drawArrays> VRWebGLCommand_drawArrays::newInstance(GLenum mode, GLint first, GLsizei count)
{
    return std::shared_ptr<VRWebGLCommand_drawArrays>(new VRWebGLCommand_drawArrays(mode, first, count));
}

bool VRWebGLCommand_drawArrays::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_drawArrays::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_drawArrays::process() 
{
    VRWebGL_glDrawArrays(m_mode, m_first, m_count);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " mode = " << m_mode << " first = " << m_first << " count = " << m_count;
#endif    
    return 0;
}

std::string VRWebGLCommand_drawArrays::name() const 
{
    return "drawArrays";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_drawElements::VRWebGLCommand_drawElements(GLenum mode, GLsizei count, GLenum type, long long offset): m_mode(mode), m_count(count), m_type(type), m_offset(offset)
{
}

std::shared_ptr<VRWebGLCommand_drawElements> VRWebGLCommand_drawElements::newInstance(GLenum mode, GLsizei count, GLenum type, long long offset)
{
    return std::shared_ptr<VRWebGLCommand_drawElements>(new VRWebGLCommand_drawElements(mode, count, type, offset));
}

bool VRWebGLCommand_drawElements::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_drawElements::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_drawElements::process() 
{
    VRWebGL_glDrawElements(m_mode, m_count, m_type, reinterpret_cast<void*>(static_cast<intptr_t>(m_offset)));
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " mode = " << m_mode << " count = " << m_count << " type = " << m_type << " offsdet = " << m_offset;
#endif    
    return 0;
}

std::string VRWebGLCommand_drawElements::name() const 
{
    return "drawElements";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_enable::VRWebGLCommand_enable(GLenum cap): m_cap(cap)
{
}

std::shared_ptr<VRWebGLCommand_enable> VRWebGLCommand_enable::newInstance(GLenum cap)
{
    return std::shared_ptr<VRWebGLCommand_enable>(new VRWebGLCommand_enable(cap));
}

bool VRWebGLCommand_enable::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_enable::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_enable::process() 
{
    VRWebGL_glEnable(m_cap);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " cap = " << m_cap;
#endif    
    return 0;
}

std::string VRWebGLCommand_enable::name() const 
{
    return "enable";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_enableVertexAttribArray::VRWebGLCommand_enableVertexAttribArray(GLuint index): m_index(index)
{
}

std::shared_ptr<VRWebGLCommand_enableVertexAttribArray> VRWebGLCommand_enableVertexAttribArray::newInstance(GLuint index)
{
    return std::shared_ptr<VRWebGLCommand_enableVertexAttribArray>(new VRWebGLCommand_enableVertexAttribArray(index));
}

bool VRWebGLCommand_enableVertexAttribArray::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_enableVertexAttribArray::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_enableVertexAttribArray::process() 
{
    VRWebGL_glEnableVertexAttribArray(m_index);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " index = " << m_index;
#endif    
    return 0;
}

std::string VRWebGLCommand_enableVertexAttribArray::name() const 
{
    return "enableVertexAttribArray";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_framebufferRenderbuffer::VRWebGLCommand_framebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, VRWebGLRenderbuffer* renderbuffer): m_target(target), m_attachment(attachment), m_renderbuffertarget(renderbuffertarget), m_renderbuffer(renderbuffer)
{
}

std::shared_ptr<VRWebGLCommand_framebufferRenderbuffer> VRWebGLCommand_framebufferRenderbuffer::newInstance(GLenum target, GLenum attachment, GLenum renderbuffertarget, VRWebGLRenderbuffer* renderbuffer)
{
    return std::shared_ptr<VRWebGLCommand_framebufferRenderbuffer>(new VRWebGLCommand_framebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer));
}

bool VRWebGLCommand_framebufferRenderbuffer::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_framebufferRenderbuffer::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_framebufferRenderbuffer::process() 
{
    GLuint renderbuffer = m_renderbuffer != 0 ? m_renderbuffer->id() : 0;
    VRWebGL_glFramebufferRenderbuffer(m_target, m_attachment, m_renderbuffertarget, renderbuffer);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " attachment = " << m_attachment << " renderbuffertarget = " << m_renderbuffertarget << " renderbuffer = " << renderbuffer;
#endif    
    return 0;
}

std::string VRWebGLCommand_framebufferRenderbuffer::name() const 
{
    return "framebufferRenderbuffer";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_framebufferTexture2D::VRWebGLCommand_framebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, VRWebGLTexture* texture, GLint level): m_target(target), m_attachment(attachment), m_textarget(textarget), m_texture(texture), m_level(level)
{
}

std::shared_ptr<VRWebGLCommand_framebufferTexture2D> VRWebGLCommand_framebufferTexture2D::newInstance(GLenum target, GLenum attachment, GLenum textarget, VRWebGLTexture* texture, GLint level)
{
    return std::shared_ptr<VRWebGLCommand_framebufferTexture2D>(new VRWebGLCommand_framebufferTexture2D(target, attachment, textarget, texture, level));
}

bool VRWebGLCommand_framebufferTexture2D::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_framebufferTexture2D::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_framebufferTexture2D::process() 
{
    GLuint texture = m_texture != 0 ? m_texture->id() : 0;
    VRWebGL_glFramebufferTexture2D(m_target, m_attachment, m_textarget, texture, m_level);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " attachment = " << m_attachment << " textarget = " << m_textarget << " texture = " << texture << " level = " << m_level;
#endif    
    return 0;
}

std::string VRWebGLCommand_framebufferTexture2D::name() const 
{
    return "framebufferTexture2D";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_frontFace::VRWebGLCommand_frontFace(GLenum mode): m_mode(mode)
{
}

std::shared_ptr<VRWebGLCommand_frontFace> VRWebGLCommand_frontFace::newInstance(GLenum mode)
{
    return std::shared_ptr<VRWebGLCommand_frontFace>(new VRWebGLCommand_frontFace(mode));
}

bool VRWebGLCommand_frontFace::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_frontFace::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_frontFace::process() 
{
    VRWebGL_glFrontFace(m_mode);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " mode = " << m_mode;
#endif    
    return 0;
}

std::string VRWebGLCommand_frontFace::name() const 
{
    return "frontFace";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_generateMipmap::VRWebGLCommand_generateMipmap(GLenum target): m_target(target)
{
}

std::shared_ptr<VRWebGLCommand_generateMipmap> VRWebGLCommand_generateMipmap::newInstance(GLenum target)
{
    return std::shared_ptr<VRWebGLCommand_generateMipmap>(new VRWebGLCommand_generateMipmap(target));
}

bool VRWebGLCommand_generateMipmap::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_generateMipmap::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_generateMipmap::process() 
{
    if (!m_processed)
    {
        VRWebGL_glGenerateMipmap(m_target);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target;
#endif  
        m_processed = true;
    }  
    return 0;
}

std::string VRWebGLCommand_generateMipmap::name() const 
{
    return "generateMipmap";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_getActiveAttrib::VRWebGLCommand_getActiveAttrib(VRWebGLProgram* program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name): m_program(program), m_index(index), m_bufSize(bufSize), m_length(length), m_size(size), m_type(type), m_name(name)
{
}

std::shared_ptr<VRWebGLCommand_getActiveAttrib> VRWebGLCommand_getActiveAttrib::newInstance(VRWebGLProgram* program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    return std::shared_ptr<VRWebGLCommand_getActiveAttrib>(new VRWebGLCommand_getActiveAttrib(program, index, bufSize, length, size, type, name));
}

bool VRWebGLCommand_getActiveAttrib::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getActiveAttrib::canBeProcessedImmediately() const
{
#ifdef VRWEBGL_USE_CACHE
    return m_program->cached;
#else
    return false;
#endif
}

void* VRWebGLCommand_getActiveAttrib::process() 
{
#if !defined(VRWEBGL_USE_CACHE) || defined(VRWEBGL_SHOW_LOG)
    GLuint program = m_program->id();
#endif
#ifdef VRWEBGL_USE_CACHE
    std::shared_ptr<VRWebGLProgram::ActiveInfo> activeInfo = m_program->getActiveAttribute(m_index);
    if (activeInfo)
    {
        *m_length = activeInfo->name.size();
        strcpy(m_name, activeInfo->name.c_str());
        *m_type = activeInfo->type;
        *m_size = activeInfo->size; 
    }
#else
    VRWebGL_glGetActiveAttrib(program, m_index, m_bufSize, m_length, m_size, m_type, m_name);
#endif
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program << " index = " << m_index << " bufSize = " << m_bufSize << " length = " << *m_length << " size = " << *m_size << " type = " << *m_type << " name = " << m_name;
#endif
    return 0;
}

std::string VRWebGLCommand_getActiveAttrib::name() const 
{
    return "getActiveAttrib";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_getActiveUniform::VRWebGLCommand_getActiveUniform(VRWebGLProgram* program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name): m_program(program), m_index(index), m_bufSize(bufSize), m_length(length), m_size(size), m_type(type), m_name(name)
{
}

std::shared_ptr<VRWebGLCommand_getActiveUniform> VRWebGLCommand_getActiveUniform::newInstance(VRWebGLProgram* program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    return std::shared_ptr<VRWebGLCommand_getActiveUniform>(new VRWebGLCommand_getActiveUniform(program, index, bufSize, length, size, type, name));
}

bool VRWebGLCommand_getActiveUniform::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getActiveUniform::canBeProcessedImmediately() const
{
#ifdef VRWEBGL_USE_CACHE
    return m_program->cached;
#else
    return false;
#endif
}

void* VRWebGLCommand_getActiveUniform::process() 
{
#if !defined(VRWEBGL_USE_CACHE) || defined(VRWEBGL_SHOW_LOG)
    GLuint program = m_program->id();
#endif
#ifdef VRWEBGL_USE_CACHE
    std::shared_ptr<VRWebGLProgram::ActiveInfo> activeInfo = m_program->getActiveUniform(m_index);
    if (activeInfo)
    {
        *m_length = activeInfo->name.size();
        strcpy(m_name, activeInfo->name.c_str());
        *m_type = activeInfo->type;
        *m_size = activeInfo->size; 
    }
#else
    VRWebGL_glGetActiveUniform(program, m_index, m_bufSize, m_length, m_size, m_type, m_name);
#endif
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program << " index = " << m_index << " bufSize = " << m_bufSize << " length = " << *m_length << " size = " << *m_size << " type = " << *m_type << " name = " << m_name;
#endif    
    return 0;
}

std::string VRWebGLCommand_getActiveUniform::name() const 
{
    return "getActiveUniform";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_getAttribLocation::VRWebGLCommand_getAttribLocation(VRWebGLProgram* program, const std::string& name): m_program(program), m_name(name)
{
}

std::shared_ptr<VRWebGLCommand_getAttribLocation> VRWebGLCommand_getAttribLocation::newInstance(VRWebGLProgram* program, const std::string& name)
{
    return std::shared_ptr<VRWebGLCommand_getAttribLocation>(new VRWebGLCommand_getAttribLocation(program, name));
}

bool VRWebGLCommand_getAttribLocation::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getAttribLocation::canBeProcessedImmediately() const
{
#ifdef VRWEBGL_USE_CACHE
    return m_program->cached;
#else
    return false;
#endif
}

void* VRWebGLCommand_getAttribLocation::process() 
{
#if !defined(VRWEBGL_USE_CACHE) || defined(VRWEBGL_SHOW_LOG)
    GLuint program = m_program->id();
#endif
#ifdef VRWEBGL_USE_CACHE
    m_attribLocation = m_program->getAttributeLocation(m_name);
#else
    m_attribLocation = VRWebGL_glGetAttribLocation(program, m_name.c_str());
#endif
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program << " name = " << m_name.c_str() << " attribLocation = " << m_attribLocation;
#endif    
    return &m_attribLocation;
}

std::string VRWebGLCommand_getAttribLocation::name() const 
{
    return "getAttribLocation";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_getError::VRWebGLCommand_getError()
{
}

std::shared_ptr<VRWebGLCommand_getError> VRWebGLCommand_getError::newInstance()
{
    return std::shared_ptr<VRWebGLCommand_getError>(new VRWebGLCommand_getError());
}

bool VRWebGLCommand_getError::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getError::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_getError::process() 
{
    m_error = VRWebGL_glGetError();
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " error = " << m_error;
#endif    
    return &m_error;
}

std::string VRWebGLCommand_getError::name() const 
{
    return "getError";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_getBooleanv::VRWebGLCommand_getBooleanv(GLenum pname): m_pname(pname)
{
}

std::shared_ptr<VRWebGLCommand_getBooleanv> VRWebGLCommand_getBooleanv::newInstance(GLenum pname)
{
    return std::shared_ptr<VRWebGLCommand_getBooleanv>(new VRWebGLCommand_getBooleanv(pname));
}

bool VRWebGLCommand_getBooleanv::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getBooleanv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_getBooleanv::process() 
{
    VRWebGL_glGetBooleanv(m_pname, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " pname = " << m_pname;
#endif    
    return m_value;
}

std::string VRWebGLCommand_getBooleanv::name() const 
{
    return "getBooleanv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_getFloatv::VRWebGLCommand_getFloatv(GLenum pname): m_pname(pname)
{
}

std::shared_ptr<VRWebGLCommand_getFloatv> VRWebGLCommand_getFloatv::newInstance(GLenum pname)
{
    return std::shared_ptr<VRWebGLCommand_getFloatv>(new VRWebGLCommand_getFloatv(pname));
}

bool VRWebGLCommand_getFloatv::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getFloatv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_getFloatv::process() 
{
    VRWebGL_glGetFloatv(m_pname, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " pname = " << m_pname;
#endif    
    return m_value;
}

std::string VRWebGLCommand_getFloatv::name() const 
{
    return "getFloatv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_getIntegerv::VRWebGLCommand_getIntegerv(GLenum pname): m_pname(pname)
{
}

std::shared_ptr<VRWebGLCommand_getIntegerv> VRWebGLCommand_getIntegerv::newInstance(GLenum pname)
{
    return std::shared_ptr<VRWebGLCommand_getIntegerv>(new VRWebGLCommand_getIntegerv(pname));
}

bool VRWebGLCommand_getIntegerv::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getIntegerv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_getIntegerv::process() 
{
    VRWebGL_glGetIntegerv(m_pname, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " pname = " << m_pname;
#endif    
    return m_value;
}

std::string VRWebGLCommand_getIntegerv::name() const 
{
    return "getIntegerv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_getProgramiv::VRWebGLCommand_getProgramiv(VRWebGLProgram* program, GLenum pname): m_program(program), m_pname(pname)
{
}

std::shared_ptr<VRWebGLCommand_getProgramiv> VRWebGLCommand_getProgramiv::newInstance(VRWebGLProgram* program, GLenum pname)
{
    return std::shared_ptr<VRWebGLCommand_getProgramiv>(new VRWebGLCommand_getProgramiv(program, pname));
}

bool VRWebGLCommand_getProgramiv::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getProgramiv::canBeProcessedImmediately() const
{
#ifdef VRWEBGL_USE_CACHE
    return m_program->cached;
#else
    return false;
#endif
}

void* VRWebGLCommand_getProgramiv::process() 
{
#if !defined(VRWEBGL_USE_CACHE) || defined(VRWEBGL_SHOW_LOG)
    GLuint program = m_program->id();
#endif
#ifdef VRWEBGL_USE_CACHE
    switch(m_pname)
    {
        case GL_LINK_STATUS:
            m_programParameter = m_program->getLinkStatus();
            break; 
        case GL_ACTIVE_ATTRIBUTES:
            m_programParameter = m_program->getNumberOfActiveAttributes();
            break;
        case GL_ACTIVE_UNIFORMS:
            m_programParameter = m_program->getNumberOfActiveUniforms();
            break;
        case GL_DELETE_STATUS:
            m_programParameter = m_program->isDeleted();
            break;
        case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
            m_programParameter = m_program->getActiveAttributeMaxLength();
            break;
        case GL_ACTIVE_UNIFORM_MAX_LENGTH:
            m_programParameter = m_program->getActiveUniformMaxLength();
            break;
        default:
            VLOG(0) << "VRWebGL: getProgramiv requesting the pname '" << std::hex << m_pname << std::dec <<"' that is not currently supported in the cache mechanism.";
    } 
#else
    VRWebGL_glGetProgramiv(program, m_pname, &m_programParameter);
#endif
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program << " pname = " << m_pname << " programParameter = " << m_programParameter;
#endif    
    return &m_programParameter;
}

std::string VRWebGLCommand_getProgramiv::name() const 
{
    return "getProgramiv";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_getProgramInfoLog::VRWebGLCommand_getProgramInfoLog(VRWebGLProgram* program): m_program(program)
{
}

std::shared_ptr<VRWebGLCommand_getProgramInfoLog> VRWebGLCommand_getProgramInfoLog::newInstance(VRWebGLProgram* program)
{
    return std::shared_ptr<VRWebGLCommand_getProgramInfoLog>(new VRWebGLCommand_getProgramInfoLog(program));
}

bool VRWebGLCommand_getProgramInfoLog::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getProgramInfoLog::canBeProcessedImmediately() const
{
#ifdef VRWEBGL_USE_CACHE
    return m_program->cached;
#else
    return false;
#endif
}

void* VRWebGLCommand_getProgramInfoLog::process() 
{
#if !defined(VRWEBGL_USE_CACHE) || defined(VRWEBGL_SHOW_LOG)
    GLuint program = m_program->id();
#endif
#ifdef VRWEBGL_USE_CACHE
    m_programInfoLog = m_program->getInfoLog();
#else
    GLchar infoLog[1000];
    GLsizei length;
    VRWebGL_glGetProgramInfoLog(program, sizeof(infoLog) / sizeof(GLchar), &length, infoLog);
    m_programInfoLog = infoLog;
#endif
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program << " infoLog = " << m_programInfoLog.c_str();
#endif    
    return (void*)(m_programInfoLog.c_str());
}

std::string VRWebGLCommand_getProgramInfoLog::name() const 
{
    return "getProgramInfoLog";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_getShaderiv::VRWebGLCommand_getShaderiv(VRWebGLShader* shader, GLenum pname): m_shader(shader), m_pname(pname), m_shaderParameter(0)
{
}

std::shared_ptr<VRWebGLCommand_getShaderiv> VRWebGLCommand_getShaderiv::newInstance(VRWebGLShader* shader, GLenum pname)
{
    return std::shared_ptr<VRWebGLCommand_getShaderiv>(new VRWebGLCommand_getShaderiv(shader, pname));
}

bool VRWebGLCommand_getShaderiv::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getShaderiv::canBeProcessedImmediately() const
{
#ifdef VRWEBGL_USE_CACHE
    return m_shader->cached;
#else
    return false;
#endif
}

void* VRWebGLCommand_getShaderiv::process() 
{
#if !defined(VRWEBGL_USE_CACHE) || defined(VRWEBGL_SHOW_LOG)
    GLuint shader = m_shader->id();
#endif
#ifdef VRWEBGL_USE_CACHE
    switch(m_pname)
    {
        case GL_COMPILE_STATUS:
            m_shaderParameter = m_shader->getCompileStatus();
            break; 
        case GL_SHADER_TYPE:
            m_shaderParameter = m_shader->getType();
            break;
        case GL_DELETE_STATUS:
            m_shaderParameter = m_shader->isDeleted();
            break;
    } 
#else
    VRWebGL_glGetShaderiv(shader, m_pname, &m_shaderParameter);
#endif
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " shader = " << shader << " pname = " << m_pname << " result = " << m_shaderParameter;
#endif    
    return &m_shaderParameter;
}

std::string VRWebGLCommand_getShaderiv::name() const 
{
    return "getShaderiv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_getShaderInfoLog::VRWebGLCommand_getShaderInfoLog(VRWebGLShader* shader): m_shader(shader)
{
}

std::shared_ptr<VRWebGLCommand_getShaderInfoLog> VRWebGLCommand_getShaderInfoLog::newInstance(VRWebGLShader* shader)
{
    return std::shared_ptr<VRWebGLCommand_getShaderInfoLog>(new VRWebGLCommand_getShaderInfoLog(shader));
}

bool VRWebGLCommand_getShaderInfoLog::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getShaderInfoLog::canBeProcessedImmediately() const
{
#ifdef VRWEBGL_USE_CACHE
    return m_shader->cached;
#else
    return false;
#endif
}

void* VRWebGLCommand_getShaderInfoLog::process() 
{
#if !defined(VRWEBGL_USE_CACHE) || defined(VRWEBGL_SHOW_LOG)
    GLuint shader = m_shader->id();
#endif
#ifdef VRWEBGL_USE_CACHE
    m_shaderInfoLog = m_shader->getInfoLog();
#else
    GLchar infoLog[1000];
    GLsizei length;
    VRWebGL_glGetShaderInfoLog(shader, sizeof(infoLog) / sizeof(GLchar), &length, infoLog);
    m_shaderInfoLog = infoLog;
#endif
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " shader = " << shader << " infoLog = " << m_shaderInfoLog.c_str();
#endif    
    return (void*)(m_shaderInfoLog.c_str());
}

std::string VRWebGLCommand_getShaderInfoLog::name() const 
{
    return "getShaderInfoLog";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_getShaderPrecisionFormat::VRWebGLCommand_getShaderPrecisionFormat(GLenum shaderType, GLenum precisionType): m_shaderType(shaderType), m_precisionType(precisionType)
{
}

std::shared_ptr<VRWebGLCommand_getShaderPrecisionFormat> VRWebGLCommand_getShaderPrecisionFormat::newInstance(GLenum shaderType, GLenum precisionType)
{
    return std::shared_ptr<VRWebGLCommand_getShaderPrecisionFormat>(new VRWebGLCommand_getShaderPrecisionFormat(shaderType, precisionType));
}

bool VRWebGLCommand_getShaderPrecisionFormat::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getShaderPrecisionFormat::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_getShaderPrecisionFormat::process() 
{
    VRWebGL_glGetShaderPrecisionFormat(m_shaderType, m_precisionType, m_rangeAndPrecision, &m_rangeAndPrecision[2]);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " shaderType = " << m_shaderType << " precisionType = " << m_precisionType;
#endif    
    return (void*)(m_rangeAndPrecision);
}

std::string VRWebGLCommand_getShaderPrecisionFormat::name() const 
{
    return "getShaderPrecisionFormat";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_getUniformLocation::VRWebGLCommand_getUniformLocation(VRWebGLUniformLocation* uniformLocation, VRWebGLProgram* program, const std::string& name): m_uniformLocation(uniformLocation), m_program(program), m_name(name)
{
}

std::shared_ptr<VRWebGLCommand_getUniformLocation> VRWebGLCommand_getUniformLocation::newInstance(VRWebGLUniformLocation* uniformLocation, VRWebGLProgram* program, const std::string& name)
{
    return std::shared_ptr<VRWebGLCommand_getUniformLocation>(new VRWebGLCommand_getUniformLocation(uniformLocation, program, name));
}

bool VRWebGLCommand_getUniformLocation::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_getUniformLocation::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_getUniformLocation::process() 
{
    GLuint program = m_program->id();
    GLint uniformLocation = VRWebGL_glGetUniformLocation(program, m_name.c_str());
    m_uniformLocation->setLocation(uniformLocation);
    VRWebGLCommandProcessor::getInstance()->setMatrixUniformLocationForName(program, uniformLocation, m_name);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program << " name = " << m_name.c_str() << " uniformLocation = " << uniformLocation;
#endif    
    return 0;
}

std::string VRWebGLCommand_getUniformLocation::name() const 
{
    return "getUniformLocation";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_lineWidth::VRWebGLCommand_lineWidth(GLfloat width): m_width(width)
{
}

std::shared_ptr<VRWebGLCommand_lineWidth> VRWebGLCommand_lineWidth::newInstance(GLfloat width)
{
    return std::shared_ptr<VRWebGLCommand_lineWidth>(new VRWebGLCommand_lineWidth(width));
}

bool VRWebGLCommand_lineWidth::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_lineWidth::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_lineWidth::process() 
{
    VRWebGL_glLineWidth(m_width);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " width = " << m_width;
#endif    
    return 0;
}

std::string VRWebGLCommand_lineWidth::name() const 
{
    return "lineWidth";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_linkProgram::VRWebGLCommand_linkProgram(VRWebGLProgram* program): m_program(program)
{
}

std::shared_ptr<VRWebGLCommand_linkProgram> VRWebGLCommand_linkProgram::newInstance(VRWebGLProgram* program)
{
    return std::shared_ptr<VRWebGLCommand_linkProgram>(new VRWebGLCommand_linkProgram(program));
}

bool VRWebGLCommand_linkProgram::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_linkProgram::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_linkProgram::process() 
{
    if (!m_processed)
    {
        GLuint program = m_program->id();
        VRWebGL_glLinkProgram(program);
#ifdef VRWEBGL_USE_CACHE   
        // Cache the link status
        GLint linkStatus;
        VRWebGL_glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        m_program->setLinkStatus(linkStatus);

        // Cache the info log
        GLint infoLogLength;
        VRWebGL_glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0)
        {
            GLchar* infoLog = new GLchar[infoLogLength];
            VRWebGL_glGetProgramInfoLog(program, infoLogLength, &infoLogLength, infoLog);
            m_program->setInfoLog(infoLog);
            delete [] infoLog;
            infoLog = 0;
        }        
        // Cache the active uniforms
        GLint numberOfActiveUniforms;
        VRWebGL_glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numberOfActiveUniforms);

        GLint activeUniformMaxLength;
        VRWebGL_glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &activeUniformMaxLength);
        m_program->setActiveUniformMaxLength(activeUniformMaxLength);

        if (numberOfActiveUniforms > 0 && activeUniformMaxLength > 0)
        {
            GLchar* name = new GLchar[activeUniformMaxLength];
            GLenum type;
            GLsizei length;
            GLint size;
            for (GLint i = 0; i < numberOfActiveUniforms; i++)
            {
                VRWebGL_glGetActiveUniform(program, i, activeUniformMaxLength, &length, &size, &type, name);
                if (size > 0)
                {
                    m_program->addActiveUniform(std::string(name, length), type, size);

                    std::shared_ptr<VRWebGLProgram::ActiveInfo> ai = m_program->getActiveUniform(i);
                }
            }
            delete [] name;
            name = 0;
        }
        // Cache the active attributes
        GLint numberOfActiveAttributes;
        VRWebGL_glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numberOfActiveAttributes);

        GLint activeAttributeMaxLength;
        VRWebGL_glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &activeAttributeMaxLength);
        m_program->setActiveAttributeMaxLength(activeAttributeMaxLength);

        if (numberOfActiveAttributes > 0 && activeAttributeMaxLength > 0)
        {
            GLchar* name = new GLchar[activeAttributeMaxLength];
            GLenum type;
            GLsizei length;
            GLint size;
            GLint attributeLocation;
            for (GLint i = 0; i < numberOfActiveAttributes; i++)
            {
                VRWebGL_glGetActiveAttrib(program, i, activeAttributeMaxLength, &length, &size, &type, name);
                if (size > 0)
                {
                    std::string realName = std::string(name, length);
                    m_program->addActiveAttribute(realName, type, size);
                    // Also cache the attribute location
                    attributeLocation = VRWebGL_glGetAttribLocation(program, realName.c_str());
                    m_program->addAttributeLocation(realName, attributeLocation);

                    std::shared_ptr<VRWebGLProgram::ActiveInfo> ai = m_program->getActiveAttribute(i);
                }
            }
            delete [] name;
            name = 0;
        }
        m_program->cached = true;
#endif
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_linkProgram::name() const 
{
    return "linkProgram";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_pixelStorei::VRWebGLCommand_pixelStorei(GLenum pname, GLint param): m_pname(pname), m_param(param)
{
}

std::shared_ptr<VRWebGLCommand_pixelStorei> VRWebGLCommand_pixelStorei::newInstance(GLenum pname, GLint param)
{
    return std::shared_ptr<VRWebGLCommand_pixelStorei>(new VRWebGLCommand_pixelStorei(pname, param));
}

bool VRWebGLCommand_pixelStorei::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_pixelStorei::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_pixelStorei::process() 
{
    VRWebGL_glPixelStorei(m_pname, m_param);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " pname = " << m_pname << " param = " << m_param;
#endif    
    return 0;
}

std::string VRWebGLCommand_pixelStorei::name() const 
{
    return "pixelStorei";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_readPixels::VRWebGLCommand_readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * data): m_x(x), m_y(y), m_width(width), m_height(height), m_format(format), m_type(type), m_data(data)
{
}

std::shared_ptr<VRWebGLCommand_readPixels> VRWebGLCommand_readPixels::newInstance(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * data)
{
    return std::shared_ptr<VRWebGLCommand_readPixels>(new VRWebGLCommand_readPixels(x, y, width, height, format, type, data));
}

bool VRWebGLCommand_readPixels::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_readPixels::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_readPixels::process() 
{
    VRWebGL_glReadPixels(m_x, m_y, m_width, m_height, m_format, m_type, m_data);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " x = " << m_x << " y = " << m_y << " width = " << m_width << " height = " << " format = " << m_format << " type = " << m_type;
#endif    
    return 0;
}

std::string VRWebGLCommand_readPixels::name() const 
{
    return "readPixels";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_renderbufferStorage::VRWebGLCommand_renderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height): m_target(target), m_internalformat(internalformat), m_width(width), m_height(height)
{
}

std::shared_ptr<VRWebGLCommand_renderbufferStorage> VRWebGLCommand_renderbufferStorage::newInstance(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    return std::shared_ptr<VRWebGLCommand_renderbufferStorage>(new VRWebGLCommand_renderbufferStorage(target, internalformat, width, height));
}

bool VRWebGLCommand_renderbufferStorage::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_renderbufferStorage::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_renderbufferStorage::process() 
{
    VRWebGL_glRenderbufferStorage(m_target, m_internalformat, m_width, m_height);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " internalformat = " << m_internalformat << " width = " << m_width << " height = " << m_height;
#endif    
    return 0;
}

std::string VRWebGLCommand_renderbufferStorage::name() const 
{
    return "renderbufferStorage";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_scissor::VRWebGLCommand_scissor(GLint x, GLint y, GLsizei width, GLsizei height): m_x(x), m_y(y), m_width(width), m_height(height)
{
}

std::shared_ptr<VRWebGLCommand_scissor> VRWebGLCommand_scissor::newInstance(GLint x, GLint y, GLsizei width, GLsizei height)
{
    return std::shared_ptr<VRWebGLCommand_scissor>(new VRWebGLCommand_scissor(x, y, width, height));
}

bool VRWebGLCommand_scissor::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_scissor::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_scissor::process() 
{
    VRWebGL_glScissor(m_x, m_y, m_width, m_height);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " x = " << m_x << " y = " << m_y << " width = " << m_width << " height = " << m_height;
#endif    
    return 0;
}

std::string VRWebGLCommand_scissor::name() const 
{
    return "scissor";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_shaderSource::VRWebGLCommand_shaderSource(VRWebGLShader* shader, const GLchar* source, GLint length): m_shader(shader), m_length(length)
{
    m_source = new GLchar[length];
    memcpy(m_source, source, length);
}

std::shared_ptr<VRWebGLCommand_shaderSource> VRWebGLCommand_shaderSource::newInstance(VRWebGLShader* shader, const GLchar* source, GLint length)
{
    return std::shared_ptr<VRWebGLCommand_shaderSource>(new VRWebGLCommand_shaderSource(shader, source, length));
}

VRWebGLCommand_shaderSource::~VRWebGLCommand_shaderSource()
{
    delete [] m_source;
    m_source = 0;
}

bool VRWebGLCommand_shaderSource::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_shaderSource::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_shaderSource::process() 
{
    if (!m_processed)
    {
        GLuint shader = m_shader->id();
        VRWebGL_glShaderSource(shader, 1, (const GLchar**)&m_source, &m_length);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " shader = " << shader << " length = " << m_length;
#endif    
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_shaderSource::name() const 
{
    return "shaderSource";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_texParameteri::VRWebGLCommand_texParameteri(GLenum target, GLenum pname, GLint param, VRWebGLTexture* texture): m_target(target), m_pname(pname), m_param(param), m_texture(texture)
{
}

std::shared_ptr<VRWebGLCommand_texParameteri> VRWebGLCommand_texParameteri::newInstance(GLenum target, GLenum pname, GLint param, VRWebGLTexture* texture)
{
    return std::shared_ptr<VRWebGLCommand_texParameteri>(new VRWebGLCommand_texParameteri(target, pname, param, texture));
}

bool VRWebGLCommand_texParameteri::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_texParameteri::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_texParameteri::process() 
{
    // A hack to fix the assignment of the correct filter to the external textures.
    if (m_texture && m_texture->externalTextureId() != 0 && (m_pname == GL_TEXTURE_MIN_FILTER || m_pname == GL_TEXTURE_MAG_FILTER))
    {
        m_target = GL_TEXTURE_EXTERNAL_OES;
    }
    
    VRWebGL_glTexParameteri(m_target, m_pname, m_param);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " pname = " << m_pname << " param = " << m_param;
#endif    
    return 0;
}

std::string VRWebGLCommand_texParameteri::name() const 
{
    return "texParameteri";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_texParameterf::VRWebGLCommand_texParameterf(GLenum target, GLenum pname, GLfloat param): m_target(target), m_pname(pname), m_param(param)
{
}

std::shared_ptr<VRWebGLCommand_texParameterf> VRWebGLCommand_texParameterf::newInstance(GLenum target, GLenum pname, GLfloat param)
{
    return std::shared_ptr<VRWebGLCommand_texParameterf>(new VRWebGLCommand_texParameterf(target, pname, param));
}

bool VRWebGLCommand_texParameterf::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_texParameterf::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_texParameterf::process() 
{
    VRWebGL_glTexParameterf(m_target, m_pname, m_param);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " pname = " << m_pname << " param = " << m_param;
#endif    
    return 0;
}

std::string VRWebGLCommand_texParameterf::name() const 
{
    return "texParameterf";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_texImage2D::VRWebGLCommand_texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data): m_target(target), m_level(level), m_internalformat(internalformat), m_width(width), m_height(height), m_border(border), m_format(format), m_type(type)
{
    if (data != 0) 
    {
        GLsizei bytesPerPixel = 4;
        switch(internalformat)
        {
            case GL_ALPHA:
            case GL_LUMINANCE:
                bytesPerPixel = 1;
                break;
            case GL_LUMINANCE_ALPHA:
                bytesPerPixel = 2;
                break;
            case GL_RGB:
                bytesPerPixel = 3;
                break;
        }
        GLsizei size = width * height * bytesPerPixel;
        m_data = new GLbyte[size];
        memcpy(m_data, data, size);
    }
    else {
        m_data = 0;
    }
}

std::shared_ptr<VRWebGLCommand_texImage2D> VRWebGLCommand_texImage2D::newInstance(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data)
{
    return std::shared_ptr<VRWebGLCommand_texImage2D>(new VRWebGLCommand_texImage2D(target, level, internalformat, width, height, border, format, type, data));
}

VRWebGLCommand_texImage2D::~VRWebGLCommand_texImage2D()
{
    delete [] m_data;
    m_data = 0;
}

bool VRWebGLCommand_texImage2D::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_texImage2D::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_texImage2D::process() 
{
    if (!m_processed)
    {
        VRWebGL_glTexImage2D(m_target, m_level, m_internalformat, m_width, m_height, m_border, m_format, m_type, m_data);
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " target = " << m_target << " level = " << m_level << " internalformat = " << m_internalformat << " width = " << m_width << " height = " << m_height << " border = " << m_border << " format = " << m_format << " type = " << m_type;
#endif  
        m_processed = true;
    }  
    return 0;
}

std::string VRWebGLCommand_texImage2D::name() const 
{
    return "texImage2D";
}
    
// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform1i::VRWebGLCommand_uniform1i(const VRWebGLUniformLocation* location, GLint x): m_location(location), m_x(x)
{
}

std::shared_ptr<VRWebGLCommand_uniform1i> VRWebGLCommand_uniform1i::newInstance(const VRWebGLUniformLocation* location, GLint x)
{
    return std::shared_ptr<VRWebGLCommand_uniform1i>(new VRWebGLCommand_uniform1i(location, x));
}

bool VRWebGLCommand_uniform1i::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform1i::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform1i::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform1i(location, m_x);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " x = " << m_x;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform1i::name() const 
{
    return "uniform1i";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform1iv::VRWebGLCommand_uniform1iv(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value): m_location(location), m_count(count)
{
    m_value = new GLint[count];
    memcpy(m_value, value, count * sizeof(GLint));
}

std::shared_ptr<VRWebGLCommand_uniform1iv> VRWebGLCommand_uniform1iv::newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value)
{
    return std::shared_ptr<VRWebGLCommand_uniform1iv>(new VRWebGLCommand_uniform1iv(location, count, value));
}

VRWebGLCommand_uniform1iv::~VRWebGLCommand_uniform1iv()
{
    delete [] m_value;
    m_value = 0;
}

bool VRWebGLCommand_uniform1iv::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform1iv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform1iv::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform1iv(location, m_count, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " count = " << m_count;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform1iv::name() const 
{
    return "uniform1iv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform1f::VRWebGLCommand_uniform1f(const VRWebGLUniformLocation* location, GLfloat x): m_location(location), m_x(x)
{
}

std::shared_ptr<VRWebGLCommand_uniform1f> VRWebGLCommand_uniform1f::newInstance(const VRWebGLUniformLocation* location, GLfloat x)
{
    return std::shared_ptr<VRWebGLCommand_uniform1f>(new VRWebGLCommand_uniform1f(location, x));
}

bool VRWebGLCommand_uniform1f::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform1f::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform1f::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform1f(location, m_x);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " x = " << m_x;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform1f::name() const 
{
    return "uniform1f";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform1fv::VRWebGLCommand_uniform1fv(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value): m_location(location), m_count(count)
{
    m_value = new GLfloat[count];
    memcpy(m_value, value, count * sizeof(GLfloat));
}

std::shared_ptr<VRWebGLCommand_uniform1fv> VRWebGLCommand_uniform1fv::newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value)
{
    return std::shared_ptr<VRWebGLCommand_uniform1fv>(new VRWebGLCommand_uniform1fv(location, count, value));
}

VRWebGLCommand_uniform1fv::~VRWebGLCommand_uniform1fv()
{
    delete [] m_value;
    m_value = 0;
}

bool VRWebGLCommand_uniform1fv::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform1fv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform1fv::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform1fv(location, m_count, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " count = " << m_count;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform1fv::name() const 
{
    return "uniform1fv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform2f::VRWebGLCommand_uniform2f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y): m_location(location), m_x(x), m_y(y)
{
}

std::shared_ptr<VRWebGLCommand_uniform2f> VRWebGLCommand_uniform2f::newInstance(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y)
{
    return std::shared_ptr<VRWebGLCommand_uniform2f>(new VRWebGLCommand_uniform2f(location, x, y));
}

bool VRWebGLCommand_uniform2f::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform2f::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform2f::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform2f(location, m_x, m_y);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " x = " << m_x << " y = " << m_y;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform2f::name() const 
{
    return "uniform2f";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform2fv::VRWebGLCommand_uniform2fv(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value): m_location(location), m_count(count)
{
    m_value = new GLfloat[count * 2];
    memcpy(m_value, value, count * 2 * sizeof(GLfloat));
}

std::shared_ptr<VRWebGLCommand_uniform2fv> VRWebGLCommand_uniform2fv::newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value)
{
    return std::shared_ptr<VRWebGLCommand_uniform2fv>(new VRWebGLCommand_uniform2fv(location, count, value));
}

VRWebGLCommand_uniform2fv::~VRWebGLCommand_uniform2fv()
{
    delete [] m_value;
    m_value = 0;
}

bool VRWebGLCommand_uniform2fv::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform2fv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform2fv::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform2fv(location, m_count, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " count = " << m_count;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform2fv::name() const 
{
    return "uniform2fv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform2i::VRWebGLCommand_uniform2i(const VRWebGLUniformLocation* location, GLint x, GLint y): m_location(location), m_x(x), m_y(y)
{
}

std::shared_ptr<VRWebGLCommand_uniform2i> VRWebGLCommand_uniform2i::newInstance(const VRWebGLUniformLocation* location, GLint x, GLint y)
{
    return std::shared_ptr<VRWebGLCommand_uniform2i>(new VRWebGLCommand_uniform2i(location, x, y));
}

bool VRWebGLCommand_uniform2i::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform2i::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform2i::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform2i(location, m_x, m_y);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " x = " << m_x << " y = " << m_y;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform2i::name() const 
{
    return "uniform2i";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform2iv::VRWebGLCommand_uniform2iv(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value): m_location(location), m_count(count)
{
    m_value = new GLint[count * 2];
    memcpy(m_value, value, count * 2 * sizeof(GLint));
}

std::shared_ptr<VRWebGLCommand_uniform2iv> VRWebGLCommand_uniform2iv::newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value)
{
    return std::shared_ptr<VRWebGLCommand_uniform2iv>(new VRWebGLCommand_uniform2iv(location, count, value));
}

VRWebGLCommand_uniform2iv::~VRWebGLCommand_uniform2iv()
{
    delete [] m_value;
    m_value = 0;
}

bool VRWebGLCommand_uniform2iv::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform2iv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform2iv::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform2iv(location, m_count, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " count = " << m_count;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform2iv::name() const 
{
    return "uniform2iv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform3f::VRWebGLCommand_uniform3f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z): m_location(location), m_x(x), m_y(y), m_z(z)
{
}

std::shared_ptr<VRWebGLCommand_uniform3f> VRWebGLCommand_uniform3f::newInstance(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z)
{
    return std::shared_ptr<VRWebGLCommand_uniform3f>(new VRWebGLCommand_uniform3f(location, x, y, z));
}

bool VRWebGLCommand_uniform3f::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform3f::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform3f::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform3f(location, m_x, m_y, m_z);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " x = " << m_x << " y = " << m_y << " z = " << m_z;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform3f::name() const 
{
    return "uniform3f";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform3fv::VRWebGLCommand_uniform3fv(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value): m_location(location), m_count(count)
{
    m_value = new GLfloat[count * 3];
    memcpy(m_value, value, count * 3 * sizeof(GLfloat));
}

std::shared_ptr<VRWebGLCommand_uniform3fv> VRWebGLCommand_uniform3fv::newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value)
{
    return std::shared_ptr<VRWebGLCommand_uniform3fv>(new VRWebGLCommand_uniform3fv(location, count, value));
}

VRWebGLCommand_uniform3fv::~VRWebGLCommand_uniform3fv()
{
    delete [] m_value;
    m_value = 0;
}

bool VRWebGLCommand_uniform3fv::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform3fv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform3fv::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform3fv(location, m_count, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " count = " << m_count;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform3fv::name() const 
{
    return "uniform3fv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform3i::VRWebGLCommand_uniform3i(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z): m_location(location), m_x(x), m_y(y), m_z(z)
{
}

std::shared_ptr<VRWebGLCommand_uniform3i> VRWebGLCommand_uniform3i::newInstance(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z)
{
    return std::shared_ptr<VRWebGLCommand_uniform3i>(new VRWebGLCommand_uniform3i(location, x, y, z));
}

bool VRWebGLCommand_uniform3i::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform3i::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform3i::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform3i(location, m_x, m_y, m_z);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " x = " << m_x << " y = " << m_y << " z = " << m_z;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform3i::name() const 
{
    return "uniform3i";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform3iv::VRWebGLCommand_uniform3iv(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value): m_location(location), m_count(count)
{
    m_value = new GLint[count * 3];
    memcpy(m_value, value, count * 3 * sizeof(GLint));
}

std::shared_ptr<VRWebGLCommand_uniform3iv> VRWebGLCommand_uniform3iv::newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value)
{
    return std::shared_ptr<VRWebGLCommand_uniform3iv>(new VRWebGLCommand_uniform3iv(location, count, value));
}

VRWebGLCommand_uniform3iv::~VRWebGLCommand_uniform3iv()
{
    delete [] m_value;
    m_value = 0;
}

bool VRWebGLCommand_uniform3iv::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform3iv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform3iv::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform3iv(location, m_count, m_value);
#ifdef VRWEBGL_SHOW_LOG    
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " count = " << m_count;
#endif
    return 0;
}
    
std::string VRWebGLCommand_uniform3iv::name() const 
{
    return "uniform3iv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform4f::VRWebGLCommand_uniform4f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z, GLfloat w): m_location(location), m_x(x), m_y(y), m_z(z), m_w(w)
{
}

std::shared_ptr<VRWebGLCommand_uniform4f> VRWebGLCommand_uniform4f::newInstance(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    return std::shared_ptr<VRWebGLCommand_uniform4f>(new VRWebGLCommand_uniform4f(location, x, y, z, w));
}

bool VRWebGLCommand_uniform4f::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform4f::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform4f::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform4f(location, m_x, m_y, m_z, m_w);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " x = " << m_x << " y = " << m_y << " z = " << m_z << " w = " << m_w;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform4f::name() const 
{
    return "uniform4f";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform4fv::VRWebGLCommand_uniform4fv(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value): m_location(location), m_count(count)
{
    m_value = new GLfloat[count * 4];
    memcpy(m_value, value, count * 4 * sizeof(GLfloat));
}

std::shared_ptr<VRWebGLCommand_uniform4fv> VRWebGLCommand_uniform4fv::newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLfloat *value)
{
    return std::shared_ptr<VRWebGLCommand_uniform4fv>(new VRWebGLCommand_uniform4fv(location, count, value));
}

VRWebGLCommand_uniform4fv::~VRWebGLCommand_uniform4fv()
{
    delete [] m_value;
    m_value = 0;
}

bool VRWebGLCommand_uniform4fv::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform4fv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform4fv::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform4fv(location, m_count, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " count = " << m_count;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform4fv::name() const 
{
    return "uniform4fv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform4i::VRWebGLCommand_uniform4i(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z, GLint w): m_location(location), m_x(x), m_y(y), m_z(z), m_w(w)
{
}

std::shared_ptr<VRWebGLCommand_uniform4i> VRWebGLCommand_uniform4i::newInstance(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z, GLint w)
{
    return std::shared_ptr<VRWebGLCommand_uniform4i>(new VRWebGLCommand_uniform4i(location, x, y, z, w));
}

bool VRWebGLCommand_uniform4i::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform4i::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform4i::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform4i(location, m_x, m_y, m_z, m_w);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " x = " << m_x << " y = " << m_y << " z = " << m_z << " w = " << m_w;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform4i::name() const 
{
    return "uniform4i";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniform4iv::VRWebGLCommand_uniform4iv(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value): m_location(location), m_count(count)
{
    m_value = new GLint[count * 4];
    memcpy(m_value, value, count * 4 * sizeof(GLint));
}

std::shared_ptr<VRWebGLCommand_uniform4iv> VRWebGLCommand_uniform4iv::newInstance(const VRWebGLUniformLocation* location, GLsizei count, const GLint *value)
{
    return std::shared_ptr<VRWebGLCommand_uniform4iv>(new VRWebGLCommand_uniform4iv(location, count, value));
}

VRWebGLCommand_uniform4iv::~VRWebGLCommand_uniform4iv()
{
    delete [] m_value;
    m_value = 0;
}

bool VRWebGLCommand_uniform4iv::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniform4iv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniform4iv::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniform4iv(location, m_count, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " count = " << m_count;
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniform4iv::name() const 
{
    return "uniform4iv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniformMatrix2fv::VRWebGLCommand_uniformMatrix2fv(const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value): m_location(location), m_count(count), m_transpose(transpose)
{
    m_value = new GLfloat[count * 4];
    memcpy(m_value, value, count * 4 * sizeof(GLfloat));
}

std::shared_ptr<VRWebGLCommand_uniformMatrix2fv> VRWebGLCommand_uniformMatrix2fv::newInstance(const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return std::shared_ptr<VRWebGLCommand_uniformMatrix2fv>(new VRWebGLCommand_uniformMatrix2fv(location, count, transpose, value));
}

VRWebGLCommand_uniformMatrix2fv::~VRWebGLCommand_uniformMatrix2fv()
{
    delete [] m_value;
    m_value = 0;
}

bool VRWebGLCommand_uniformMatrix2fv::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniformMatrix2fv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniformMatrix2fv::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniformMatrix2fv(location, m_count, m_transpose, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " count = " << m_count << " transpose = " << (m_transpose ? "true" : "false");
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniformMatrix2fv::name() const 
{
    return "uniformMatrix2fv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniformMatrix3fv::VRWebGLCommand_uniformMatrix3fv(const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value): m_location(location), m_count(count), m_transpose(transpose)
{
    m_value = new GLfloat[count * 9];
    memcpy(m_value, value, count * 9 * sizeof(GLfloat));
}

std::shared_ptr<VRWebGLCommand_uniformMatrix3fv> VRWebGLCommand_uniformMatrix3fv::newInstance(const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return std::shared_ptr<VRWebGLCommand_uniformMatrix3fv>(new VRWebGLCommand_uniformMatrix3fv(location, count, transpose, value));
}

VRWebGLCommand_uniformMatrix3fv::~VRWebGLCommand_uniformMatrix3fv()
{
    delete [] m_value;
    m_value = 0;
}

bool VRWebGLCommand_uniformMatrix3fv::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniformMatrix3fv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniformMatrix3fv::process() 
{
    GLint location = m_location->location();
    VRWebGL_glUniformMatrix3fv(location, m_count, m_transpose, m_value);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " count = " << m_count << " transpose = " << (m_transpose ? "true" : "false");
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniformMatrix3fv::name() const 
{
    return "uniformMatrix3fv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_uniformMatrix4fv::VRWebGLCommand_uniformMatrix4fv(const VRWebGLProgram* program, const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value): m_program(program), m_location(location), m_count(count), m_transpose(transpose)
{
    m_value = new GLfloat[count * 16];
    memcpy(m_value, value, count * 16 * sizeof(GLfloat));
}

std::shared_ptr<VRWebGLCommand_uniformMatrix4fv> VRWebGLCommand_uniformMatrix4fv::newInstance(const VRWebGLProgram* program, const VRWebGLUniformLocation* location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return std::shared_ptr<VRWebGLCommand_uniformMatrix4fv>(new VRWebGLCommand_uniformMatrix4fv(program, location, count, transpose, value));
}

VRWebGLCommand_uniformMatrix4fv::~VRWebGLCommand_uniformMatrix4fv()
{
    delete [] m_value;
    m_value = 0;
}

bool VRWebGLCommand_uniformMatrix4fv::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_uniformMatrix4fv::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_uniformMatrix4fv::process() 
{
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: VRWebGLCommand_uniformMatrix4fv::process() begins";
#endif    
    
    GLuint program = m_program->id();
    GLint location = m_location->location();
    VRWebGLCommandProcessor* commandProcessor = VRWebGLCommandProcessor::getInstance();
    if (commandProcessor->isProjectionMatrixUniformLocation(program, location))
    {
        VRWebGL_glUniformMatrix4fv(location, m_count, m_transpose, commandProcessor->getProjectionMatrix());
#ifdef VRWEBGL_SHOW_LOG  
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " VR projection matrix replaced!";
#endif    
    }
    else 
    {
        bool isModelViewProjectionMatrixUniformLocation = commandProcessor->isModelViewProjectionMatrixUniformLocation(program, location);
        if (commandProcessor->isModelViewMatrixUniformLocation(program, location) || isModelViewProjectionMatrixUniformLocation)
        {
            const GLfloat* cameraWorldMatrix = commandProcessor->getCameraWorldMatrix();
            const GLfloat* cameraWorldMatrixWithTranslationOnly = commandProcessor->getCameraWorldMatrixWithTranslationOnly();

            static GLfloat modelWorldMatrix[16];
            VRWebGL_multiplyMatrices4(cameraWorldMatrix, m_value, modelWorldMatrix);

            static GLfloat modelWorldMatrixWithCameraTranslation[16];
            VRWebGL_multiplyMatrices4(cameraWorldMatrixWithTranslationOnly, modelWorldMatrix, modelWorldMatrixWithCameraTranslation);

            static GLfloat result[16];
            if (isModelViewProjectionMatrixUniformLocation)
            {
                const GLfloat* viewProjectionMatrix = commandProcessor->getViewProjectionMatrix();
                VRWebGL_multiplyMatrices4(viewProjectionMatrix, modelWorldMatrixWithCameraTranslation, result);
            }
            else
            {
                const GLfloat* viewMatrix = commandProcessor->getViewMatrix();
                VRWebGL_multiplyMatrices4(viewMatrix, modelWorldMatrixWithCameraTranslation, result);
            }

            VRWebGL_glUniformMatrix4fv(location, m_count, m_transpose, result);

#ifdef VRWEBGL_SHOW_LOG  
            VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " VR modelview matrix added!" << (isModelViewProjectionMatrixUniformLocation ? " (with model and projection matrix combination)." : "");
#endif    
        }
        else
        {
            VRWebGL_glUniformMatrix4fv(location, m_count, m_transpose, m_value);
        }
    }
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " location = " << location << " count = " << m_count << " transpose = " << (m_transpose ? "true" : "false");
#endif    
    return 0;
}
    
std::string VRWebGLCommand_uniformMatrix4fv::name() const 
{
    return "uniformMatrix4fv";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_useProgram::VRWebGLCommand_useProgram(VRWebGLProgram* program): m_program(program)
{
}

std::shared_ptr<VRWebGLCommand_useProgram> VRWebGLCommand_useProgram::newInstance(VRWebGLProgram* program)
{
    return std::shared_ptr<VRWebGLCommand_useProgram>(new VRWebGLCommand_useProgram(program));
}

bool VRWebGLCommand_useProgram::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_useProgram::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_useProgram::process() 
{
    GLuint program = m_program != 0 ? m_program->id() : 0;
    VRWebGL_glUseProgram(program);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " program = " << program;
#endif    
    return 0;
}

std::string VRWebGLCommand_useProgram::name() const 
{
    return "useProgram";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_vertexAttribPointer::VRWebGLCommand_vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, long long offset): m_index(index), m_size(size), m_type(type), m_normalized(normalized), m_stride(stride), m_offset(offset)
{
}

std::shared_ptr<VRWebGLCommand_vertexAttribPointer> VRWebGLCommand_vertexAttribPointer::newInstance(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, long long offset)
{
    return std::shared_ptr<VRWebGLCommand_vertexAttribPointer>(new VRWebGLCommand_vertexAttribPointer(index, size, type, normalized, stride, offset));
}

bool VRWebGLCommand_vertexAttribPointer::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_vertexAttribPointer::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_vertexAttribPointer::process() 
{
    VRWebGL_glVertexAttribPointer(m_index, m_size, m_type, m_normalized, m_stride, reinterpret_cast<void*>(static_cast<intptr_t>(m_offset)));
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name();
#endif    
    return 0;
}

std::string VRWebGLCommand_vertexAttribPointer::name() const 
{
    return "vertexAttribPointer";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_viewport::VRWebGLCommand_viewport(GLint x, GLint y, GLsizei width, GLsizei height, bool useViewportFromCommandProcessor): m_x(x), m_y(y), m_width(width), m_height(height), m_useViewportFromCommandProcessor(useViewportFromCommandProcessor)
{
}

std::shared_ptr<VRWebGLCommand_viewport> VRWebGLCommand_viewport::newInstance(GLint x, GLint y, GLsizei width, GLsizei height, bool useViewportFromCommandProcessor)
{
    return std::shared_ptr<VRWebGLCommand_viewport>(new VRWebGLCommand_viewport(x, y, width, height, useViewportFromCommandProcessor));
}

bool VRWebGLCommand_viewport::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_viewport::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_viewport::process() 
{
    if (m_useViewportFromCommandProcessor)
    {
        VRWebGLCommandProcessor::getInstance()->getViewport(m_x, m_y, m_width, m_height);
    }
    VRWebGL_glViewport(m_x, m_y, m_width, m_height);
#ifdef VRWEBGL_SHOW_LOG  
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " x = " << m_x << " y = " << m_y << " width = " << m_width << " height = " << m_height;
#endif    
    return 0;
}

std::string VRWebGLCommand_viewport::name() const 
{
    return "viewport";
}

// ======================================================================================
// ======================================================================================

VRWebGLCommand_getString::VRWebGLCommand_getString(GLenum pname): m_pname(pname)
{
}
    
std::shared_ptr<VRWebGLCommand_getString> VRWebGLCommand_getString::newInstance(GLenum pname)
{
    return std::shared_ptr<VRWebGLCommand_getString>(new VRWebGLCommand_getString(pname));
}

bool VRWebGLCommand_getString::isSynchronous() const 
{
    return true;
}

bool VRWebGLCommand_getString::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_getString::process() 
{
    const GLubyte* string = VRWebGL_glGetString(m_pname);
#ifdef VRWEBGL_SHOW_LOG    
    VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << " string = << " << string;
#endif
    return (void*)string;
}

std::string VRWebGLCommand_getString::name() const 
{
    return "getString";
}

// ======================================================================================
// This is not a WebGL/OpenGL command per se. We use it to set the camera world matrix in the right order among other VRWebGLCommands
VRWebGLCommand_setCameraWorldMatrix::VRWebGLCommand_setCameraWorldMatrix(const GLfloat* matrix)
{
    memcpy(m_matrix, matrix, sizeof(m_matrix));
}
    
std::shared_ptr<VRWebGLCommand_setCameraWorldMatrix> VRWebGLCommand_setCameraWorldMatrix::newInstance(const GLfloat* matrix)
{
    return std::shared_ptr<VRWebGLCommand_setCameraWorldMatrix>(new VRWebGLCommand_setCameraWorldMatrix(matrix));
}

bool VRWebGLCommand_setCameraWorldMatrix::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_setCameraWorldMatrix::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_setCameraWorldMatrix::process() 
{
    if (!m_processed)
    {
        VRWebGLCommandProcessor::getInstance()->setCameraWorldMatrix(m_matrix);
#ifdef VRWEBGL_SHOW_LOG    
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name();
#endif
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_setCameraWorldMatrix::name() const 
{
    return "setCameraWorldMatrix";
}

// ======================================================================================
// This is not a WebGL/OpenGL command per se. We use it to set the camera world matrix in the right order among other VRWebGLCommands
VRWebGLCommand_updateSurfaceTexture::VRWebGLCommand_updateSurfaceTexture(GLuint textureId): m_textureId(textureId)
{
}
    
std::shared_ptr<VRWebGLCommand_updateSurfaceTexture> VRWebGLCommand_updateSurfaceTexture::newInstance(GLuint textureId)
{
    return std::shared_ptr<VRWebGLCommand_updateSurfaceTexture>(new VRWebGLCommand_updateSurfaceTexture(textureId));
}

bool VRWebGLCommand_updateSurfaceTexture::isSynchronous() const 
{
    return false;
}

bool VRWebGLCommand_updateSurfaceTexture::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_updateSurfaceTexture::process() 
{
    if (!m_processed)
    {
        GLenum target = GL_TEXTURE_EXTERNAL_OES;
        VRWebGL_glBindTexture(target, m_textureId);
        VRWebGLCommandProcessor::getInstance()->updateSurfaceTexture(m_textureId);
#ifdef VRWEBGL_SHOW_LOG    
        VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << name() << ", textureId = " << m_textureId;
#endif
        m_processed = true;
    }
    return 0;
}

std::string VRWebGLCommand_updateSurfaceTexture::name() const 
{
    return "updateSurfaceTexture";
}
