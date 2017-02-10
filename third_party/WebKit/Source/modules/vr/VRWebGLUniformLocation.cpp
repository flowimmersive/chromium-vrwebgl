#include "modules/vr/VRWebGLUniformLocation.h"

namespace blink {

GLint VRWebGLUniformLocation::location() const
{
	return m_location;
}

void VRWebGLUniformLocation::setLocation(GLint location)
{
	m_location = location;
}

DEFINE_TRACE(VRWebGLUniformLocation)
{
}

} // namespace blink
