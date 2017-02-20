#include "modules/vr/VRWebGLProgram.h"
#include "modules/vr/VRWebGLActiveInfo.h"

namespace blink {

VRWebGLProgram::VRWebGLProgram():
	cached(false),
	m_numberOfShaders(0),
	m_deleted(false),
	m_validateStatus(0),
	m_linkStatus(0),
	m_activeUniformMaxLength(0),
	m_activeAttributeMaxLength(0)
{
}

void VRWebGLProgram::addActiveUniform(const std::string& name, GLenum type, GLint size)
{
	m_activeUniforms.push_back(std::shared_ptr<ActiveInfo>(new ActiveInfo(name.c_str(), type, size)));
}

void VRWebGLProgram::addActiveAttribute(const std::string& name, GLenum type, GLint size)
{
	m_activeAttributes.push_back(std::shared_ptr<ActiveInfo>(new ActiveInfo(name.c_str(), type, size)));
}

void VRWebGLProgram::addAttributeLocation(const std::string& name, GLint location)
{
	std::string names;
	for (std::map<std::string, GLint>::iterator it = m_attributeLocations.begin(); it != m_attributeLocations.end(); it++)
	{
		names += it->first + ", ";
	}
	VLOG(0) << "VRWebGLProgram::addAttributeLocation('" << name.c_str() << "') Vs " << names.c_str() << std::endl;
	m_attributeLocations[name] = location;
}

void VRWebGLProgram::addShader()
{
	m_numberOfShaders++;
}

void VRWebGLProgram::markAsDeleted()
{
	m_deleted = true;
}

void VRWebGLProgram::setValidateStatus(GLint validateStatus)
{
	m_validateStatus = validateStatus;
}

void VRWebGLProgram::setLinkStatus(GLint linkStatus)
{
	m_linkStatus = linkStatus;
}

void VRWebGLProgram::setActiveUniformMaxLength(GLint activeUniformMaxLength)
{
	m_activeUniformMaxLength = activeUniformMaxLength;
}

void VRWebGLProgram::setActiveAttributeMaxLength(GLint activeAttributeMaxLength)
{
	m_activeAttributeMaxLength = activeAttributeMaxLength;
}

void VRWebGLProgram::setInfoLog(const std::string& infoLog)
{
	m_infoLog = infoLog;
}

size_t VRWebGLProgram::getNumberOfActiveUniforms() const
{
	return m_activeUniforms.size();
}

size_t VRWebGLProgram::getNumberOfActiveAttributes() const
{
	return m_activeAttributes.size();
}

std::shared_ptr<VRWebGLProgram::ActiveInfo> VRWebGLProgram::getActiveUniform(size_t index) const
{
	return index < m_activeUniforms.size() ? m_activeUniforms[index] : std::shared_ptr<ActiveInfo>(); 
}

std::shared_ptr<VRWebGLProgram::ActiveInfo> VRWebGLProgram::getActiveAttribute(unsigned int index) const
{
	return index < m_activeAttributes.size() ? m_activeAttributes[index] : std::shared_ptr<ActiveInfo>(); 
}

GLint VRWebGLProgram::getAttributeLocation(const std::string& name)
{
	std::string names;
	for (std::map<std::string, GLint>::iterator it = m_attributeLocations.begin(); it != m_attributeLocations.end(); it++)
	{
		names += it->first + ", ";
	}
	VLOG(0) << "VRWebGLProgram::getAttributeLocation('" << name.c_str() << "') Vs " << names.c_str() << std::endl;
	return m_attributeLocations.find(name) == m_attributeLocations.end() ? -1 : m_attributeLocations[name];
}

size_t VRWebGLProgram::getNumberOfShaders() const
{
	return m_numberOfShaders;
}

bool VRWebGLProgram::isDeleted() const
{
	return m_deleted;
}

GLint VRWebGLProgram::getValidateStatus() const
{
	return m_validateStatus;
}

GLint VRWebGLProgram::getLinkStatus() const
{
	return m_linkStatus;
}

GLint VRWebGLProgram::getActiveUniformMaxLength() const
{
	return m_activeUniformMaxLength;
}

GLint VRWebGLProgram::getActiveAttributeMaxLength() const
{
	return m_activeAttributeMaxLength;
}

std::string VRWebGLProgram::getInfoLog() const
{
	return m_infoLog;
}

} // namespace blink