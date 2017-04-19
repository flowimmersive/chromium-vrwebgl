#include "modules/vr/VRWebGLTexture.h"

namespace blink {

void VRWebGLTexture::setExternalTextureId(GLuint externalTextureId)
{
	m_externalTextureId = externalTextureId;
}

GLuint VRWebGLTexture::externalTextureId() const
{
	return m_externalTextureId;
}

} // namespace blink
