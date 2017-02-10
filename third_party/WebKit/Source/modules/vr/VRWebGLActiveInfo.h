#ifndef VRWebGLActiveInfo_h
#define VRWebGLActiveInfo_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "third_party/khronos/GLES2/gl2.h"

namespace blink {

class VRWebGLActiveInfo final : public GarbageCollectedFinalized<VRWebGLActiveInfo>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static VRWebGLActiveInfo* create(const String& name, GLenum type, GLint size)
    {
        return new VRWebGLActiveInfo(name, type, size);
    }
    String name() const { return m_name; }
    GLenum type() const { return m_type; }
    GLint size() const { return m_size; }

    DEFINE_INLINE_TRACE() { }

private:
    VRWebGLActiveInfo(const String& name, GLenum type, GLint size)
        : m_name(name)
        , m_type(type)
        , m_size(size)
    {
        ASSERT(name.length());
        ASSERT(type);
        ASSERT(size);
    }
    String m_name;
    GLenum m_type;
    GLint m_size;
};

} // namespace blink

#endif // VRWebGLActiveInfo_h
