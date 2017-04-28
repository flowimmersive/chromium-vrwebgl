#include "modules/vr/VRWebGLANGLEInstancedArrays.h"
#include "modules/vr/VRWebGL.h"
#include "modules/vr/VRWebGLCommandProcessor.h"

namespace blink {

VRWebGLANGLEInstancedArrays::VRWebGLANGLEInstancedArrays()
{
}

VRWebGLANGLEInstancedArrays* VRWebGLANGLEInstancedArrays::create() {
  return new VRWebGLANGLEInstancedArrays();
}

void VRWebGLANGLEInstancedArrays::drawArraysInstancedANGLE(GLenum mode,
                                                    GLint first,
                                                    GLsizei count,
                                                    GLsizei primcount) 
{
  VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_drawArraysInstancedANGLE::newInstance(mode, first, count, primcount));
}

void VRWebGLANGLEInstancedArrays::drawElementsInstancedANGLE(GLenum mode,
                                                      GLsizei count,
                                                      GLenum type,
                                                      long long offset,
                                                      GLsizei primcount) 
{
  VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_drawElementsInstancedANGLE::newInstance(mode, count, type, offset, primcount));
}

void VRWebGLANGLEInstancedArrays::vertexAttribDivisorANGLE(GLuint index,
                                                    GLuint divisor) 
{
  VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_vertexAttribDivisorANGLE::newInstance(index, divisor));
}

DEFINE_TRACE(VRWebGLANGLEInstancedArrays)
{
}

}  // namespace blink
