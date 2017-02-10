#include "modules/vr/VRWebGLVideo.h"
#include "modules/vr/VRWebGLCommandProcessor.h"

namespace blink {

VRWebGLVideo* VRWebGLVideo::create()
{
	return new VRWebGLVideo();
}

VRWebGLVideo::~VRWebGLVideo()
{
	VRWebGLCommandProcessor::getInstance()->deleteVideoTexture(m_videoTextureId);
}

GLuint VRWebGLVideo::videoTextureId() const
{
	return m_videoTextureId;
}

String VRWebGLVideo::src() const
{
	return m_src;
}

void VRWebGLVideo::setSrc(const String& src)
{
	if (m_src != src)
	{
		VRWebGLCommandProcessor::getInstance()->setVideoSource(m_videoTextureId, src.utf8().data());
	}
	m_src = src;
	m_prepared = false;
	m_ended = false;
}

double VRWebGLVideo::volume() const
{
	return m_volume;
}

void VRWebGLVideo::setVolume(double volume)
{
	if (m_volume != volume)
	{
		VRWebGLCommandProcessor::getInstance()->setVideoVolume(m_videoTextureId, volume);
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
		VRWebGLCommandProcessor::getInstance()->setVideoLoop(m_videoTextureId, loop);
	}
	m_loop = loop;
}

double VRWebGLVideo::currentTime() const
{
	return VRWebGLCommandProcessor::getInstance()->getVideoCurrentTime(m_videoTextureId);
}

void VRWebGLVideo::setCurrentTime(double currentTime)
{
	if (m_currentTime != currentTime)
	{
		VRWebGLCommandProcessor::getInstance()->setVideoCurrentTime(m_videoTextureId, currentTime);
	}
	m_currentTime = currentTime;
	m_ended = false;
}

bool VRWebGLVideo::paused() const
{
	return m_paused;
}

double VRWebGLVideo::duration() const
{
	return VRWebGLCommandProcessor::getInstance()->getVideoDuration(m_videoTextureId);
}

long VRWebGLVideo::videoWidth() const
{
	return VRWebGLCommandProcessor::getInstance()->getVideoWidth(m_videoTextureId);
}

long VRWebGLVideo::videoHeight() const
{
	return VRWebGLCommandProcessor::getInstance()->getVideoHeight(m_videoTextureId);
}

bool VRWebGLVideo::prepared() const
{
	return m_prepared;
}

bool VRWebGLVideo::ended() const
{
	return m_ended;
}

void VRWebGLVideo::play()
{
	VRWebGLCommandProcessor::getInstance()->playVideo(m_videoTextureId, m_volume, m_loop);
	m_paused = false;
}

void VRWebGLVideo::pause()
{
	VRWebGLCommandProcessor::getInstance()->pauseVideo(m_videoTextureId);
	m_paused = true;
}

bool VRWebGLVideo::checkPrepared() 
{
	m_prepared = VRWebGLCommandProcessor::getInstance()->checkVideoPrepared(m_videoTextureId);
	return m_prepared;
}

bool VRWebGLVideo::checkEnded() 
{
	m_ended = VRWebGLCommandProcessor::getInstance()->checkVideoEnded(m_videoTextureId);
	return m_ended;
}

VRWebGLVideo::VRWebGLVideo()
{
	m_videoTextureId = VRWebGLCommandProcessor::getInstance()->newVideoTexture();
}

DEFINE_TRACE(VRWebGLVideo)
{
}

} // namespace blink
