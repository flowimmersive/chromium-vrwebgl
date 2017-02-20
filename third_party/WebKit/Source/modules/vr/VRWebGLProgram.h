#ifndef VRWebGLProgram_h
#define VRWebGLProgram_h

#include "modules/vr/VRWebGLObject.h"
#include "wtf/text/WTFString.h"

#include <vector>
#include <map>
#include <memory>
#include <atomic>

namespace blink {

class VRWebGLProgram final : public VRWebGLObject {
    DEFINE_WRAPPERTYPEINFO();

  public:

  	class ActiveInfo
  	{
  	public:
  		ActiveInfo(const std::string& name, GLenum type, GLint size): name(name), type(type), size(size) {}
  		std::string name;
  		GLenum type;
  		GLint size;
  	};

  	VRWebGLProgram();

  	void addActiveUniform(const std::string& name, GLenum type, GLint size);
  	void addActiveAttribute(const std::string& name, GLenum type, GLint size);
    void addAttributeLocation(const std::string& name, GLint location);
  	void addShader();
  	void markAsDeleted();
  	void setValidateStatus(GLint validateStatus);
  	void setLinkStatus(GLint linkStatus);
  	void setActiveUniformMaxLength(GLint activeUniformMaxLength);
  	void setActiveAttributeMaxLength(GLint activeAttributeMaxLength);
  	void setInfoLog(const std::string& infoLog);
  	size_t getNumberOfActiveUniforms() const;
  	size_t getNumberOfActiveAttributes() const;
  	std::shared_ptr<ActiveInfo> getActiveUniform(unsigned int index) const;
  	std::shared_ptr<ActiveInfo> getActiveAttribute(unsigned int index) const;
    GLint getAttributeLocation(const std::string& name);
  	size_t getNumberOfShaders() const;
  	bool isDeleted() const;
  	GLint getValidateStatus() const;
  	GLint getLinkStatus() const;
  	GLint getActiveUniformMaxLength() const;
  	GLint getActiveAttributeMaxLength() const;
  	std::string getInfoLog() const;

  	std::atomic<bool> cached;

  private:
  	std::vector<std::shared_ptr<ActiveInfo>> m_activeUniforms;
  	std::vector<std::shared_ptr<ActiveInfo>> m_activeAttributes;
    std::map<std::string, GLint> m_attributeLocations;
  	size_t m_numberOfShaders;
  	bool m_deleted;
  	GLint m_validateStatus;
  	GLint m_linkStatus;
  	GLint m_activeUniformMaxLength;
  	GLint m_activeAttributeMaxLength;
  	std::string m_infoLog;
};

} // namespace blink

#endif // VRWebGLProgram_h
