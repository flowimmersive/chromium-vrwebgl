#include "modules/vr/VRWebGLShader.h"

namespace blink {

VRWebGLShader::VRWebGLShader(GLenum type): 
	cached(false),
	m_type(type), 
	m_compileStatus(0), 
	m_deleted(false)
{
}

void VRWebGLShader::setCompileStatus(GLint compileStatus)
{
	m_compileStatus = compileStatus;
}

void VRWebGLShader::setInfoLog(const std::string& infoLog)
{
	m_infoLog = infoLog;
}

void VRWebGLShader::markAsDeleted()
{
	m_deleted = true;
}

GLenum VRWebGLShader::getType() const
{
	return m_type;
}

GLint VRWebGLShader::getCompileStatus() const
{
	return m_compileStatus;
}

std::string VRWebGLShader::getInfoLog() const
{
	return m_infoLog;
}

bool VRWebGLShader::isDeleted() const
{
	return m_deleted;
}

} // namespace blink
