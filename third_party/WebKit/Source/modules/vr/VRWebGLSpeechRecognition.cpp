#include "modules/vr/VRWebGLSpeechRecognition.h"
#include "modules/vr/VRWebGLCommandProcessor.h"

namespace blink {

long VRWebGLSpeechRecognition::ids = 0;

VRWebGLSpeechRecognition* VRWebGLSpeechRecognition::create()
{
  return new VRWebGLSpeechRecognition();
}

VRWebGLSpeechRecognition::~VRWebGLSpeechRecognition()
{
  VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_deleteSpeechRecognition::newInstance(m_id));
}

long VRWebGLSpeechRecognition::id() const
{
  return m_id;
}

void VRWebGLSpeechRecognition::start()
{
  VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_startSpeechRecognition::newInstance(m_id));
}

void VRWebGLSpeechRecognition::stop()
{
  VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_stopSpeechRecognition::newInstance(m_id));
}

VRWebGLSpeechRecognition::VRWebGLSpeechRecognition()
{
  m_id = ids++;
  VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_newSpeechRecognition::newInstance(m_id));
}

DEFINE_TRACE(VRWebGLSpeechRecognition)
{
}

} // namespace blink
