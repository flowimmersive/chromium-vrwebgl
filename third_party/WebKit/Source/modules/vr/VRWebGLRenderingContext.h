#ifndef VRWebGLRenderingContext_h
#define VRWebGLRenderingContext_h

#include "platform/heap/Handle.h"
#include "platform/graphics/gpu/WebGLImageConversion.h"
#include "platform/graphics/ImageBuffer.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "wtf/HashSet.h"
#include "modules/vr/VRWebGLObject.h"
#include "modules/vr/VRWebGLPose.h"
#include "modules/vr/VRWebGLEyeParameters.h"
#include "core/dom/DOMTypedArray.h"
#include "bindings/core/v8/ScriptWrappable.h"
#include "bindings/core/v8/ScriptState.h"
#include "bindings/core/v8/ScriptValue.h"
#include "bindings/core/v8/Nullable.h"
#include "core/dom/TypedFlexibleArrayBufferView.h"

// TODO: Remove any reference to this macro as it was an initial testing solution while the Oculud Mobile SDK thread was still not ready.
// #define VRWEBGL_SIMULATE_OPENGL_THREAD   

namespace blink {

class VRWebGLShader;
class VRWebGLTexture;
class VRWebGLProgram;
class VRWebGLBuffer;
class VRWebGLUniformLocation;
class VRWebGLShaderPrecisionFormat;
class VRWebGLActiveInfo;
class VRWebGLFramebuffer;
class VRWebGLRenderbuffer;
class VRWebGLVideo;
class VRWebGLWebView;

class VRWebGLANGLEInstancedArrays;

class HTMLFormElement;
class HTMLImageElement;
class HTMLCanvasElement;
class HTMLVideoElement;
class ImageBitmap;
class ImageData;

class VRPose;
class VREyeParameters;
class Gamepad;

class VRWebGLRenderingContext : public GarbageCollectedFinalized<VRWebGLRenderingContext>, public ScriptWrappable 
{
    DEFINE_WRAPPERTYPEINFO();
public:
	// Constructor
	// static VRWebGLRenderingContext* create(HTMLCanvasElement*);
    static VRWebGLRenderingContext* create();

	virtual ~VRWebGLRenderingContext();

	// IDL properties
    // HTMLCanvasElement* canvas() const { return m_canvas; }

