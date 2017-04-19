#ifndef VRWebGLTexture_h
#define VRWebGLTexture_h

#include "modules/vr/VRWebGLObject.h"
#include "wtf/text/WTFString.h"

namespace blink {

class VRWebGLTexture final : public VRWebGLObject {
public:
	void setExternalTextureId(GLuint externalTextureId);
	GLuint externalTextureId() const;
	
    DEFINE_WRAPPERTYPEINFO();
private:
	GLuint m_externalTextureId = 0;
};

} // namespace blink

#endif // VRWebGLTexture_h
