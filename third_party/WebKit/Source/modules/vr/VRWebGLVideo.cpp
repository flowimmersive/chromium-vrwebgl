#include "modules/vr/VRWebGLVideo.h"
#include "modules/vr/VRWebGLCommandProcessor.h"

namespace blink {

VRWebGLVideo* VRWebGLVideo::create()
{
	return new VRWebGLVideo();
}

VRWebGLVideo::~VRWebGLVideo()
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_deleteVideo::newInstance(m_textureId));
}

GLuint VRWebGLVideo::textureId() const
{
	return m_textureId;
}

long VRWebGLVideo::id() const
{
	return m_textureId;
}

String VRWebGLVideo::src() const
{
	return m_src;
}

void VRWebGLVideo::setSrc(const String& src)
{
	if (m_src != src)
	{
		VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_setVideoSrc::newInstance(m_textureId, src.utf8().data()));
		m_readyState = 0;
	}
	m_src = src;
}

double VRWebGLVideo::volume() const
{
	return m_volume;
}

void VRWebGLVideo::setVolume(double volume)
{
	if (m_volume != volume)
	{
		VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_setVideoVolume::newInstance(m_textureId, (float)volume));
	}
	m_volume = volume;
}

bool VRWebGLVideo::loop() const
{
	return m_loop;
}

void VRWebGLVideo::setLoop(bool loop)
{
	if (m_loop != loop)
	{
		VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_setVideoLoop::newInstance(m_textureId, loop));
	}
	m_loop = loop;
}

double VRWebGLVideo::currentTime() const
{
	return *(float*)VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_getVideoCurrentTime::newInstance(m_textureId));
}

void VRWebGLVideo::setCurrentTime(double currentTime)
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_setVideoCurrentTime::newInstance(m_textureId, (float)currentTime));
	m_ended = false; // TODO: It this correct?
}

long VRWebGLVideo::readyState() const
{
	return m_readyState;
}

void VRWebGLVideo::setReadyState(long readyState)
{
	m_readyState = readyState;
}

bool VRWebGLVideo::paused() const
{
	return m_paused;
}

double VRWebGLVideo::duration() const
{
	return *(float*)VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_getVideoDuration::newInstance(m_textureId));
}

long VRWebGLVideo::videoWidth() const
{
	return *(long*)VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_getVideoWidth::newInstance(m_textureId));
}

long VRWebGLVideo::videoHeight() const
{
	return *(long*)VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_getVideoHeight::newInstance(m_textureId));
}

bool VRWebGLVideo::ended() const
{
	return m_ended;
}

void VRWebGLVideo::setEnded(bool ended)
{
	m_ended = ended;
}

void VRWebGLVideo::play()
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchVideoPlaybackEvent::newInstance(m_textureId, VRWebGLCommand_dispatchVideoPlaybackEvent::PLAY, m_volume, m_loop));
	m_paused = false;
}

void VRWebGLVideo::pause()
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchVideoPlaybackEvent::newInstance(m_textureId, VRWebGLCommand_dispatchVideoPlaybackEvent::PAUSE, m_volume, m_loop));
	m_paused = true;
}

VRWebGLVideo::VRWebGLVideo()
{
	m_textureId = *(GLuint*)VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_newVideo::newInstance());
}

DEFINE_TRACE(VRWebGLVideo)
{
}

} // namespace blink
