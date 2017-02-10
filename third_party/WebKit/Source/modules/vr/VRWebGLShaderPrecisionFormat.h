#ifndef VRWebGLShaderPrecisionFormat_h
#define VRWebGLShaderPrecisionFormat_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "third_party/khronos/GLES2/gl2.h"

namespace blink {

class VRWebGLShaderPrecisionFormat final : public GarbageCollected<VRWebGLShaderPrecisionFormat>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static VRWebGLShaderPrecisionFormat* create(GLint rangeMin, GLint rangeMax, GLint precision);

    GLint rangeMin() const;
    GLint rangeMax() const;
    GLint precision() const;

    DEFINE_INLINE_TRACE() { }

private:
    VRWebGLShaderPrecisionFormat(GLint rangeMin, GLint rangeMax, GLint precision);

    GLint m_rangeMin;
    GLint m_rangeMax;
    GLint m_precision;
};

} // namespace blink

#endif // VRWebGLShaderPrecisionFormat_h