    // IDL methods
    void activeTexture(GLenum texture);
    void attachShader(ScriptState*, VRWebGLProgram*, VRWebGLShader*);    
    void bindAttribLocation(VRWebGLProgram* program, GLuint index, const String& name);
    void bindBuffer(ScriptState*, GLenum target, VRWebGLBuffer*);
    void bindFramebuffer(ScriptState* scriptState, GLenum target, VRWebGLFramebuffer* buffer);
    void bindRenderbuffer(ScriptState* scriptState, GLenum target, VRWebGLRenderbuffer* buffer);
    void bindTexture(ScriptState* scriptState, GLenum target, VRWebGLTexture* texture);
    void blendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void blendEquation(GLenum mode);
    void blendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
    void blendFunc(GLenum sfactor, GLenum dfactor);
    void blendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    void bufferData(GLenum target, long long size, GLenum usage);
    void bufferData(GLenum target, DOMArrayBuffer* data, GLenum usage);
    void bufferData(GLenum target, DOMArrayBufferView* data, GLenum usage);
    void bufferSubData(GLenum target, long long offset, DOMArrayBuffer* data);
    void bufferSubData(GLenum target, long long offset, const FlexibleArrayBufferView& data);
    GLenum checkFramebufferStatus(GLenum target);
    void clear(GLbitfield mask);
    void clearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void clearDepth(GLfloat depth);
    void clearStencil(GLint s);
    void colorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    void compileShader(VRWebGLShader*);
	VRWebGLBuffer* createBuffer();
    VRWebGLFramebuffer* createFramebuffer();
	VRWebGLProgram* createProgram();
    VRWebGLRenderbuffer* createRenderbuffer();
	VRWebGLShader* createShader(GLenum type);
    VRWebGLTexture* createTexture();
    void cullFace(GLenum mode);
    void deleteBuffer(VRWebGLBuffer* buffer);
    void deleteFramebuffer(VRWebGLFramebuffer* framebuffer);
    void deleteProgram(VRWebGLProgram* program);
    void deleteRenderbuffer(VRWebGLRenderbuffer* renderbuffer);
    void deleteShader(VRWebGLShader* shader);
    void deleteTexture(VRWebGLTexture* texture);
    void depthFunc(GLenum func);
    void depthMask(GLboolean flag);
    void depthRange(GLfloat zNear, GLfloat zFar);
    void detachShader(ScriptState* scriptState, VRWebGLProgram* program, VRWebGLShader* shader);
    void disable(GLenum cap);
    void disableVertexAttribArray(GLuint index);
    void drawArrays(GLenum mode, GLint first, GLsizei count);
    void drawElements(GLenum mode, GLsizei count, GLenum type, long long offset);
    void enable(GLenum cap);
    void enableVertexAttribArray(GLuint index);
    void framebufferRenderbuffer(ScriptState* scriptState, GLenum target, GLenum attachment, GLenum renderbuffertarget, VRWebGLRenderbuffer* buffer);
    void framebufferTexture2D(ScriptState* scriptState, GLenum target, GLenum attachment, GLenum textarget, VRWebGLTexture* texture, GLint level);
    void frontFace(GLenum mode);
    void generateMipmap(GLenum target);
    VRWebGLActiveInfo* getActiveAttrib(VRWebGLProgram* program, GLuint index);
    VRWebGLActiveInfo* getActiveUniform(VRWebGLProgram* program, GLuint index);
    GLint getAttribLocation(VRWebGLProgram*, const String& name);
    Nullable<Vector<String>> getSupportedExtensions();
    GLenum getError();
    ScriptValue getExtension(ScriptState* scriptState, const String& name);
    ScriptValue getParameter(ScriptState* scriptState, GLenum pname);
    ScriptValue getProgramParameter(ScriptState*, VRWebGLProgram*, GLenum pname);
    String getProgramInfoLog(VRWebGLProgram* program);
    ScriptValue getShaderParameter(ScriptState*, VRWebGLShader*, GLenum pname);
    String getShaderInfoLog(VRWebGLShader*);
    VRWebGLShaderPrecisionFormat* getShaderPrecisionFormat(GLenum shaderType, GLenum precisionType);
    VRWebGLUniformLocation* getUniformLocation(VRWebGLProgram*, const String&);
    void lineWidth(GLfloat width);
    void linkProgram(VRWebGLProgram*);
    void pixelStorei(GLenum pname, GLint param);
    void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, DOMArrayBufferView* pixels);
    void renderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void scissor(GLint x, GLint y, GLsizei width, GLsizei height);
    void shaderSource(VRWebGLShader*, const String&);
    void texParameterf(GLenum target, GLenum pname, GLfloat param);
    void texParameteri(GLenum target, GLenum pname, GLint param);
    void texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, DOMArrayBufferView* pixels);
    void texImage2D(GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, ImageData* pixels);
    void texImage2D(GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, HTMLImageElement* image, ExceptionState& exceptionState);
    void texImage2D(GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, HTMLCanvasElement* canvas, ExceptionState& exceptionState);
    void texImage2D(GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, HTMLVideoElement* video, ExceptionState& exceptionState);
    void texImage2D(GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, VRWebGLVideo* video, ExceptionState& exceptionState);
    void texImage2D(GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, VRWebGLWebView* webview, ExceptionState& exceptionState);
    void texImage2D(GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, ImageBitmap* bitmap, ExceptionState& exceptionState);
    void uniform1f(const VRWebGLUniformLocation* location, GLfloat x);
    void uniform1fv(const VRWebGLUniformLocation* location, const FlexibleFloat32ArrayView& v);
    void uniform1fv(const VRWebGLUniformLocation* location, Vector<GLfloat>& v);
    void uniform1i(const VRWebGLUniformLocation* location, GLint x);
    void uniform1iv(const VRWebGLUniformLocation* location, const FlexibleInt32ArrayView& v);
    void uniform1iv(const VRWebGLUniformLocation* location, Vector<GLint>& v);
    void uniform2f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y);
    void uniform2fv(const VRWebGLUniformLocation* location, const FlexibleFloat32ArrayView& v);
    void uniform2fv(const VRWebGLUniformLocation* location, Vector<GLfloat>& v);
    void uniform2i(const VRWebGLUniformLocation* location, GLint x, GLint y);
    void uniform2iv(const VRWebGLUniformLocation* location, const FlexibleInt32ArrayView& v);
    void uniform2iv(const VRWebGLUniformLocation* location, Vector<GLint>& v);
    void uniform3f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z);
    void uniform3fv(const VRWebGLUniformLocation* location, const FlexibleFloat32ArrayView& v);
    void uniform3fv(const VRWebGLUniformLocation* location, Vector<GLfloat>& v);
    void uniform3i(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z);
    void uniform3iv(const VRWebGLUniformLocation* location, const FlexibleInt32ArrayView& v);
    void uniform3iv(const VRWebGLUniformLocation* location, Vector<GLint>& v);
    void uniform4f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void uniform4fv(const VRWebGLUniformLocation* location, const FlexibleFloat32ArrayView& v);
    void uniform4fv(const VRWebGLUniformLocation* location, Vector<GLfloat>& v);
    void uniform4i(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z, GLint w);
    void uniform4iv(const VRWebGLUniformLocation* location, const FlexibleInt32ArrayView& v);
    void uniform4iv(const VRWebGLUniformLocation* location, Vector<GLint>& v);
    void uniformMatrix2fv(const VRWebGLUniformLocation*, GLboolean transpose, DOMFloat32Array* value);
    void uniformMatrix2fv(const VRWebGLUniformLocation*, GLboolean transpose, Vector<GLfloat>& value);
    void uniformMatrix3fv(const VRWebGLUniformLocation*, GLboolean transpose, DOMFloat32Array* value);
    void uniformMatrix3fv(const VRWebGLUniformLocation*, GLboolean transpose, Vector<GLfloat>& value);
    void uniformMatrix4fv(const VRWebGLUniformLocation*, GLboolean transpose, DOMFloat32Array* value);
    void uniformMatrix4fv(const VRWebGLUniformLocation*, GLboolean transpose, Vector<GLfloat>& value);
    void useProgram(ScriptState*, VRWebGLProgram*);
    void vertexAttribPointer(ScriptState*, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, long long offset);
    void viewport(GLint x, GLint y, GLsizei width, GLsizei height);

    void startFrame();
    void endFrame();
    Vector<GLfloat> getModelViewMatrix();
    VRPose* getPose();
    VREyeParameters* getEyeParameters(const String& eye);
    void setCameraWorldMatrix(DOMFloat32Array* value);
    void setCameraWorldMatrix(Vector<GLfloat>& value);
    void setCameraProjectionMatrix(DOMFloat32Array* value);
    void setCameraProjectionMatrix(Vector<GLfloat>& value);
    void setRenderEnabled(bool flag);
    Gamepad* getGamepad();

    DECLARE_VIRTUAL_TRACE();

    bool destroyed() const { return m_destroyed; }

