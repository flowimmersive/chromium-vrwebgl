#ifndef VRWebGLTexture_h
#define VRWebGLTexture_h

#include "modules/vr/VRWebGLObject.h"
#include "wtf/text/WTFString.h"

namespace blink {

class VRWebGLTexture final : public VRWebGLObject {
public:
	void setTextureId(GLuint videoTextureId);
	GLuint textureId() const;
	
    DEFINE_WRAPPERTYPEINFO();
private:
	GLuint m_textureId = 0;
};

} // namespace blink

#endif // VRWebGLTexture_h
