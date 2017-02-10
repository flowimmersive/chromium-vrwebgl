#include "modules/vr/VRWebGLObject.h"

namespace blink {

VRWebGLObject::VRWebGLObject(): m_id(0)
{
}

void VRWebGLObject::setId(GLuint id)
{
	ASSERT(!m_id); // Do not allow to assign the id twice
	m_id = id;
}

}

