#ifndef VRWebGLUniformLocation_h
#define VRWebGLUniformLocation_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "platform/heap/Handle.h"
// #include "third_party/khronos/GLES2/gl2.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
// #include <GLES3/gl3ext.h>

namespace blink {

class VRWebGLUniformLocation final : public GarbageCollected<VRWebGLUniformLocation>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();

public:
    GLint location() const;
    void setLocation(GLint location);

    DECLARE_TRACE();
    
private:
   GLint m_location;
};

} // namespace blink

#endif // VRWebGLUniformLocation_h
