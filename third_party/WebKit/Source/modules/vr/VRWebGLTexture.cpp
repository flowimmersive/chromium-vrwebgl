#include "modules/vr/VRWebGLTexture.h"

namespace blink {

void VRWebGLTexture::setTextureId(GLuint textureId)
{
	m_textureId = textureId;
}

GLuint VRWebGLTexture::textureId() const
{
	return m_textureId;
}

} // namespace blink
