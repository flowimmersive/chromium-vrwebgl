#include "modules/vr/VRWebGLCommand.h"

unsigned int VRWebGLCommand::numInstances = 0;

VRWebGLCommand::VRWebGLCommand()
{
	numInstances++;
}

VRWebGLCommand::~VRWebGLCommand()
{
	numInstances--;
}

void VRWebGLCommand::setInsideAFrame(bool insideAFrame)
{
	m_insideAFrame = insideAFrame;
}

bool VRWebGLCommand::insideAFrame() const
{
	return m_insideAFrame;
}

bool VRWebGLCommand::isForUpdate() const
{
  return false;
}
