#ifndef VRWebGLObject_h
#define VRWebGLObject_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "platform/heap/Handle.h"
// #include "third_party/khronos/GLES2/gl2.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
// #include <GLES3/gl3ext.h>


namespace blink {

class VRWebGLObject : public GarbageCollectedFinalized<VRWebGLObject>, public ScriptWrappable 
{
public:
    DEFINE_INLINE_VIRTUAL_TRACE() { }

    VRWebGLObject();
    virtual ~VRWebGLObject() { }

    inline GLuint id() const { return m_id; }
    void setId(GLuint id);
    
private:
	GLuint m_id;
};

}

#endif