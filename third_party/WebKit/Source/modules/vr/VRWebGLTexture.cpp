#include "modules/vr/VRWebGLTexture.h"

namespace blink {

void VRWebGLTexture::setVideoTextureId(GLuint videoTextureId)
{
	m_videoTextureId = videoTextureId;
}

GLuint VRWebGLTexture::videoTextureId() const
{
	return m_videoTextureId;
}

} // namespace blink
