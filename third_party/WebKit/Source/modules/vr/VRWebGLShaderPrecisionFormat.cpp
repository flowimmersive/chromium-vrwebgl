#include "modules/vr/VRWebGLShaderPrecisionFormat.h"

namespace blink {

// static
VRWebGLShaderPrecisionFormat* VRWebGLShaderPrecisionFormat::create(GLint rangeMin, GLint rangeMax, GLint precision)
{
    return new VRWebGLShaderPrecisionFormat(rangeMin, rangeMax, precision);
}

GLint VRWebGLShaderPrecisionFormat::rangeMin() const
{
    return m_rangeMin;
}

GLint VRWebGLShaderPrecisionFormat::rangeMax() const
{
    return m_rangeMax;
}

GLint VRWebGLShaderPrecisionFormat::precision() const
{
    return m_precision;
}

VRWebGLShaderPrecisionFormat::VRWebGLShaderPrecisionFormat(GLint rangeMin, GLint rangeMax, GLint precision)
    : m_rangeMin(rangeMin)
    , m_rangeMax(rangeMax)
    , m_precision(precision)
{
}

} // namespace blink
