#ifndef VRWebGLExtension_h
#define VRWebGLExtension_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "platform/heap/Handle.h"

namespace blink {

class VRWebGLExtension final : public GarbageCollected<VRWebGLExtension>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();

public:

    DECLARE_TRACE();
};

} // namespace blink

#endif // VRWebGLExtension_h
