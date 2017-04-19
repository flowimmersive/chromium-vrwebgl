#ifndef VRWebGLSpeechRecognition_h
#define VRWebGLSpeechRecognition_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "wtf/text/WTFString.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

namespace blink {

class VRWebGLSpeechRecognition final : public GarbageCollectedFinalized<VRWebGLSpeechRecognition>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static VRWebGLSpeechRecognition* create();
    ~VRWebGLSpeechRecognition();

    long id() const;

    void start();
    void stop();

    DECLARE_TRACE()

private:
    static long ids;
    long m_id;

    VRWebGLSpeechRecognition();
};

} // namespace blink

#endif // VRWebGLSpeechRecognition_h
