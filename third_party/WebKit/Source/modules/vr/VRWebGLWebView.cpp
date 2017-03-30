#include "modules/vr/VRWebGLWebView.h"
#include "modules/vr/VRWebGLCommandProcessor.h"

namespace blink {

VRWebGLWebView* VRWebGLWebView::create()
{
	return new VRWebGLWebView();
}

VRWebGLWebView::~VRWebGLWebView()
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_deleteWebView::newInstance(m_textureId));
}

GLuint VRWebGLWebView::textureId() const
{
	return m_textureId;
}

long VRWebGLWebView::id() const
{
	return m_textureId;
}

String VRWebGLWebView::src() const
{
	return m_src;
}

void VRWebGLWebView::setSrc(const String& src)
{
	if (m_src != src)
	{
		VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_setWebViewSrc::newInstance(m_textureId, src.utf8().data()));
	}
	m_src = src;
	m_loaded = false;
}

long VRWebGLWebView::width() const
{
	return VRWebGLCommandProcessor::getInstance()->getVideoWidth(m_textureId);
}

long VRWebGLWebView::height() const
{
	return VRWebGLCommandProcessor::getInstance()->getVideoHeight(m_textureId);
}

void VRWebGLWebView::touchstart(float x, float y)
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewTouchEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewTouchEvent::TOUCH_START, x, y));
}

void VRWebGLWebView::touchmove(float x, float y)
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewTouchEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewTouchEvent::TOUCH_MOVE, x, y));
}

void VRWebGLWebView::touchend(float x, float y)
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewTouchEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewTouchEvent::TOUCH_END, x, y));
}

void VRWebGLWebView::back()
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewNavigationEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewNavigationEvent::NAVIGATION_BACK));
}

void VRWebGLWebView::forward()
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewNavigationEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewNavigationEvent::NAVIGATION_FORWARD));
}

void VRWebGLWebView::reload()
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewNavigationEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewNavigationEvent::NAVIGATION_RELOAD));
}

void VRWebGLWebView::voiceSearch()
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewNavigationEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewNavigationEvent::NAVIGATION_VOICE_SEARCH));
}

void VRWebGLWebView::keydown(int keycode)
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewKeyboardEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewKeyboardEvent::KEY_DOWN, keycode));
}

void VRWebGLWebView::keyup(int keycode)
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewKeyboardEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewKeyboardEvent::KEY_UP, keycode));
}

void VRWebGLWebView::cursorenter(float x, float y)
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewCursorEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewCursorEvent::CURSOR_ENTER, x, y));
}

void VRWebGLWebView::cursormove(float x, float y)
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewCursorEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewCursorEvent::CURSOR_MOVE, x, y));
}

void VRWebGLWebView::cursorexit()
{
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_dispatchWebViewCursorEvent::newInstance(m_textureId, VRWebGLCommand_dispatchWebViewCursorEvent::CURSOR_EXIT, -1, -1));
}

VRWebGLWebView::VRWebGLWebView()
{
	m_textureId = *(GLuint*)VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_newWebView::newInstance());
}

DEFINE_TRACE(VRWebGLWebView)
{
}

} // namespace blink
