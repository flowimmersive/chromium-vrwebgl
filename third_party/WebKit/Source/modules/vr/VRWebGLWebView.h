#ifndef VRWebGLWebView_h
#define VRWebGLWebView_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "wtf/text/WTFString.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

namespace blink {

class VRWebGLWebView final : public GarbageCollectedFinalized<VRWebGLWebView>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static VRWebGLWebView* create();
    ~VRWebGLWebView();

    GLuint textureId() const;
    long id() const;

    String src() const;
    void setSrc(const String&);

    long width() const;
    long height() const;

    void touchstart(float x, float y);
    void touchmove(float x, float y);
    void touchend(float x, float y);

    void back();
    void forward();
    void reload();
    void voiceSearch();

    void keydown(int keycode);
    void keyup(int keycode);

    void cursorenter(float x, float y);
    void cursormove(float x, float y);
    void cursorexit();

    bool transparent() const;
    void setTransparent(bool transparent);

    void setScale(long scale, bool loadWithOverviewMode, bool useWideViewPort);

    DECLARE_TRACE()

private:
    VRWebGLWebView();

    GLuint m_textureId = -1;
    String m_src;
    bool m_transparent = false;
};

} // namespace blink

#endif // VRWebGLWebView_h
