#ifndef ANGLEInstancedArrays_h
#define ANGLEInstancedArrays_h

#include "modules/webgl/WebGLExtension.h"

namespace blink {

class WebGLRenderingContextBase;

class VRWebGLANGLEInstancedArrays final : public GarbageCollectedFinalized<VRWebGLANGLEInstancedArrays>, public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static VRWebGLANGLEInstancedArrays* create();

  // static bool supported(WebGLRenderingContextBase*);
  // static const char* extensionName();
  // WebGLExtensionName name() const override;

  void drawArraysInstancedANGLE(GLenum mode,
                                GLint first,
                                GLsizei count,
                                GLsizei primcount);
  void drawElementsInstancedANGLE(GLenum mode,
                                  GLsizei count,
                                  GLenum type,
                                  long long offset,
                                  GLsizei primcount);
  void vertexAttribDivisorANGLE(GLuint index, GLuint divisor);

  DECLARE_TRACE()

 private:
  explicit VRWebGLANGLEInstancedArrays();
};

}  // namespace blink

#endif  // ANGLEInstancedArrays_h
