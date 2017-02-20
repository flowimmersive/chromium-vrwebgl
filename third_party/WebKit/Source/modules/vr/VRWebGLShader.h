#ifndef VRWebGLShader_h
#define VRWebGLShader_h

#include "modules/vr/VRWebGLObject.h"
#include "wtf/text/WTFString.h"

#include <atomic>

namespace blink {

class VRWebGLShader final : public VRWebGLObject {
    DEFINE_WRAPPERTYPEINFO();

	public:
    VRWebGLShader(GLenum type);

    void setCompileStatus(GLint compileStatus);
    void setInfoLog(const std::string& infoLog);
    void markAsDeleted();
    GLenum getType() const;
    GLint getCompileStatus() const;
    std::string getInfoLog() const;
    bool isDeleted() const;

    std::atomic<bool> cached;

  private:
  	GLenum m_type;
  	GLint m_compileStatus;
  	std::string m_infoLog;
  	bool m_deleted;
};

} // namespace blink

#endif // VRWebGLShader_h