private:

    enum TexImageFunctionType {
        TexImage,
        TexSubImage,
        CopyTexImage,
        CompressedTexImage
    };
    enum TexImageFunctionID {
        TexImage2D,
        TexSubImage2D,
        TexImage3D,
        TexSubImage3D
    };
    enum TexImageDimension {
        Tex2D,
        Tex3D
    };

    class LRUImageBufferCache {
    public:
        LRUImageBufferCache(int capacity);
        // The pointer returned is owned by the image buffer map.
        ImageBuffer* imageBuffer(const IntSize&);
    private:
        void bubbleToFront(int idx);
        std::unique_ptr<std::unique_ptr<ImageBuffer>[]> m_buffers;
        int m_capacity;
    };

	// VRWebGLRenderingContext(HTMLCanvasElement*);
    VRWebGLRenderingContext();
    void resetUnpackParameters();
    void restoreUnpackParameters();
    void texImage2DBase(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
    // void texImage2DImpl(GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, Image* image, WebGLImageConversion::ImageHtmlDomSource domSource, bool flipY, bool premultiplyAlpha);
    void texImageImpl(TexImageFunctionID functionID, GLenum target, GLint level, GLint internalformat, GLint xoffset, GLint yoffset, GLint zoffset, GLenum format, GLenum type, Image* image, WebGLImageConversion::ImageHtmlDomSource domSource, bool flipY, bool premultiplyAlpha, const IntRect& sourceImageRect, GLsizei depth, GLint unpackImageHeight);
    PassRefPtr<Image> drawImageIntoBuffer(PassRefPtr<Image> passImage, int width, int height, const char* functionName);
    GLenum convertTexInternalFormat(GLenum internalformat, GLenum type);
    void texParameter(GLenum target, GLenum pname, GLfloat paramf, GLint parami, bool isFloat);
    ScriptValue getBooleanParameter(ScriptState* scriptState, GLenum pname);
    ScriptValue getBooleanArrayParameter(ScriptState* scriptState, GLenum pname);
    ScriptValue getFloatParameter(ScriptState* scriptState, GLenum pname);
    ScriptValue getIntParameter(ScriptState* scriptState, GLenum pname);
    // ScriptValue getInt64Parameter(ScriptState* scriptState, GLenum pname);
    ScriptValue getUnsignedIntParameter(ScriptState* scriptState, GLenum pname);
    ScriptValue getWebGLFloatArrayParameter(ScriptState* scriptState, GLenum pname);
    ScriptValue getWebGLIntArrayParameter(ScriptState* scriptState, GLenum pname);
    static bool isPrefixReserved(const String& name);
    const char* getTexImageFunctionName(TexImageFunctionID funcName);
    IntRect sentinelEmptyRect();
    IntRect safeGetImageSize(Image* image);
    IntRect getImageDataSize(ImageData* pixels);
    PassRefPtr<Image> videoFrameToImage(HTMLVideoElement* video);
    void texImageHelperHTMLVideoElement(TexImageFunctionID functionID,
        GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, GLint xoffset,
        GLint yoffset, GLint zoffset, HTMLVideoElement* video, const IntRect& sourceImageRect,
        GLsizei depth, GLint unpackImageHeight, ExceptionState& exceptionState);
    void texImageHelperDOMArrayBufferView(TexImageFunctionID functionID,
        GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border,
        GLenum format, GLenum type, GLsizei depth, GLint xoffset, GLint yoffset, GLint zoffset, DOMArrayBufferView* pixels);
    void texImageHelperImageData(TexImageFunctionID functionID,
        GLenum target, GLint level, GLint internalformat, GLint border, GLenum format,
        GLenum type, GLsizei depth, GLint xoffset, GLint yoffset, GLint zoffset, ImageData* pixels, const IntRect& sourceImageRect, GLint unpackImageHeight);
    void texImageHelperHTMLImageElement(TexImageFunctionID functionID,
        GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, GLint xoffset,
        GLint yoffset, GLint zoffset, HTMLImageElement* image, const IntRect& sourceImageRect,
        GLsizei depth, GLint unpackImageHeight, ExceptionState& exceptionState);

    template <typename T>
    IntRect getTextureSourceSize(T* textureSource) {
        return IntRect(0, 0, textureSource->width(), textureSource->height());
    }

    template <typename T>
    bool validateTexImageSubRectangle(
        const char* functionName, TexImageFunctionID functionID, T* image, 
        const IntRect& subRect, GLsizei depth, GLint unpackImageHeight,
        bool* selectingSubRectangle) 
    {
        // DCHECK(functionName);
        // DCHECK(selectingSubRectangle);
        // DCHECK(image);
        // int imageWidth = static_cast<int>(image->width());
        // int imageHeight = static_cast<int>(image->height());
        // *selectingSubRectangle =
        //     !(subRect.x() == 0 && subRect.y() == 0 &&
        //     subRect.width() == imageWidth && subRect.height() == imageHeight);
        // // If the source image rect selects anything except the entire
        // // contents of the image, assert that we're running WebGL 2.0 or
        // // higher, since this should never happen for WebGL 1.0 (even though
        // // the code could support it). If the image is null, that will be
        // // signaled as an error later.
        // DCHECK(!*selectingSubRectangle || isWebGL2OrHigher())
        //     << "subRect = (" << subRect.width() << " x " << subRect.height()
        //     << ") @ (" << subRect.x() << ", " << subRect.y() << "), image = ("
        //     << imageWidth << " x " << imageHeight << ")";

        // if (subRect.x() < 0 || subRect.y() < 0 || subRect.maxX() > imageWidth ||
        //         subRect.maxY() > imageHeight || subRect.width() < 0 ||
        //         subRect.height() < 0) {
        //     synthesizeGLError(GL_INVALID_OPERATION, functionName,
        //     "source sub-rectangle specified via pixel unpack "
        //     "parameters is invalid");
        //     return false;
        // }

        // if (functionID == TexImage3D || functionID == TexSubImage3D) {
        //     DCHECK_GE(unpackImageHeight, 0);

        //     if (depth < 1) {
        //     synthesizeGLError(GL_INVALID_OPERATION, functionName,
        //     "Can't define a 3D texture with depth < 1");
        //     return false;
        // }

        // // According to the WebGL 2.0 spec, specifying depth > 1 means
        // // to select multiple rectangles stacked vertically.
        // WTF::CheckedNumeric<GLint> maxYAccessed;
        // if (unpackImageHeight) {
        //     maxYAccessed = unpackImageHeight;
        // } else {
        //     maxYAccessed = subRect.height();
        // }
        // maxYAccessed *= depth - 1;
        // maxYAccessed += subRect.height();
        // maxYAccessed += subRect.y();

        // if (!maxYAccessed.IsValid()) {
        //     synthesizeGLError(GL_INVALID_OPERATION, functionName,
        //     "Out-of-range parameters passed for 3D texture "
        //     "upload");
        //     return false;
        // }

        // if (maxYAccessed.ValueOrDie() > imageHeight) {
        //     synthesizeGLError(GL_INVALID_OPERATION, functionName,
        //     "Not enough data supplied to upload to a 3D texture "
        //     "with depth > 1");
        //     return false;
        // }
        // } else {
        //     DCHECK_EQ(depth, 1);
        //     DCHECK_EQ(unpackImageHeight, 0);
        // }
        return true;
    }

	HashSet<UntracedMember<VRWebGLObject>> m_vrWebGLObjects;
	// Member<HTMLCanvasElement> m_canvas;

#ifdef VRWEBGL_SIMULATE_OPENGL_THREAD   
    pthread_t m_openGLThread;
#endif

    bool m_destroyed = false;

    GLint m_unpackFlipY;
    GLint m_unpackPremultiplyAlpha;
    GLint m_packAlignment;
    GLint m_unpackAlignment;
    GLenum m_unpackColorspaceConversion;
    LRUImageBufferCache m_generatedImageCache;   
    Member<VRWebGLProgram> m_programCurrentlyInUse;
    Member<VRWebGLFramebuffer> m_framebufferCurrentlyBound;
    Member<VRWebGLTexture> m_textureCurrentlyBound;
    Vector<String> m_supportedExtensionNames;

    Vector<GLfloat> m_modelViewMatrix;
    VRWebGLPose m_vrWebGLPose;
    VRWebGLEyeParameters m_vrWebGLEyeParameters;
    Member<VRPose> m_vrPose;
    Member<VREyeParameters> m_vrEyeParametersLeft;
    Member<VREyeParameters> m_vrEyeParametersRight;
    Member<Gamepad> m_gamepad;

    Member<VRWebGLANGLEInstancedArrays> m_angleInstancedArraysExtension;
};

} // namespace blink

#endif // VRWebGLRenderingContext_h
