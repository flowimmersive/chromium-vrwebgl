#ifndef VRWebGLVideo_h
#define VRWebGLVideo_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "wtf/text/WTFString.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

namespace blink {

class VRWebGLVideo final : public GarbageCollectedFinalized<VRWebGLVideo>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static VRWebGLVideo* create();
    ~VRWebGLVideo();

    GLuint videoTextureId() const;

    String src() const;
    void setSrc(const String&);

    double volume() const;
    void setVolume(double);

    bool loop() const;
    void setLoop(bool);

    double currentTime() const;
    void setCurrentTime(double);

    bool paused() const;
    double duration() const;
    long videoWidth() const;
    long videoHeight() const;
    bool prepared() const;
    bool ended() const;

    void play();
    void pause();
    bool checkPrepared();
    bool checkEnded();

    DECLARE_TRACE()

private:
    VRWebGLVideo();

    GLuint m_videoTextureId = -1;
    String m_src;
    double m_volume = 1.0;
    bool m_loop = false;
    double m_currentTime = 0;
    bool m_paused = true;
    bool m_prepared = false;
    bool m_ended = false;
};

} // namespace blink

#endif // VRWebGLVideo_h
