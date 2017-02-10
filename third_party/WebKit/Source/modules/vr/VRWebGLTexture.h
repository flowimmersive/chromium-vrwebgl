#ifndef VRWebGLTexture_h
#define VRWebGLTexture_h

#include "modules/vr/VRWebGLObject.h"
#include "wtf/text/WTFString.h"

namespace blink {

class VRWebGLTexture final : public VRWebGLObject {
public:
	void setVideoTextureId(GLuint videoTextureId);
	GLuint videoTextureId() const;
	
    DEFINE_WRAPPERTYPEINFO();
private:
	GLuint m_videoTextureId = 0;
};

} // namespace blink

#endif // VRWebGLTexture_h
