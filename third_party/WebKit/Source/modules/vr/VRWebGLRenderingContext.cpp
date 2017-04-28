#include "modules/vr/VRWebGLRenderingContext.h"
#include "modules/vr/VRWebGLShader.h"
#include "modules/vr/VRWebGLTexture.h"
#include "modules/vr/VRWebGLProgram.h"
#include "modules/vr/VRWebGLBuffer.h"
#include "modules/vr/VRWebGLUniformLocation.h"
#include "modules/vr/VRWebGLShaderPrecisionFormat.h"
#include "modules/vr/VRWebGLActiveInfo.h"
#include "modules/vr/VRWebGLFramebuffer.h"
#include "modules/vr/VRWebGLRenderbuffer.h"
#include "modules/vr/VRWebGLVideo.h"
#include "modules/vr/VRWebGLWebView.h"

#include "modules/vr/VRWebGLANGLEInstancedArrays.h"

#include "modules/vr/VRWebGLCommand.h"
#include "modules/vr/VRWebGLCommandProcessor.h"
#include "modules/vr/VRWebGL.h"

#include "bindings/modules/v8/WebGLAny.h"

#include "platform/graphics/GraphicsTypes3D.h"

#include "wtf/text/StringUTF8Adaptor.h"

#include "core/html/HTMLCanvasElement.h"
#include "core/html/HTMLImageElement.h"
#include "core/html/HTMLVideoElement.h"
#include "core/html/ImageData.h"
#include "core/frame/ImageBitmap.h"

#include "modules/vr/VRPose.h"
#include "modules/vr/VREyeParameters.h"

#include "modules/gamepad/Gamepad.h"

namespace blink 
{

VRWebGLRenderingContext::LRUImageBufferCache::LRUImageBufferCache(int capacity)
    :m_buffers(wrapArrayUnique(new std::unique_ptr<ImageBuffer>[capacity]))
    , m_capacity(capacity)
{
}

ImageBuffer* VRWebGLRenderingContext::LRUImageBufferCache::imageBuffer(const IntSize& size)
{
    int i;
    for (i = 0; i < m_capacity; ++i) {
        ImageBuffer* buf = m_buffers[i].get();
        if (!buf)
            break;
        if (buf->size() != size)
            continue;
        bubbleToFront(i);
        return buf;
    }

    std::unique_ptr<ImageBuffer> temp(ImageBuffer::create(size));
    if (!temp)
        return nullptr;
    i = std::min(m_capacity - 1, i);
    m_buffers[i] = std::move(temp);

    ImageBuffer* buf = m_buffers[i].get();
    bubbleToFront(i);
    return buf;
}

void VRWebGLRenderingContext::LRUImageBufferCache::bubbleToFront(int idx)
{
    for (int i = idx; i > 0; --i)
        m_buffers[i].swap(m_buffers[i-1]);
}

// Strips comments from shader text. This allows non-ASCII characters
// to be used in comments without potentially breaking OpenGL
// implementations not expecting characters outside the GLSL ES set.
class StripComments {
public:
    StripComments(const String& str)
        : m_parseState(BeginningOfLine)
        , m_sourceString(str)
        , m_length(str.length())
        , m_position(0)
    {
        parse();
    }

    String result()
    {
        return m_builder.toString();
    }

private:
    bool hasMoreCharacters() const
    {
        return (m_position < m_length);
    }

    void parse()
    {
        while (hasMoreCharacters()) {
            process(current());
            // process() might advance the position.
            if (hasMoreCharacters())
                advance();
        }
    }

    void process(UChar);

    bool peek(UChar& character) const
    {
        if (m_position + 1 >= m_length)
            return false;
        character = m_sourceString[m_position + 1];
        return true;
    }

    UChar current()
    {
        ASSERT(m_position < m_length);
        return m_sourceString[m_position];
    }

    void advance()
    {
        ++m_position;
    }

    static bool isNewline(UChar character)
    {
        // Don't attempt to canonicalize newline related characters.
        return (character == '\n' || character == '\r');
    }

    void emit(UChar character)
    {
        m_builder.append(character);
    }

    enum ParseState {
        // Have not seen an ASCII non-whitespace character yet on
        // this line. Possible that we might see a preprocessor
        // directive.
        BeginningOfLine,

        // Have seen at least one ASCII non-whitespace character
        // on this line.
        MiddleOfLine,

        // Handling a preprocessor directive. Passes through all
        // characters up to the end of the line. Disables comment
        // processing.
        InPreprocessorDirective,

        // Handling a single-line comment. The comment text is
        // replaced with a single space.
        InSingleLineComment,

        // Handling a multi-line comment. Newlines are passed
        // through to preserve line numbers.
        InMultiLineComment
    };

    ParseState m_parseState;
    String m_sourceString;
    unsigned m_length;
    unsigned m_position;
    StringBuilder m_builder;
};

void StripComments::process(UChar c)
{
    if (isNewline(c)) {
        // No matter what state we are in, pass through newlines
        // so we preserve line numbers.
        emit(c);

        if (m_parseState != InMultiLineComment)
            m_parseState = BeginningOfLine;

        return;
    }

    UChar temp = 0;
    switch (m_parseState) {
    case BeginningOfLine:
        if (WTF::isASCIISpace(c)) {
            emit(c);
            break;
        }

        if (c == '#') {
            m_parseState = InPreprocessorDirective;
            emit(c);
            break;
        }

        // Transition to normal state and re-handle character.
        m_parseState = MiddleOfLine;
        process(c);
        break;

    case MiddleOfLine:
        if (c == '/' && peek(temp)) {
            if (temp == '/') {
                m_parseState = InSingleLineComment;
                emit(' ');
                advance();
                break;
            }

            if (temp == '*') {
                m_parseState = InMultiLineComment;
                // Emit the comment start in case the user has
                // an unclosed comment and we want to later
                // signal an error.
                emit('/');
                emit('*');
                advance();
                break;
            }
        }

        emit(c);
        break;

    case InPreprocessorDirective:
        // No matter what the character is, just pass it
        // through. Do not parse comments in this state. This
        // might not be the right thing to do long term, but it
        // should handle the #error preprocessor directive.
        emit(c);
        break;

    case InSingleLineComment:
        // The newline code at the top of this function takes care
        // of resetting our state when we get out of the
        // single-line comment. Swallow all other characters.
        break;

    case InMultiLineComment:
        if (c == '*' && peek(temp) && temp == '/') {
            emit('*');
            emit('/');
            m_parseState = MiddleOfLine;
            advance();
            break;
        }

        // Swallow all other characters. Unclear whether we may
        // want or need to just emit a space per character to try
        // to preserve column numbers for debugging purposes.
        break;
    }
}

#ifdef VRWEBGL_SIMULATE_OPENGL_THREAD	

void* openGLThreadFunction( void * param )
{
	VRWebGLRenderingContext* vrWebGLRenderingContext = (VRWebGLRenderingContext*)param;
    VRWebGLCommandProcessor* vrWebGLCommandProcessor = VRWebGLCommandProcessor::getInstance();

	vrWebGLCommandProcessor->setCurrentThreadName("GL");

	VLOG(0) << "VRWebGL: openGL thread started!";

    while(!vrWebGLRenderingContext->destroyed())
    {
        vrWebGLCommandProcessor->update();
        usleep( 8000 );
        vrWebGLCommandProcessor->renderFrame();
        usleep( 8000 );
    }

	VLOG(0) << "VRWebGL: openGL thread finished!";

    return 0;
}

#endif // VRWEBGL_SIMULATE_OPENGL_THREAD

VRWebGLRenderingContext* VRWebGLRenderingContext::create()
{
	VLOG(0) << "VRWebGL: VRWebGLRenderingContext::create begin";
	VRWebGLCommandProcessor::getInstance()->setCurrentThreadName("JS");

	VRWebGLRenderingContext* instance = new VRWebGLRenderingContext();

#ifdef VRWEBGL_SIMULATE_OPENGL_THREAD	
    const int createErr = pthread_create(&(instance->m_openGLThread), NULL, openGLThreadFunction, instance);
    if ( createErr != 0 )
    {
        VLOG(0) << "VRWebGL: pthread_create returned " << createErr << " while creating simulated openGL thread." << std::endl;
    }
#endif    

	VLOG(0) << "VRWebGL: VRWebGLRenderingContext::create end";
	return instance;
}

VRWebGLRenderingContext::~VRWebGLRenderingContext()
{
	VLOG(0) << "VRWebGL: VRWebGLRenderingContext::~VRWebGLRenderingContext begin";

	m_destroyed = true;

#ifdef VRWEBGL_SIMULATE_OPENGL_THREAD	
	pthread_join( m_openGLThread, NULL );
#endif

	VLOG(0) << "VRWebGL: VRWebGLRenderingContext::~VRWebGLRenderingContext end";
}

void VRWebGLRenderingContext::activeTexture(GLenum texture)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::activeTexture begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_activeTexture::newInstance(texture);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::activeTexture end";
}

void VRWebGLRenderingContext::attachShader(ScriptState* scriptState, VRWebGLProgram* program, VRWebGLShader* shader)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::attachShader begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_attachShader::newInstance(program, shader);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::attachShader end";
}

void VRWebGLRenderingContext::bindAttribLocation(VRWebGLProgram* program, GLuint index, const String& name)
{
    if (isPrefixReserved(name)) {
        VLOG(0) << "VRWebGL: ERROR: bindAttribLocation, reserved prefix";
        return;
    }
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bindAttribLocation begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_bindAttribLocation::newInstance(program, index, name.utf8().data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bindAttribLocation end";
}

void VRWebGLRenderingContext::bindBuffer(ScriptState* scriptState, GLenum target, VRWebGLBuffer* buffer)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bindBuffer begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_bindBuffer::newInstance(target, buffer);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bindBuffer end";
}

void VRWebGLRenderingContext::bindFramebuffer(ScriptState* scriptState, GLenum target, VRWebGLFramebuffer* buffer)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bindFramebuffer begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_bindFramebuffer::newInstance(target, buffer);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bindFramebuffer end";
	m_framebufferCurrentlyBound = buffer;
}

void VRWebGLRenderingContext::bindRenderbuffer(ScriptState* scriptState, GLenum target, VRWebGLRenderbuffer* buffer)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bindRenderbuffer begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_bindRenderbuffer::newInstance(target, buffer);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bindRenderbuffer end";
}

void VRWebGLRenderingContext::bindTexture(ScriptState* scriptState, GLenum target, VRWebGLTexture* texture)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bindTexture begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_bindTexture::newInstance(target, texture);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bindTexture end";	
    m_textureCurrentlyBound = texture;
}
    
void VRWebGLRenderingContext::blendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::blendColor begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_blendColor::newInstance(red, green, blue, alpha);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::blendColor end";
}

void VRWebGLRenderingContext::blendEquation(GLenum mode)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::blendEquation begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_blendEquation::newInstance(mode);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::blendEquation end";	
}

void VRWebGLRenderingContext::blendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::blendEquationSeparate begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_blendEquationSeparate::newInstance(modeRGB, modeAlpha);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::blendEquationSeparate end";	
}

void VRWebGLRenderingContext::blendFunc(GLenum sfactor, GLenum dfactor)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::blendFunc begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_blendFunc::newInstance(sfactor, dfactor);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::blendFunc end";	
}

void VRWebGLRenderingContext::blendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::blendFuncSeparate begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_blendFuncSeparate::newInstance(srcRGB, dstRGB, srcAlpha, dstAlpha);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::blendFuncSeparate end";	
}

void VRWebGLRenderingContext::bufferData(GLenum target, long long size, GLenum usage)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bufferData 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_bufferData::newInstance(target, size, 0, usage);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bufferData 1 end";
}

void VRWebGLRenderingContext::bufferData(GLenum target, DOMArrayBuffer* data, GLenum usage)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bufferData 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_bufferData::newInstance(target, data->byteLength(), data->data(), usage);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bufferData 2 end";
}

void VRWebGLRenderingContext::bufferData(GLenum target, DOMArrayBufferView* data, GLenum usage)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bufferData 3 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_bufferData::newInstance(target, data->byteLength(), data->baseAddress(), usage);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bufferData 3 end";
}

void VRWebGLRenderingContext::bufferSubData(GLenum target, long long offset, DOMArrayBuffer* data)
{
   if (!data) {
        VLOG(0) << "VRWebGL: ERROR: bufferSubData, no data";
        return;
    }
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bufferSubData 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_bufferSubData::newInstance(target, static_cast<GLintptr>(offset), data->byteLength(), data->data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bufferSubData 1 end";
}

void VRWebGLRenderingContext::bufferSubData(GLenum target, long long offset, const FlexibleArrayBufferView& data)
{
   if (!data) {
        VLOG(0) << "VRWebGL: ERROR: bufferSubData, no data";
        return;
    }
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bufferSubData 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_bufferSubData::newInstance(target, static_cast<GLintptr>(offset), data.byteLength(), data.baseAddressMaybeOnStack());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::bufferSubData 2 end";
}

GLenum VRWebGLRenderingContext::checkFramebufferStatus(GLenum target)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::checkFramebufferStatus begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_checkFramebufferStatus::newInstance(target);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::checkFramebufferStatus end";
	return *((GLenum*)result);
}

void VRWebGLRenderingContext::clear(GLbitfield mask)
{
	// if (m_framebufferCurrentlyBound != 0 || ((mask & GL_COLOR_BUFFER_BIT) == 0))
	{
		// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::clear begin";
		std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_clear::newInstance(mask);
		// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
		VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
		// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::clear end";
	}
}

void VRWebGLRenderingContext::clearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::clearColor begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_clearColor::newInstance(red, green, blue, alpha);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::clearColor end";
}

void VRWebGLRenderingContext::clearDepth(GLfloat depth)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::clearDepth begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_clearDepthf::newInstance(depth);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::clearDepth end";
}

void VRWebGLRenderingContext::clearStencil(GLint s)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::clearStencil begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_clearStencil::newInstance(s);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::clearStencil end";
}

void VRWebGLRenderingContext::colorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::colorMask begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_colorMask::newInstance(red, green, blue, alpha);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::colorMask end";
}

void VRWebGLRenderingContext::compileShader(VRWebGLShader* shader)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::compileShader begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_compileShader::newInstance(shader);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::compileShader end";
}

VRWebGLBuffer* VRWebGLRenderingContext::createBuffer()
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createBuffer begin";
	VRWebGLBuffer* vrWebGLBuffer = new VRWebGLBuffer();
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_createBuffer::newInstance(vrWebGLBuffer);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createBuffer end";

	m_vrWebGLObjects.add(vrWebGLBuffer);
	
	return vrWebGLBuffer;
}

VRWebGLFramebuffer* VRWebGLRenderingContext::createFramebuffer()
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createFramebuffer begin";
	VRWebGLFramebuffer* vrWebGLFramebuffer = new VRWebGLFramebuffer();
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_createFramebuffer::newInstance(vrWebGLFramebuffer);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createFramebuffer end";

	m_vrWebGLObjects.add(vrWebGLFramebuffer);
	
	return vrWebGLFramebuffer;
}

VRWebGLProgram* VRWebGLRenderingContext::createProgram()
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createProgram begin";
	VRWebGLProgram* vrWebGLProgram = new VRWebGLProgram();
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_createProgram::newInstance(vrWebGLProgram);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createProgram end";

	m_vrWebGLObjects.add(vrWebGLProgram);
	
	return vrWebGLProgram;
}

VRWebGLRenderbuffer* VRWebGLRenderingContext::createRenderbuffer()
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createRenderbuffer begin";
	VRWebGLRenderbuffer* vrWebGLRenderbuffer = new VRWebGLRenderbuffer();
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_createRenderbuffer::newInstance(vrWebGLRenderbuffer);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createRenderbuffer end";

	m_vrWebGLObjects.add(vrWebGLRenderbuffer);
	
	return vrWebGLRenderbuffer;
}

VRWebGLShader* VRWebGLRenderingContext::createShader(GLenum type)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createShader begin";
	VRWebGLShader* vrWebGLShader = new VRWebGLShader(type);
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_createShader::newInstance(vrWebGLShader, type);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createShader end";

	m_vrWebGLObjects.add(vrWebGLShader);

	return vrWebGLShader;
}

VRWebGLTexture* VRWebGLRenderingContext::createTexture()
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createTexture begin";
	VRWebGLTexture* vrWebGLTexture = new VRWebGLTexture();
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_createTexture::newInstance(vrWebGLTexture);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::createTexture end";

	m_vrWebGLObjects.add(vrWebGLTexture);
	
	return vrWebGLTexture;
}

void VRWebGLRenderingContext::cullFace(GLenum mode)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::cullFace begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_cullFace::newInstance(mode);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::cullFace end";	
}

void VRWebGLRenderingContext::deleteBuffer(VRWebGLBuffer* buffer)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteBuffer begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_deleteBuffer::newInstance(buffer);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteBuffer end";
}

void VRWebGLRenderingContext::deleteFramebuffer(VRWebGLFramebuffer* framebuffer)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteFramebuffer begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_deleteFramebuffer::newInstance(framebuffer);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteFramebuffer end";
}

void VRWebGLRenderingContext::deleteProgram(VRWebGLProgram* program)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteProgram begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_deleteProgram::newInstance(program);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteProgram end";
    // TODO: Not ideal as there could be a race condition (VRWebGLCommand_deleteProgram is not synchronous)
    program->markAsDeleted();
}

void VRWebGLRenderingContext::deleteRenderbuffer(VRWebGLRenderbuffer* renderbuffer)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteRenderbuffer begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_deleteRenderbuffer::newInstance(renderbuffer);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteRenderbuffer end";
}

void VRWebGLRenderingContext::deleteShader(VRWebGLShader* shader)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteShader begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_deleteShader::newInstance(shader);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteShader end";
    // TODO: Not ideal as there could be a race condition (VRWebGLCommand_deleteShader is not synchronous)
    shader->markAsDeleted();
}

void VRWebGLRenderingContext::deleteTexture(VRWebGLTexture* texture)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteTexture begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_deleteTexture::newInstance(texture);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::deleteTexture end";
}

void VRWebGLRenderingContext::depthFunc(GLenum func)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::depthFunc begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_depthFunc::newInstance(func);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::depthFunc end";	
}

void VRWebGLRenderingContext::depthMask(GLboolean flag)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::depthMask begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_depthMask::newInstance(flag);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::depthMask end";	
}

void VRWebGLRenderingContext::depthRange(GLfloat zNear, GLfloat zFar)
{
    // Check required by WebGL spec section 6.12
    if (zNear > zFar) {
        VLOG(0) << "VRWebGL: ERROR: depthRange, zNear > zFar";
        return;
    }
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::depthRange begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_depthRangef::newInstance(zNear, zFar);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::depthRange end";	
}

void VRWebGLRenderingContext::detachShader(ScriptState* scriptState, VRWebGLProgram* program, VRWebGLShader* shader)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::detachShader begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_detachShader::newInstance(program, shader);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::detachShader end";
}

void VRWebGLRenderingContext::disable(GLenum cap)
{
	// TODO
    // if (cap == GL_STENCIL_TEST) {
    //     m_stencilEnabled = false;
    //     applyStencilTest();
    //     return;
    // }
    // if (cap == GL_SCISSOR_TEST) {
    //     m_scissorEnabled = false;
    //     drawingBuffer()->setScissorEnabled(m_scissorEnabled);
    // }
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::disable begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_disable::newInstance(cap);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::disable end";	
}


void VRWebGLRenderingContext::disableVertexAttribArray(GLuint index)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::disableVertexAttribArray begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_disableVertexAttribArray::newInstance(index);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::disableVertexAttribArray end";
}

void VRWebGLRenderingContext::drawArrays(GLenum mode, GLint first, GLsizei count)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::drawArrays begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_drawArrays::newInstance(mode, first, count);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::drawArrays end";
}

void VRWebGLRenderingContext::drawElements(GLenum mode, GLsizei count, GLenum type, long long offset)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::drawElements begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_drawElements::newInstance(mode, count, type, offset);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::drawElements end";
}

void VRWebGLRenderingContext::enable(GLenum cap)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::enable begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_enable::newInstance(cap);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::enable end";	
}

void VRWebGLRenderingContext::enableVertexAttribArray(GLuint index)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::enableVertexAttribArray begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_enableVertexAttribArray::newInstance(index);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::enableVertexAttribArray end";
}

void VRWebGLRenderingContext::framebufferRenderbuffer(ScriptState* scriptState, GLenum target, GLenum attachment, GLenum renderbuffertarget, VRWebGLRenderbuffer* buffer)
{
    if (renderbuffertarget != GL_RENDERBUFFER) {
        VLOG(0) << "VRWebGL: ERROR: framebufferRenderbuffer, invalid target";
        return;
    }
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::framebufferRenderbuffer begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_framebufferRenderbuffer::newInstance(target, attachment, renderbuffertarget, buffer);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::framebufferRenderbuffer end";
}

void VRWebGLRenderingContext::framebufferTexture2D(ScriptState* scriptState, GLenum target, GLenum attachment, GLenum textarget, VRWebGLTexture* texture, GLint level)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::framebufferTexture2D begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_framebufferTexture2D::newInstance(target, attachment, textarget, texture, level);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::framebufferTexture2D end";
}

void VRWebGLRenderingContext::frontFace(GLenum mode)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::frontFace begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_frontFace::newInstance(mode);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::frontFace end";	
}

void VRWebGLRenderingContext::generateMipmap(GLenum target)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::generateMipmap begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_generateMipmap::newInstance(target);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::generateMipmap end";
}

VRWebGLActiveInfo* VRWebGLRenderingContext::getActiveAttrib(VRWebGLProgram* program, GLuint index)
{
	// Take advantage that these 2 commands are synchronous.

	// First call the getProgramiv to get the max size among the active attribute names
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getProgramiv begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getProgramiv::newInstance(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getProgramiv end";
    GLint maxNameLength = *((GLint*)result);
    if (maxNameLength < 0)
        return nullptr;
    if (maxNameLength == 0) {
        VLOG(0) << "VRWebGL: ERROR: getActiveAttrib, no active attributes exist";
        return nullptr;
    }
    // Now call getActiveAttrib.
    LChar* namePtr;
    RefPtr<StringImpl> nameImpl = StringImpl::createUninitialized(maxNameLength, namePtr);
    GLsizei length = 0;
    GLint size = -1;
    GLenum type = 0;
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getActiveAttrib begin";
	vrWebGLCommand = VRWebGLCommand_getActiveAttrib::newInstance(program, index, maxNameLength, &length, &size, &type, reinterpret_cast<GLchar*>(namePtr));
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getActiveAttrib end";
    if (size < 0)
        return nullptr;
    return VRWebGLActiveInfo::create(nameImpl->substring(0, length), type, size);
}

VRWebGLActiveInfo* VRWebGLRenderingContext::getActiveUniform(VRWebGLProgram* program, GLuint index)
{
	// Take advantage that these 2 commands are synchronous.
	
	// First call the getProgramiv to get the max size among the active uniform names
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getProgramiv begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getProgramiv::newInstance(program, GL_ACTIVE_UNIFORM_MAX_LENGTH);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getProgramiv end";
    GLint maxNameLength = *((GLint*)result);
    if (maxNameLength < 0)
        return nullptr;
    if (maxNameLength == 0) {
        VLOG(0) << "VRWebGL: ERROR: getActiveUniform, no active uniforms exist";
        return nullptr;
    }
    // Now call getActiveUniform.
    LChar* namePtr;
    RefPtr<StringImpl> nameImpl = StringImpl::createUninitialized(maxNameLength, namePtr);
    GLsizei length = 0;
    GLint size = -1;
    GLenum type = 0;
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getActiveUniform begin";
	vrWebGLCommand = VRWebGLCommand_getActiveUniform::newInstance(program, index, maxNameLength, &length, &size, &type, reinterpret_cast<GLchar*>(namePtr));
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getActiveUniform end";
    if (size < 0)
        return nullptr;
    return VRWebGLActiveInfo::create(nameImpl->substring(0, length), type, size);
}

GLint VRWebGLRenderingContext::getAttribLocation(VRWebGLProgram* program, const String& name)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getAttribLocation begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getAttribLocation::newInstance(program, name.utf8().data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getAttribLocation end";
	return *((GLint*)result);
}

Nullable<Vector<String>> VRWebGLRenderingContext::getSupportedExtensions()
{
     return m_supportedExtensionNames;
}

GLenum VRWebGLRenderingContext::getError()
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getError begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getError::newInstance();
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getError end";
	return *((GLenum*)result);
}

ScriptValue VRWebGLRenderingContext::getExtension(ScriptState* scriptState, const String& name)
{
	// TODO
	bool result = false;
	for (size_t i = 0; !result && i < m_supportedExtensionNames.size(); i++) 
	{
		result = name.contains(m_supportedExtensionNames[i], TextCaseASCIIInsensitive);
	}

    ScriptValue sv = ScriptValue::createNull(scriptState);
    // TODO: Create a proper structure (map) to hold all the supported extensions.
    if (result)
    {
        // TODO: Get rid of this when the proper structure to hold the extensions is in place.
        if (name.contains("ANGLE_instanced_arrays", TextCaseASCIIInsensitive))
        {
            v8::Local<v8::Value> wrappedExtension =
              ToV8(m_angleInstancedArraysExtension, scriptState->context()->Global(), scriptState->isolate());
            sv = ScriptValue(scriptState, wrappedExtension);
        }
    }
    return sv;
}

ScriptValue VRWebGLRenderingContext::getParameter(ScriptState* scriptState, GLenum pname)
{
	// TODO
    // const int intZero = 0;
    switch (pname) {
    case GL_ACTIVE_TEXTURE:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_ALIASED_LINE_WIDTH_RANGE:
        return getWebGLFloatArrayParameter(scriptState, pname);
    case GL_ALIASED_POINT_SIZE_RANGE:
        return getWebGLFloatArrayParameter(scriptState, pname);
    case GL_ALPHA_BITS:
    	// TODO
        // if (m_drawingBuffer->requiresAlphaChannelToBePreserved())
        //     return WebGLAny(scriptState, 0);
        return getIntParameter(scriptState, pname);
    // TODO
    // case GL_ARRAY_BUFFER_BINDING:
    //     return WebGLAny(scriptState, m_boundArrayBuffer.get());
    case GL_BLEND:
        return getBooleanParameter(scriptState, pname);
    case GL_BLEND_COLOR:
        return getWebGLFloatArrayParameter(scriptState, pname);
    case GL_BLEND_DST_ALPHA:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_BLEND_DST_RGB:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_BLEND_EQUATION_ALPHA:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_BLEND_EQUATION_RGB:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_BLEND_SRC_ALPHA:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_BLEND_SRC_RGB:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_BLUE_BITS:
        return getIntParameter(scriptState, pname);
    case GL_COLOR_CLEAR_VALUE:
        return getWebGLFloatArrayParameter(scriptState, pname);
    case GL_COLOR_WRITEMASK:
        return getBooleanArrayParameter(scriptState, pname);
    // TODO
    // case GL_COMPRESSED_TEXTURE_FORMATS:
    //     return WebGLAny(scriptState, DOMUint32Array::create(m_compressedTextureFormats.data(), m_compressedTextureFormats.size()));
    case GL_CULL_FACE:
        return getBooleanParameter(scriptState, pname);
    case GL_CULL_FACE_MODE:
        return getUnsignedIntParameter(scriptState, pname);
    // TODO
    // case GL_CURRENT_PROGRAM:
    //     return WebGLAny(scriptState, m_currentProgram.get());
    case GL_DEPTH_BITS:
    	// TODO
        // if (!m_framebufferBinding && !m_requestedAttributes.depth())
        //     return WebGLAny(scriptState, intZero);
        return getIntParameter(scriptState, pname);
    case GL_DEPTH_CLEAR_VALUE:
        return getFloatParameter(scriptState, pname);
    case GL_DEPTH_FUNC:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_DEPTH_RANGE:
        return getWebGLFloatArrayParameter(scriptState, pname);
    case GL_DEPTH_TEST:
        return getBooleanParameter(scriptState, pname);
    case GL_DEPTH_WRITEMASK:
        return getBooleanParameter(scriptState, pname);
    case GL_DITHER:
        return getBooleanParameter(scriptState, pname);
    // TODO
    // case GL_ELEMENT_ARRAY_BUFFER_BINDING:
    //     return WebGLAny(scriptState, m_boundVertexArrayObject->boundElementArrayBuffer());
    // case GL_FRAMEBUFFER_BINDING:
    //     return WebGLAny(scriptState, m_framebufferBinding.get());
    case GL_FRONT_FACE:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_GENERATE_MIPMAP_HINT:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_GREEN_BITS:
        return getIntParameter(scriptState, pname);
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
        return getIntParameter(scriptState, pname);
    case GL_IMPLEMENTATION_COLOR_READ_TYPE:
        return getIntParameter(scriptState, pname);
    case GL_LINE_WIDTH:
        return getFloatParameter(scriptState, pname);
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
        return getIntParameter(scriptState, pname);
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
        return getIntParameter(scriptState, pname);
    case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
        return getIntParameter(scriptState, pname);
    case GL_MAX_RENDERBUFFER_SIZE:
        return getIntParameter(scriptState, pname);
    case GL_MAX_TEXTURE_IMAGE_UNITS:
        return getIntParameter(scriptState, pname);
    case GL_MAX_TEXTURE_SIZE:
        return getIntParameter(scriptState, pname);
    case GL_MAX_VARYING_VECTORS:
        return getIntParameter(scriptState, pname);
    case GL_MAX_VERTEX_ATTRIBS:
        return getIntParameter(scriptState, pname);
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
        return getIntParameter(scriptState, pname);
    case GL_MAX_VERTEX_UNIFORM_VECTORS:
        return getIntParameter(scriptState, pname);
    case GL_MAX_VIEWPORT_DIMS:
        return getWebGLIntArrayParameter(scriptState, pname);
    case GL_NUM_SHADER_BINARY_FORMATS:
        // FIXME: should we always return 0 for this?
        return getIntParameter(scriptState, pname);
    case GL_PACK_ALIGNMENT:
        return getIntParameter(scriptState, pname);
    case GL_POLYGON_OFFSET_FACTOR:
        return getFloatParameter(scriptState, pname);
    case GL_POLYGON_OFFSET_FILL:
        return getBooleanParameter(scriptState, pname);
    case GL_POLYGON_OFFSET_UNITS:
        return getFloatParameter(scriptState, pname);
    case GL_RED_BITS:
        return getIntParameter(scriptState, pname);
    // TODO
    // case GL_RENDERBUFFER_BINDING:
    //     return WebGLAny(scriptState, m_renderbufferBinding.get());
    case GL_RENDERER:
        return WebGLAny(scriptState, String("WebKit VRWebGL"));
    case GL_SAMPLE_BUFFERS:
        return getIntParameter(scriptState, pname);
    case GL_SAMPLE_COVERAGE_INVERT:
        return getBooleanParameter(scriptState, pname);
    case GL_SAMPLE_COVERAGE_VALUE:
        return getFloatParameter(scriptState, pname);
    case GL_SAMPLES:
        return getIntParameter(scriptState, pname);
    case GL_SCISSOR_BOX:
        return getWebGLIntArrayParameter(scriptState, pname);
    case GL_SCISSOR_TEST:
        return getBooleanParameter(scriptState, pname);
    case GL_SHADING_LANGUAGE_VERSION:
        return WebGLAny(scriptState, String("WebGL GLSL ES 1.0"));
    case GL_STENCIL_BACK_FAIL:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_STENCIL_BACK_FUNC:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_STENCIL_BACK_PASS_DEPTH_PASS:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_STENCIL_BACK_REF:
        return getIntParameter(scriptState, pname);
    case GL_STENCIL_BACK_VALUE_MASK:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_STENCIL_BACK_WRITEMASK:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_STENCIL_BITS:
    	// TODO
        // if (!m_framebufferBinding && !m_requestedAttributes.stencil())
        //     return WebGLAny(scriptState, intZero);
        return getIntParameter(scriptState, pname);
    case GL_STENCIL_CLEAR_VALUE:
        return getIntParameter(scriptState, pname);
    case GL_STENCIL_FAIL:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_STENCIL_FUNC:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_STENCIL_PASS_DEPTH_FAIL:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_STENCIL_PASS_DEPTH_PASS:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_STENCIL_REF:
        return getIntParameter(scriptState, pname);
    case GL_STENCIL_TEST:
        return getBooleanParameter(scriptState, pname);
    case GL_STENCIL_VALUE_MASK:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_STENCIL_WRITEMASK:
        return getUnsignedIntParameter(scriptState, pname);
    case GL_SUBPIXEL_BITS:
        return getIntParameter(scriptState, pname);
    // TODO
    // case GL_TEXTURE_BINDING_2D:
    //     return WebGLAny(scriptState, m_textureUnits[m_activeTextureUnit].m_texture2DBinding.get());
    // case GL_TEXTURE_BINDING_CUBE_MAP:
    //     return WebGLAny(scriptState, m_textureUnits[m_activeTextureUnit].m_textureCubeMapBinding.get());
    case GL_UNPACK_ALIGNMENT:
        return getIntParameter(scriptState, pname);
    case GC3D_UNPACK_FLIP_Y_WEBGL:
        return WebGLAny(scriptState, m_unpackFlipY);
    case GC3D_UNPACK_PREMULTIPLY_ALPHA_WEBGL:
        return WebGLAny(scriptState, m_unpackPremultiplyAlpha);
    case GC3D_UNPACK_COLORSPACE_CONVERSION_WEBGL:
        return WebGLAny(scriptState, m_unpackColorspaceConversion);
    case GL_VENDOR:
        return WebGLAny(scriptState, String("WebKit"));
    case GL_VERSION:
        return WebGLAny(scriptState, String("WebGL 1.0"));
    case GL_VIEWPORT:
        return getWebGLIntArrayParameter(scriptState, pname);
	// TODO
    // case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES: // OES_standard_derivatives
    //     if (extensionEnabled(OESStandardDerivativesName) || isWebGL2OrHigher())
    //         return getUnsignedIntParameter(scriptState, GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES);
    //     synthesizeGLError(GL_INVALID_ENUM, "getParameter", "invalid parameter name, OES_standard_derivatives not enabled");
    //     return ScriptValue::createNull(scriptState);
    // case WebGLDebugRendererInfo::UNMASKED_RENDERER_WEBGL:
    //     if (extensionEnabled(WebGLDebugRendererInfoName))
    //         return WebGLAny(scriptState, String(contextGL()->GetString(GL_RENDERER)));
    //     synthesizeGLError(GL_INVALID_ENUM, "getParameter", "invalid parameter name, WEBGL_debug_renderer_info not enabled");
    //     return ScriptValue::createNull(scriptState);
    // case WebGLDebugRendererInfo::UNMASKED_VENDOR_WEBGL:
    //     if (extensionEnabled(WebGLDebugRendererInfoName))
    //         return WebGLAny(scriptState, String(contextGL()->GetString(GL_VENDOR)));
    //     synthesizeGLError(GL_INVALID_ENUM, "getParameter", "invalid parameter name, WEBGL_debug_renderer_info not enabled");
    //     return ScriptValue::createNull(scriptState);
    // case GL_VERTEX_ARRAY_BINDING_OES: // OES_vertex_array_object
    //     if (extensionEnabled(OESVertexArrayObjectName) || isWebGL2OrHigher()) {
    //         if (!m_boundVertexArrayObject->isDefaultObject())
    //             return WebGLAny(scriptState, m_boundVertexArrayObject.get());
    //         return ScriptValue::createNull(scriptState);
    //     }
    //     synthesizeGLError(GL_INVALID_ENUM, "getParameter", "invalid parameter name, OES_vertex_array_object not enabled");
    //     return ScriptValue::createNull(scriptState);
    // case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT: // EXT_texture_filter_anisotropic
    //     if (extensionEnabled(EXTTextureFilterAnisotropicName))
    //         return getUnsignedIntParameter(scriptState, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT);
    //     synthesizeGLError(GL_INVALID_ENUM, "getParameter", "invalid parameter name, EXT_texture_filter_anisotropic not enabled");
    //     return ScriptValue::createNull(scriptState);
    // case GL_MAX_COLOR_ATTACHMENTS_EXT: // EXT_draw_buffers BEGIN
    //     if (extensionEnabled(WebGLDrawBuffersName) || isWebGL2OrHigher())
    //         return WebGLAny(scriptState, maxColorAttachments());
    //     synthesizeGLError(GL_INVALID_ENUM, "getParameter", "invalid parameter name, WEBGL_draw_buffers not enabled");
    //     return ScriptValue::createNull(scriptState);
    // case GL_MAX_DRAW_BUFFERS_EXT:
    //     if (extensionEnabled(WebGLDrawBuffersName) || isWebGL2OrHigher())
    //         return WebGLAny(scriptState, maxDrawBuffers());
    //     synthesizeGLError(GL_INVALID_ENUM, "getParameter", "invalid parameter name, WEBGL_draw_buffers not enabled");
    //     return ScriptValue::createNull(scriptState);
    // case GL_TIMESTAMP_EXT:
    //     if (extensionEnabled(EXTDisjointTimerQueryName))
    //         return WebGLAny(scriptState, 0);
    //     synthesizeGLError(GL_INVALID_ENUM, "getParameter", "invalid parameter name, EXT_disjoint_timer_query not enabled");
    //     return ScriptValue::createNull(scriptState);
    // case GL_GPU_DISJOINT_EXT:
    //     if (extensionEnabled(EXTDisjointTimerQueryName))
    //         return getBooleanParameter(scriptState, GL_GPU_DISJOINT_EXT);
    //     synthesizeGLError(GL_INVALID_ENUM, "getParameter", "invalid parameter name, EXT_disjoint_timer_query not enabled");
    //     return ScriptValue::createNull(scriptState);

    default:
    	// TODO
        // if ((extensionEnabled(WebGLDrawBuffersName) || isWebGL2OrHigher())
        //     && pname >= GL_DRAW_BUFFER0_EXT
        //     && pname < static_cast<GLenum>(GL_DRAW_BUFFER0_EXT + maxDrawBuffers())) {
        //     GLint value = GL_NONE;
        //     if (m_framebufferBinding)
        //         value = m_framebufferBinding->getDrawBuffer(pname);
        //     else // emulated backbuffer
        //         value = m_backDrawBuffer;
        //     return WebGLAny(scriptState, value);
        // }
        VLOG(0) << "VRWebGL: ERROR: getParameter, invalid parameter name";
        return ScriptValue::createNull(scriptState);
    }
}

ScriptValue VRWebGLRenderingContext::getProgramParameter(ScriptState* scriptState, VRWebGLProgram* program, GLenum pname)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getProgramParameter begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getProgramiv::newInstance(program, pname);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
    GLint value = *((GLint*)result);
    switch (pname) {
    case GL_DELETE_STATUS:
        // return WebGLAny(scriptState, program->isDeleted());
    case GL_VALIDATE_STATUS:
        return WebGLAny(scriptState, static_cast<bool>(value));
    case GL_LINK_STATUS:
    // TODO
        // return WebGLAny(scriptState, program->linkStatus(this));
    // case GL_ACTIVE_UNIFORM_BLOCKS:
    // case GL_TRANSFORM_FEEDBACK_VARYINGS:
    //     if (!isWebGL2OrHigher()) {
    //         synthesizeGLError(GL_INVALID_ENUM, "getProgramParameter", "invalid parameter name");
    //         return ScriptValue::createNull(scriptState);
    //     }
    case GL_ATTACHED_SHADERS:
    case GL_ACTIVE_ATTRIBUTES:
    case GL_ACTIVE_UNIFORMS:
        return WebGLAny(scriptState, value);
    // TODO
    // case GL_TRANSFORM_FEEDBACK_BUFFER_MODE:
    //     if (isWebGL2OrHigher()) {
    //         contextGL()->GetProgramiv(objectOrZero(program), pname, &value);
    //         return WebGLAny(scriptState, static_cast<unsigned>(value));
    //     }
    default:
        return ScriptValue::createNull(scriptState);
    }	
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getProgramParameter end";
	return ScriptValue::createNull(scriptState);
}

String VRWebGLRenderingContext::getProgramInfoLog(VRWebGLProgram* program)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getProgramInfoLog begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getProgramInfoLog::newInstance(program);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	String resultString = (const char*)result;
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getProgramInfoLog end";
	return resultString;
}


ScriptValue VRWebGLRenderingContext::getShaderParameter(ScriptState* scriptState, VRWebGLShader* shader, GLenum pname)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getShaderParameter begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getShaderiv::newInstance(shader, pname);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
    GLint value = *((GLint*)result);
    switch (pname) {
    case GL_DELETE_STATUS:
        // return WebGLAny(scriptState, shader->isDeleted());
    case GL_COMPILE_STATUS:
        return WebGLAny(scriptState, static_cast<bool>(value));
    case GL_SHADER_TYPE:
        return WebGLAny(scriptState, static_cast<unsigned>(value));
    default:
        return ScriptValue::createNull(scriptState);
    }

	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getShaderParameter end";
	return ScriptValue::createNull(scriptState);
}

String VRWebGLRenderingContext::getShaderInfoLog(VRWebGLShader* shader)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getShaderInfoLog begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getShaderInfoLog::newInstance(shader);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	String resultString = (const char*)result;
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getShaderInfoLog end";
	return resultString;
}

VRWebGLShaderPrecisionFormat* VRWebGLRenderingContext::getShaderPrecisionFormat(GLenum shaderType, GLenum precisionType)
{
    switch (shaderType) {
    case GL_VERTEX_SHADER:
    case GL_FRAGMENT_SHADER:
        break;
    default:
        VLOG(0) << "VRWebGL: ERROR: getShaderPrecisionFormat, invalid shader type";
        return nullptr;
    }
    switch (precisionType) {
    case GL_LOW_FLOAT:
    case GL_MEDIUM_FLOAT:
    case GL_HIGH_FLOAT:
    case GL_LOW_INT:
    case GL_MEDIUM_INT:
    case GL_HIGH_INT:
        break;
    default:
        VLOG(0) << "getShaderPrecisionFormat, invalid precision type";
        return nullptr;
    }

	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getShaderPrecisionFormat begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getShaderPrecisionFormat::newInstance(shaderType, precisionType);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	GLint* rangeAndPrecision = (GLint*)result;
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getShaderPrecisionFormat end";

    return VRWebGLShaderPrecisionFormat::create(rangeAndPrecision[0], rangeAndPrecision[1], rangeAndPrecision[2]);
}

VRWebGLUniformLocation* VRWebGLRenderingContext::getUniformLocation(VRWebGLProgram* program, const String& name)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getUniformLocation begin";
	VRWebGLUniformLocation* vrWebGLUniformLocation = new VRWebGLUniformLocation();
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getUniformLocation::newInstance(vrWebGLUniformLocation, program, name.utf8().data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getUniformLocation end";
	return vrWebGLUniformLocation;
}

void VRWebGLRenderingContext::lineWidth(GLfloat width)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::lineWidth begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_lineWidth::newInstance(width);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::lineWidth end";
}

void VRWebGLRenderingContext::linkProgram(VRWebGLProgram* program)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::linkProgram begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_linkProgram::newInstance(program);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::linkProgram end";
}

void VRWebGLRenderingContext::pixelStorei(GLenum pname, GLint param)
{
    switch (pname) {
    case GC3D_UNPACK_FLIP_Y_WEBGL:
        m_unpackFlipY = param;
        break;
    case GC3D_UNPACK_PREMULTIPLY_ALPHA_WEBGL:
        m_unpackPremultiplyAlpha = param;
        break;
    case GC3D_UNPACK_COLORSPACE_CONVERSION_WEBGL:
        if (static_cast<GLenum>(param) == GC3D_BROWSER_DEFAULT_WEBGL || param == GL_NONE) {
            m_unpackColorspaceConversion = static_cast<GLenum>(param);
        }
        break;
    case GL_PACK_ALIGNMENT:
    case GL_UNPACK_ALIGNMENT:
        if (param == 1 || param == 2 || param == 4 || param == 8) {
            if (pname == GL_PACK_ALIGNMENT) {
                m_packAlignment = param;
                // TODO
                // drawingBuffer()->setPackAlignment(param);
            } else { // GL_UNPACK_ALIGNMENT:
                m_unpackAlignment = param;
            }
        }
        break;
    default:
        VLOG(0) << "VRWebGL: ERROR: pixelStorei, invalid parameter name";
        return;
    }
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::pixelStorei begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_pixelStorei::newInstance(pname, param);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::pixelStorei end";
}

void VRWebGLRenderingContext::readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, DOMArrayBufferView* pixels)
{
    // Validate input parameters.
    if (!pixels) {
        VLOG(0) << "VRWebGL: ERROR: readPixels, no destination ArrayBufferView";
        return;
    }

    void* data = pixels->baseAddress();

	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::readPixels begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_readPixels::newInstance(x, y, width, height, format, type, data);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::readPixels end";	
}

void VRWebGLRenderingContext::renderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::renderbufferStorage begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_renderbufferStorage::newInstance(target, internalformat, width, height);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::renderbufferStorage end";	
}

void VRWebGLRenderingContext::scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	if (m_framebufferCurrentlyBound != 0) 
	{
		// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::scissor begin";
		std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_scissor::newInstance(x, y, width, height);
		// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
		VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
		// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::scissor end";
	}
}

void VRWebGLRenderingContext::shaderSource(VRWebGLShader* shader, const String& string)
{
    String stringWithoutComments = StripComments(string).result();
    WTF::StringUTF8Adaptor adaptor(stringWithoutComments);
    const GLchar* shaderData = adaptor.data();
    const GLint shaderLength = adaptor.length();
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::shaderSource begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_shaderSource::newInstance(shader, shaderData, shaderLength);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::shaderSource end";
}

void VRWebGLRenderingContext::texParameterf(GLenum target, GLenum pname, GLfloat param)
{
    texParameter(target, pname, param, 0, true);
}

void VRWebGLRenderingContext::texParameteri(GLenum target, GLenum pname, GLint param)
{
    texParameter(target, pname, 0, param, false);
}

const char* VRWebGLRenderingContext::getTexImageFunctionName(TexImageFunctionID funcName)
{
    switch (funcName) {
    case TexImage2D:
        return "texImage2D";
    case TexSubImage2D:
        return "texSubImage2D";
    case TexSubImage3D:
        return "texSubImage3D";
    case TexImage3D:
        return "texImage3D";
    default: // Adding default to prevent compile error
        return "";
    }
}

IntRect VRWebGLRenderingContext::sentinelEmptyRect() 
{
  // Return a rectangle with -1 width and height so we can recognize
  // it later and recalculate it based on the Image whose data we'll
  // upload. It's important that there be no possible differences in
  // the logic which computes the image's size.
  return IntRect(0, 0, -1, -1);
}

IntRect VRWebGLRenderingContext::safeGetImageSize(Image* image) {
  if (!image)
    return IntRect();

  return getTextureSourceSize(image);
}

IntRect VRWebGLRenderingContext::getImageDataSize(ImageData* pixels) 
{
  DCHECK(pixels);
  return getTextureSourceSize(pixels);
}

void VRWebGLRenderingContext::texImageHelperDOMArrayBufferView(TexImageFunctionID functionID,
    GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border,
    GLenum format, GLenum type, GLsizei depth, GLint xoffset, GLint yoffset, GLint zoffset, DOMArrayBufferView* pixels)
{
    // const char* funcName = getTexImageFunctionName(functionID);
    // TexImageFunctionType functionType;
    // if (functionID == TexImage2D || functionID == TexImage3D)
    //     functionType = TexImage;
    // else
    //     functionType = TexSubImage;
    TexImageDimension sourceType;
    if (functionID == TexImage2D || functionID == TexSubImage2D)
        sourceType = Tex2D;
    else
        sourceType = Tex3D;
    switch (functionID) {
    case TexImage2D:
    case TexImage3D:
        break;
    case TexSubImage2D:
    case TexSubImage3D:
    	break;
    }
    void* data = pixels ? pixels->baseAddress() : 0;
    Vector<uint8_t> tempData;
    bool changeUnpackAlignment = false;
    if (data && (m_unpackFlipY || m_unpackPremultiplyAlpha)) {
        if (sourceType == Tex2D) {
            if (!WebGLImageConversion::extractTextureData(width, height, format, type, m_unpackAlignment, m_unpackFlipY, m_unpackPremultiplyAlpha, data, tempData))
                return;
            data = tempData.data();
        }
        changeUnpackAlignment = true;
    }
    // FIXME: implement flipY and premultiplyAlpha for tex(Sub)3D.
    if (functionID == TexImage3D) {
        // contextGL()->TexImage3D(target, level, convertTexInternalFormat(internalformat, type), width, height, depth, border, format, type, data);
        return;
    }
    if (functionID == TexSubImage3D) {
        // contextGL()->TexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
        return;
    }

    if (changeUnpackAlignment)
    {
        resetUnpackParameters();
    }
    if (functionID == TexImage2D)
    {
        texImage2DBase(target, level, internalformat, width, height, border, format, type, data);
    }
    else if (functionID == TexSubImage2D)
    {
        // contextGL()->TexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, data);
    }
    if (changeUnpackAlignment)
    {
        restoreUnpackParameters();
    }
}

void VRWebGLRenderingContext::texImageHelperImageData(TexImageFunctionID functionID,
    GLenum target, GLint level, GLint internalformat, GLint border, GLenum format,
    GLenum type, GLsizei depth, GLint xoffset, GLint yoffset, GLint zoffset, ImageData* pixels,
    const IntRect& sourceImageRect, GLint unpackImageHeight)
{
    const char* funcName = getTexImageFunctionName(functionID);
    if (!pixels) {
        VLOG(0) << "VRWebGL: ERROR: " << funcName << ", no image data";
        return;
    }
    if (pixels->data()->bufferBase()->isNeutered()) {
        VLOG(0) << "VRWebGL: ERROR: " << funcName << ", source data has been neutered.";
        return;
    }
    // TexImageFunctionType functionType;
    // if (functionID == TexImage2D)
    //     functionType = TexImage;
    // else
    //     functionType = TexSubImage;

    // Adjust the source image rectangle if doing a y-flip.
    IntRect adjustedSourceImageRect = sourceImageRect;
    if (m_unpackFlipY) {
        adjustedSourceImageRect.setY(pixels->height() - adjustedSourceImageRect.maxY());
    }

    Vector<uint8_t> data;
    bool needConversion = true;
    // The data from ImageData is always of format RGBA8.
    // No conversion is needed if destination format is RGBA and type is USIGNED_BYTE and no Flip or Premultiply operation is required.
    if (!m_unpackFlipY && !m_unpackPremultiplyAlpha && format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
        needConversion = false;
    } else {
        if (type == GL_UNSIGNED_INT_10F_11F_11F_REV) {
            // The UNSIGNED_INT_10F_11F_11F_REV type pack/unpack isn't implemented.
            type = GL_FLOAT;
        }

        if (!WebGLImageConversion::extractImageData(
                pixels->data()->data(), WebGLImageConversion::DataFormat::DataFormatRGBA8, 
                pixels->size(), adjustedSourceImageRect, depth, unpackImageHeight, format, 
                type, m_unpackFlipY, m_unpackPremultiplyAlpha, data)) {
	        VLOG(0) << "VRWebGL: ERROR: " << funcName << ", bad image data.";
            return;
        }
    }
    resetUnpackParameters();
    if (functionID == TexImage2D) {
        texImage2DBase(target, level, internalformat, pixels->width(), pixels->height(), border, format, type, needConversion ? data.data() : pixels->data()->data());
    } else if (functionID == TexSubImage2D) {
    	// TODO
        // contextGL()->TexSubImage2D(target, level, xoffset, yoffset, pixels->width(), pixels->height(), format, type, needConversion ? data.data() : pixels->data()->data());
    } else {
        DCHECK_EQ(functionID, TexSubImage3D);
        // TODO
        // contextGL()->TexSubImage3D(target, level, xoffset, yoffset, zoffset, pixels->width(), pixels->height(), depth, format, type, needConversion ? data.data() : pixels->data()->data());
    }
    restoreUnpackParameters();
}

void VRWebGLRenderingContext::texImage2D(GLenum target, GLint level, GLint internalformat,
    GLsizei width, GLsizei height, GLint border,
    GLenum format, GLenum type, DOMArrayBufferView* pixels)
{
    texImageHelperDOMArrayBufferView(TexImage2D, target, level, internalformat, width, height, border, format, type, 1, 0, 0, 0, pixels);
}

void VRWebGLRenderingContext::texImage2D(GLenum target, GLint level, GLint internalformat,
    GLenum format, GLenum type, ImageData* pixels)
{
    texImageHelperImageData(TexImage2D, target, level, internalformat, 0, format, type, 1, 0, 0, 0, pixels, getImageDataSize(pixels), 0);
}

void VRWebGLRenderingContext::texImageHelperHTMLImageElement(TexImageFunctionID functionID,
    GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, GLint xoffset,
    GLint yoffset, GLint zoffset, HTMLImageElement* image, const IntRect& sourceImageRect,
    GLsizei depth, GLint unpackImageHeight, ExceptionState& exceptionState)
{
    const char* funcName = getTexImageFunctionName(functionID);

    if (image->cachedImage() == 0)
    {
        VLOG(0) << "VRWebGL: ERROR: No image in texImage2D call.";
        return;
    }

    RefPtr<Image> imageForRender = image->cachedImage()->getImage();

    if (imageForRender && imageForRender->isSVGImage())
    {
        imageForRender = drawImageIntoBuffer(imageForRender.release(), image->width(), image->height(), funcName);
    }

    // TexImageFunctionType functionType;
    // if (functionID == TexImage2D)
    //     functionType = TexImage;
    // else
    //     functionType = TexSubImage;

    // TODO
    // if (!imageForRender || !validateTexFunc(funcName, functionType, SourceHTMLImageElement, target, level, internalformat, imageForRender->width(), imageForRender->height(), 1, 0, format, type, xoffset, yoffset, zoffset))
    //     return;

    texImageImpl(functionID, target, level, internalformat, xoffset, yoffset, zoffset, format, type, imageForRender.get(), WebGLImageConversion::HtmlDomImage, m_unpackFlipY, m_unpackPremultiplyAlpha, sourceImageRect, depth, unpackImageHeight);
}

void VRWebGLRenderingContext::texImage2D(GLenum target, GLint level, GLint internalformat,
    GLenum format, GLenum type, HTMLImageElement* image, ExceptionState& exceptionState)
{
    texImageHelperHTMLImageElement(TexImage2D, 
        target, level, internalformat, format, type, 0, 
        0, 0, image, sentinelEmptyRect(), 1, 0, exceptionState);
}

void VRWebGLRenderingContext::texImage2D(GLenum target, GLint level, GLint internalformat,
    GLenum format, GLenum type, HTMLCanvasElement* canvas, ExceptionState& exceptionState)
{
	VLOG(0) << "VRWebGL: ERROR: textImage2D with HTMLCanvasElement is still not supported. Sorry :(.";	
	// TODO
    // // texImageCanvasByGPU relies on copyTextureCHROMIUM which doesn't support float/integer/sRGB internal format.
    // // FIXME: relax the constrains if copyTextureCHROMIUM is upgraded to handle more formats.
    // if (!canvas->renderingContext() || !canvas->renderingContext()->isAccelerated() || !canUseTexImageCanvasByGPU(internalformat, type)) {
    //     // 2D canvas has only FrontBuffer.
    //     texImage2DImpl(target, level, internalformat, format, type, canvas->copiedImage(FrontBuffer, PreferAcceleration).get(),
    //         WebGLImageConversion::HtmlDomCanvas, m_unpackFlipY, m_unpackPremultiplyAlpha);
    //     return;
    // }

    // texImage2DBase(target, level, internalformat, canvas->width(), canvas->height(), 0, format, type, 0);
    // texImageCanvasByGPU(TexImage2DByGPU, texture, target, level, internalformat, type, 0, 0, 0, canvas);
}

PassRefPtr<Image> VRWebGLRenderingContext::videoFrameToImage(HTMLVideoElement* video)
{
    IntSize size(video->videoWidth(), video->videoHeight());
    ImageBuffer* buf = m_generatedImageCache.imageBuffer(size);
    if (!buf) {
        VLOG(0) << "VRWebGL: ERROR: textImage2D out of memory. Sorry :(.";  
        return nullptr;
    }
    IntRect destRect(0, 0, size.width(), size.height());
    video->paintCurrentFrame(buf->canvas(), destRect, nullptr);
    return buf->newImageSnapshot();
}

void VRWebGLRenderingContext::texImageHelperHTMLVideoElement(TexImageFunctionID functionID,
    GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, GLint xoffset,
    GLint yoffset, GLint zoffset, HTMLVideoElement* video, const IntRect& sourceImageRect,
    GLsizei depth, GLint unpackImageHeight, ExceptionState& exceptionState)
{
    // const char* funcName = getTexImageFunctionName(functionID);
    // TexImageFunctionType functionType;
    // if (functionID == TexImage2D)
    //     functionType = TexImage;
    // else
    //     functionType = TexSubImage;
    // if (!validateTexFunc(funcName, functionType, SourceHTMLVideoElement, target, level, internalformat, video->videoWidth(), video->videoHeight(), 1, 0, format, type, xoffset, yoffset, zoffset))
    //     return;

    // if (functionID == TexImage2D) {
    //     // Go through the fast path doing a GPU-GPU textures copy without a readback to system memory if possible.
    //     // Otherwise, it will fall back to the normal SW path.
    //     if (GL_TEXTURE_2D == target) {
    //         if (Extensions3DUtil::canUseCopyTextureCHROMIUM(target, internalformat, type, level)
    //             && video->copyVideoTextureToPlatformTexture(contextGL(), texture->object(), internalformat, type, m_unpackPremultiplyAlpha, m_unpackFlipY)) {
    //             return;
    //         }

    //         // Try using an accelerated image buffer, this allows YUV conversion to be done on the GPU.
    //         OwnPtr<ImageBufferSurface> surface = adoptPtr(new AcceleratedImageBufferSurface(IntSize(video->videoWidth(), video->videoHeight())));
    //         if (surface->isValid()) {
    //             OwnPtr<ImageBuffer> imageBuffer(ImageBuffer::create(std::move(surface)));
    //             if (imageBuffer) {
    //                 // The video element paints an RGBA frame into our surface here. By using an AcceleratedImageBufferSurface,
    //                 // we enable the WebMediaPlayer implementation to do any necessary color space conversion on the GPU (though it
    //                 // may still do a CPU conversion and upload the results).
    //                 video->paintCurrentFrame(imageBuffer->canvas(), IntRect(0, 0, video->videoWidth(), video->videoHeight()), nullptr);

    //                 // This is a straight GPU-GPU copy, any necessary color space conversion was handled in the paintCurrentFrameInContext() call.
    //                 if (imageBuffer->copyToPlatformTexture(contextGL(), texture->object(), internalformat, type,
    //                     level, m_unpackPremultiplyAlpha, m_unpackFlipY)) {
    //                     return;
    //                 }
    //             }
    //         }
    //     }
    // }

    RefPtr<Image> image = videoFrameToImage(video);
    if (!image)
        return;
    texImageImpl(functionID, target, level, internalformat, xoffset, yoffset, zoffset, format, type, image.get(), WebGLImageConversion::HtmlDomVideo, m_unpackFlipY, m_unpackPremultiplyAlpha, sourceImageRect, depth, unpackImageHeight);
}

void VRWebGLRenderingContext::texImage2D(GLenum target, GLint level, GLint internalformat,
    GLenum format, GLenum type, HTMLVideoElement* video, ExceptionState& exceptionState)
{
    texImageHelperHTMLVideoElement(TexImage2D, target, level, internalformat, format, type, 0, 0, 0, video, sentinelEmptyRect(), 1, 0, exceptionState);
}

void VRWebGLRenderingContext::texImage2D(GLenum target, GLint level, GLint internalformat,
    GLenum format, GLenum type, VRWebGLVideo* video, ExceptionState& exceptionState)
{
    // Queue the command to update the surface texture
    // VLOG(0) << "VRWebGL: VRWebGLRenderingContext::texImage2D begin";
    std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_updateSurfaceTexture::newInstance(video->textureId());
    // VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
    VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
    // VLOG(0) << "VRWebGL: VRWebGLRenderingContext::texImage2D end";  

    // Assign the external texture id to the current bound texture.
    if (m_textureCurrentlyBound)
    {
        if (m_textureCurrentlyBound->externalTextureId() != video->textureId()) 
        {
            m_textureCurrentlyBound->setExternalTextureId(video->textureId());
        }
    }
}

void VRWebGLRenderingContext::texImage2D(GLenum target, GLint level, GLint internalformat,
    GLenum format, GLenum type, VRWebGLWebView* webview, ExceptionState& exceptionState)
{
    // VLOG(0) << "VRWebGL: VRWebGLRenderingContext::texImage2D begin";
    std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_updateSurfaceTexture::newInstance(webview->textureId());
    // VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
    VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
    // VLOG(0) << "VRWebGL: VRWebGLRenderingContext::texImage2D end";  


    if (m_textureCurrentlyBound)
    {
        if (m_textureCurrentlyBound->externalTextureId() != webview->textureId()) 
        {
            m_textureCurrentlyBound->setExternalTextureId(webview->textureId());
        }
    }
}

void VRWebGLRenderingContext::texImage2D(GLenum target, GLint level, GLint internalformat,
    GLenum format, GLenum type, ImageBitmap* bitmap, ExceptionState& exceptionState)
{
    VLOG(0) << "VRWebGL: ERROR: textImage2D with ImageBitmap is still not supported. Sorry :(.";  
    // ASSERT(bitmap->bitmapImage());
    // OwnPtr<uint8_t[]> pixelData = bitmap->copyBitmapData(bitmap->isPremultiplied() ? PremultiplyAlpha : DontPremultiplyAlpha);
    // Vector<uint8_t> data;
    // bool needConversion = true;
    // if (format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
    //     needConversion = false;
    // } else {
    //     if (type == GL_UNSIGNED_INT_10F_11F_11F_REV) {
    //         // The UNSIGNED_INT_10F_11F_11F_REV type pack/unpack isn't implemented.
    //         type = GL_FLOAT;
    //     }
    //     // In the case of ImageBitmap, we do not need to apply flipY or premultiplyAlpha.
    //     if (!WebGLImageConversion::extractImageData(pixelData.get(), bitmap->size(), format, type, false, false, data)) {
    //         VLOG(0) << "VRWebGL: ERROR: texImage2D, bad image data";
    //         return;
    //     }
    // }
    // resetUnpackParameters();
    // texImage2DBase(target, level, internalformat, bitmap->width(), bitmap->height(), 0, format, type, needConversion ? data.data() : pixelData.get());
    // restoreUnpackParameters();
}

void VRWebGLRenderingContext::uniform1f(const VRWebGLUniformLocation* location, GLfloat x)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1f begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform1f::newInstance(location, x);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1f end";
}

void VRWebGLRenderingContext::uniform1fv(const VRWebGLUniformLocation* location, const FlexibleFloat32ArrayView& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1fv 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform1fv::newInstance(location, v.length(), v.dataMaybeOnStack());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1fv 1 end";
}

void VRWebGLRenderingContext::uniform1fv(const VRWebGLUniformLocation* location, Vector<GLfloat>& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1fv 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform1fv::newInstance(location, v.size(), v.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1fv 2 end";
}

void VRWebGLRenderingContext::uniform1i(const VRWebGLUniformLocation* location, GLint x)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1i begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform1i::newInstance(location, x);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1i end";
}

void VRWebGLRenderingContext::uniform1iv(const VRWebGLUniformLocation* location, const FlexibleInt32ArrayView& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1iv 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform1iv::newInstance(location, v.length(), v.dataMaybeOnStack());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1iv 1 end";
}

void VRWebGLRenderingContext::uniform1iv(const VRWebGLUniformLocation* location, Vector<GLint>& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1iv 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform1iv::newInstance(location, v.size(), v.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform1iv 2 end";
}

void VRWebGLRenderingContext::uniform2f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2f begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform2f::newInstance(location, x, y);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2f end";
}

void VRWebGLRenderingContext::uniform2fv(const VRWebGLUniformLocation* location, const FlexibleFloat32ArrayView& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2fv 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform2fv::newInstance(location, v.length() >> 1, v.dataMaybeOnStack());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2fv 1 end";
}

void VRWebGLRenderingContext::uniform2fv(const VRWebGLUniformLocation* location, Vector<GLfloat>& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2fv 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform2fv::newInstance(location, v.size() >> 1, v.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2fv 2 end";
}

void VRWebGLRenderingContext::uniform2i(const VRWebGLUniformLocation* location, GLint x, GLint y)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2i begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform2i::newInstance(location, x, y);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2i end";
}

void VRWebGLRenderingContext::uniform2iv(const VRWebGLUniformLocation* location, const FlexibleInt32ArrayView& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2iv 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform2iv::newInstance(location, v.length() >> 1, v.dataMaybeOnStack());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2iv 1 end";
}

void VRWebGLRenderingContext::uniform2iv(const VRWebGLUniformLocation* location, Vector<GLint>& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2iv 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform2iv::newInstance(location, v.size() >> 1, v.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform2iv 2 end";
}

void VRWebGLRenderingContext::uniform3f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3f begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform3f::newInstance(location, x, y, z);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3f end";
}

void VRWebGLRenderingContext::uniform3fv(const VRWebGLUniformLocation* location, const FlexibleFloat32ArrayView& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3fv 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform3fv::newInstance(location, v.length() / 3, v.dataMaybeOnStack());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3fv 1 end";
}

void VRWebGLRenderingContext::uniform3fv(const VRWebGLUniformLocation* location, Vector<GLfloat>& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3fv 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform3fv::newInstance(location, v.size() / 3, v.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3fv 2 end";
}

void VRWebGLRenderingContext::uniform3i(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3i begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform3i::newInstance(location, x, y, z);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3i end";
}

void VRWebGLRenderingContext::uniform3iv(const VRWebGLUniformLocation* location, const FlexibleInt32ArrayView& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3iv 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform3iv::newInstance(location, v.length() / 3, v.dataMaybeOnStack());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3iv 1 end";
}

void VRWebGLRenderingContext::uniform3iv(const VRWebGLUniformLocation* location, Vector<GLint>& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3iv 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform3iv::newInstance(location, v.size() / 3, v.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform3iv 2 end";
}

void VRWebGLRenderingContext::uniform4f(const VRWebGLUniformLocation* location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4f begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform4f::newInstance(location, x, y, z, w);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4f end";
}

void VRWebGLRenderingContext::uniform4fv(const VRWebGLUniformLocation* location, const FlexibleFloat32ArrayView& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4fv 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform4fv::newInstance(location, v.length() >> 2, v.dataMaybeOnStack());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4fv 1 end";
}

void VRWebGLRenderingContext::uniform4fv(const VRWebGLUniformLocation* location, Vector<GLfloat>& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4fv 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform4fv::newInstance(location, v.size() >> 2, v.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4fv 2 end";
}

void VRWebGLRenderingContext::uniform4i(const VRWebGLUniformLocation* location, GLint x, GLint y, GLint z, GLint w)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4i begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform4i::newInstance(location, x, y, z, w);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4i end";
}

void VRWebGLRenderingContext::uniform4iv(const VRWebGLUniformLocation* location, const FlexibleInt32ArrayView& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4iv 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform4iv::newInstance(location, v.length() >> 2, v.dataMaybeOnStack());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4iv 1 end";
}

void VRWebGLRenderingContext::uniform4iv(const VRWebGLUniformLocation* location, Vector<GLint>& v)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4iv 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniform4iv::newInstance(location, v.size() >> 2, v.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniform4iv 2 end";
}

void VRWebGLRenderingContext::uniformMatrix2fv(const VRWebGLUniformLocation* location, GLboolean transpose, DOMFloat32Array* value)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix2fv 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniformMatrix2fv::newInstance(location, value->length() >> 2, transpose, value->data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix2fv 1 end";
}

void VRWebGLRenderingContext::uniformMatrix2fv(const VRWebGLUniformLocation* location, GLboolean transpose, Vector<GLfloat>& value)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix2fv 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniformMatrix2fv::newInstance(location, value.size() >> 2, transpose, value.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix2fv 2 end";
}

void VRWebGLRenderingContext::uniformMatrix3fv(const VRWebGLUniformLocation* location, GLboolean transpose, DOMFloat32Array* value)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix3fv 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniformMatrix3fv::newInstance(location, value->length() / 9, transpose, value->data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix3fv 1 end";
}

void VRWebGLRenderingContext::uniformMatrix3fv(const VRWebGLUniformLocation* location, GLboolean transpose, Vector<GLfloat>& value)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix3fv 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniformMatrix3fv::newInstance(location, value.size() / 9, transpose, value.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix3fv 2 end";
}

void VRWebGLRenderingContext::uniformMatrix4fv(const VRWebGLUniformLocation* location, GLboolean transpose, DOMFloat32Array* value)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix4fv 1 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniformMatrix4fv::newInstance(m_programCurrentlyInUse, location, value->length() >> 4, transpose, value->data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix4fv 1 end";
}

void VRWebGLRenderingContext::uniformMatrix4fv(const VRWebGLUniformLocation* location, GLboolean transpose, Vector<GLfloat>& value)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix4fv 2 begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_uniformMatrix4fv::newInstance(m_programCurrentlyInUse, location, value.size() >> 4, transpose, value.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::uniformMatrix4fv 2 end";
}

void VRWebGLRenderingContext::useProgram(ScriptState* scriptState, VRWebGLProgram* program)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::useProgram begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_useProgram::newInstance(program);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::useProgram end";

	m_programCurrentlyInUse = program;
}

void VRWebGLRenderingContext::vertexAttribPointer(ScriptState* scriptState, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, long long offset)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::vertexAttribPointer begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_vertexAttribPointer::newInstance(index, size, type, normalized, stride, offset);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::vertexAttribPointer end";
}

void VRWebGLRenderingContext::viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::viewport begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_viewport::newInstance(x, y, width, height, m_framebufferCurrentlyBound == 0);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::viewport end";
}

void VRWebGLRenderingContext::startFrame()
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::startFrame begin";
	VRWebGLCommandProcessor::getInstance()->startFrame();
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::startFrame end";
}

void VRWebGLRenderingContext::endFrame()
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::endFrame begin";
	VRWebGLCommandProcessor::getInstance()->endFrame();
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::endFrame end";
}

Vector<GLfloat> VRWebGLRenderingContext::getModelViewMatrix()
{
	// TODO: this call to get and set the view matrix in the command processor should be synchronous
	const GLfloat* viewMatrix = VRWebGLCommandProcessor::getInstance()->getViewMatrix();
	for (size_t i = 0; i < 16; i++)
	{
		m_modelViewMatrix[i] = viewMatrix[i];
	}
	return m_modelViewMatrix;
}

VRPose* VRWebGLRenderingContext::getPose()
{
	VRWebGLCommandProcessor::getInstance()->getPose(m_vrWebGLPose);
	m_vrPose->setPose(m_vrWebGLPose);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getPose: " << m_vrWebGLPose.orientation[0] << ", " << m_vrWebGLPose.orientation[1] << ", " << m_vrWebGLPose.orientation[2] << ", " << m_vrWebGLPose.orientation[3];
	return m_vrPose;
}

VREyeParameters* VRWebGLRenderingContext::getEyeParameters(const String& eye)
{
    VRWebGLCommandProcessor::getInstance()->getEyeParameters(eye.utf8().data(), m_vrWebGLEyeParameters);
    VREyeParameters* vrEyeParameters;
    if (eye == "left")
    {
        m_vrEyeParametersLeft->update(m_vrWebGLEyeParameters);
        vrEyeParameters = m_vrEyeParametersLeft;
    }
    else if (eye == "right")
    {
        m_vrEyeParametersRight->update(m_vrWebGLEyeParameters);
        vrEyeParameters = m_vrEyeParametersRight;
    }
    // VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getEyeParameters: " << m_vrWebGLEyeParameters.offset << ", " << m_vrWebGLEyeParameters.xFOV << ", " << m_vrWebGLEyeParameters.yFOV << ", " << m_vrWebGLEyeParameters.width << ", " << m_vrWebGLEyeParameters.height];
    return vrEyeParameters;
}

void VRWebGLRenderingContext::setCameraWorldMatrix(DOMFloat32Array* value)
{
    // The camera world matrix is used for getting both the orientation
    // and the position, so it needs to be a command so it is correctly
    // processed.
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::setCameraWorldMatrix begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_setCameraWorldMatrix::newInstance(value->data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::setCameraWorldMatrix end";
}

void VRWebGLRenderingContext::setCameraWorldMatrix(Vector<GLfloat>& value)
{
    // The camera world matrix is used for getting both the orientation
    // and the position, so it needs to be a command so it is correctly
    // processed.
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::setCameraWorldMatrix begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_setCameraWorldMatrix::newInstance(value.data());
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::setCameraWorldMatrix end";
}

void VRWebGLRenderingContext::setCameraProjectionMatrix(DOMFloat32Array* value)
{
    // As the projection matrix is only used to calculate the near and far
    // planes, there is no need for a VRWebGLCommand.
	VRWebGLCommandProcessor::getInstance()->setCameraProjectionMatrix(value->data());
}

void VRWebGLRenderingContext::setCameraProjectionMatrix(Vector<GLfloat>& value)
{
    // As the projection matrix is only used to calculate the near and far
    // planes, there is no need for a VRWebGLCommand.
	VRWebGLCommandProcessor::getInstance()->setCameraProjectionMatrix(value.data());
}

void VRWebGLRenderingContext::setRenderEnabled(bool flag)
{
    VRWebGLCommandProcessor::getInstance()->setRenderEnabled(flag);    
}

Gamepad* VRWebGLRenderingContext::getGamepad()
{
    std::shared_ptr<blink::WebGamepad> gamepad = VRWebGLCommandProcessor::getInstance()->getGamepad();
    if (gamepad)
    {
        // Create the gamepad instance the first time.
        if (!m_gamepad)
        {
            m_gamepad = Gamepad::create();
            // TODO: Fix this. We should be able to use the real 16 bit type for the id string.
            m_gamepad->setId((const char*)gamepad->id);
        }
        m_gamepad->setButtons(gamepad->buttonsLength, gamepad->buttons);
        m_gamepad->setAxes(gamepad->axesLength, gamepad->axes);
        m_gamepad->setPose(gamepad->pose);
        m_gamepad->setHand(gamepad->hand);
    }
    return m_gamepad;
} 

// VRWebGLRenderingContext::VRWebGLRenderingContext(HTMLCanvasElement* canvas): m_canvas(canvas)
// {

// }

VRWebGLRenderingContext::VRWebGLRenderingContext():
	// m_canvas(nullptr),
	m_unpackFlipY(false),
	m_generatedImageCache(4),
	m_modelViewMatrix(16),
    m_gamepad(nullptr)
{
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_getString::newInstance(GL_EXTENSIONS));

	String extensionsString((const GLubyte*)result);
    extensionsString.split(' ', m_supportedExtensionNames);	
    // TODO: It seems that the right way to get all the extension available is to use the GLES3 way of asking for the number of extensions first and then go for each of the strings one by one: https://github.com/android-ndk/ndk/issues/279. Adding this extension manually for now but fix it asap.
    m_supportedExtensionNames.push_back("ANGLE_instanced_arrays");

    // TODO: We shouldn't need to do this when the extensions are properly handled.
	VLOG(0) << "VRWebGL: Supported extensions: " << extensionsString;
	for (size_t i = 0; i < m_supportedExtensionNames.size(); i++) 
	{
		m_supportedExtensionNames[i].replace("GL_", "");
		m_supportedExtensionNames[i].replace("EXT_", "");
		m_supportedExtensionNames[i].replace("OES_", "");
		m_supportedExtensionNames[i].replace("QCOM_", "");
		m_supportedExtensionNames[i].replace("ANDROID_", "");
		m_supportedExtensionNames[i].replace("OVR_", "");
		m_supportedExtensionNames[i].replace("KHR_", "");

		VLOG(0) << "VRWebGL: " << m_supportedExtensionNames[i];
	}

    m_vrPose = VRPose::create();
    m_vrEyeParametersLeft = new VREyeParameters();
    m_vrEyeParametersRight = new VREyeParameters();

    VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(VRWebGLCommand_initializeExtensions::newInstance());

    // TODO: Create a proper structure (map) to hold all the supported extensions.
    m_angleInstancedArraysExtension = VRWebGLANGLEInstancedArrays::create();
}

void VRWebGLRenderingContext::resetUnpackParameters()
{
    if (m_unpackAlignment != 1)
        pixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void VRWebGLRenderingContext::restoreUnpackParameters()
{
    if (m_unpackAlignment != 1)
        pixelStorei(GL_UNPACK_ALIGNMENT, m_unpackAlignment);
}

void VRWebGLRenderingContext::texImage2DBase(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::texImage2DBase begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_texImage2D::newInstance(target, level, convertTexInternalFormat(internalformat, type), width, height, border, format, type, pixels);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::texImage2DBase end";
}

void VRWebGLRenderingContext::texImageImpl(TexImageFunctionID functionID, GLenum target, GLint level, GLint internalformat, GLint xoffset, GLint yoffset, GLint zoffset, GLenum format, GLenum type, Image* image, WebGLImageConversion::ImageHtmlDomSource domSource, bool flipY, bool premultiplyAlpha, const IntRect& sourceImageRect, GLsizei depth, GLint unpackImageHeight)
{
    const char* funcName = getTexImageFunctionName(functionID);
    // All calling functions check isContextLost, so a duplicate check is not needed here.
    if (type == GL_UNSIGNED_INT_10F_11F_11F_REV) {
        // The UNSIGNED_INT_10F_11F_11F_REV type pack/unpack isn't implemented.
        type = GL_FLOAT;
    }

    IntRect subRect = sourceImageRect;
    if (subRect == sentinelEmptyRect()) {
        // Recalculate based on the size of the Image.
        subRect = safeGetImageSize(image);
    }
    bool selectingSubRectangle = false;
    if (!validateTexImageSubRectangle(funcName, functionID, image, subRect, depth, unpackImageHeight, &selectingSubRectangle)) {
        return;
    }
    // Adjust the source image rectangle if doing a y-flip.
    IntRect adjustedSourceImageRect = subRect;
    if (flipY) {
        adjustedSourceImageRect.setY(image->height() - adjustedSourceImageRect.maxY());
    }    

    Vector<uint8_t> data;
    WebGLImageConversion::ImageExtractor imageExtractor(image, domSource, premultiplyAlpha, m_unpackColorspaceConversion == GL_NONE);
    if (!imageExtractor.imagePixelData()) {
        VLOG(0) << "VRWebGL: ERROR: " << funcName << ", bad image data";
        return;
    }
    WebGLImageConversion::DataFormat sourceDataFormat = imageExtractor.imageSourceFormat();
    WebGLImageConversion::AlphaOp alphaOp = imageExtractor.imageAlphaOp();
    const void* imagePixelData = imageExtractor.imagePixelData();

    bool needConversion = true;
    if (type == GL_UNSIGNED_BYTE && sourceDataFormat == WebGLImageConversion::DataFormatRGBA8 && format == GL_RGBA && alphaOp == WebGLImageConversion::AlphaDoNothing && !flipY) {
        needConversion = false;
    } else {

        if (!WebGLImageConversion::packImageData(
                image, imagePixelData, format, type, flipY, alphaOp, sourceDataFormat, 
                imageExtractor.imageWidth(), imageExtractor.imageHeight(), adjustedSourceImageRect, depth,
                imageExtractor.imageSourceUnpackAlignment(), unpackImageHeight, data)) {
	        VLOG(0) << "VRWebGL: ERROR: " << funcName << ", packImage error";
            return;
        }
    }

    resetUnpackParameters();
    if (functionID == TexImage2D) {
        texImage2DBase(target, level, internalformat, imageExtractor.imageWidth(), imageExtractor.imageHeight(), 0, format, type, needConversion ? data.data() : imagePixelData);
    } else if (functionID == TexSubImage2D) {
    	// TODO
        // contextGL()->TexSubImage2D(target, level, xoffset, yoffset, imageExtractor.imageWidth(), imageExtractor.imageHeight(), format, type,  needConversion ? data.data() : imagePixelData);
    } else {
        DCHECK_EQ(functionID, TexSubImage3D);
        // TODO
        // contextGL()->TexSubImage3D(target, level, xoffset, yoffset, zoffset, imageExtractor.imageWidth(), imageExtractor.imageHeight(), 1, format, type, needConversion ? data.data() : imagePixelData);
    }
    restoreUnpackParameters();
}

// void VRWebGLRenderingContext::texImage2DImpl(GLenum target, GLint level, GLint internalformat, GLenum format, GLenum type, Image* image, WebGLImageConversion::ImageHtmlDomSource domSource, bool flipY, bool premultiplyAlpha)
// {
//     // All calling functions check isContextLost, so a duplicate check is not needed here.
//     if (type == GL_UNSIGNED_INT_10F_11F_11F_REV) {
//         // The UNSIGNED_INT_10F_11F_11F_REV type pack/unpack isn't implemented.
//         type = GL_FLOAT;
//     }
//     Vector<uint8_t> data;
//     WebGLImageConversion::ImageExtractor imageExtractor(image, domSource, premultiplyAlpha, m_unpackColorspaceConversion == GL_NONE);
//     if (!imageExtractor.imagePixelData()) {
//         VLOG(0) << "VRWebGL: ERROR: texImage2D, bad image data";
//         return;
//     }
//     WebGLImageConversion::DataFormat sourceDataFormat = imageExtractor.imageSourceFormat();
//     WebGLImageConversion::AlphaOp alphaOp = imageExtractor.imageAlphaOp();
//     const void* imagePixelData = imageExtractor.imagePixelData();

//     bool needConversion = true;
//     if (type == GL_UNSIGNED_BYTE && sourceDataFormat == WebGLImageConversion::DataFormatRGBA8 && format == GL_RGBA && alphaOp == WebGLImageConversion::AlphaDoNothing && !flipY) {
//         needConversion = false;
//     } else {
//     	VLOG(0) << "VRWebGL: converting image..." << type << " == " << GL_UNSIGNED_BYTE << ", " << (int)sourceDataFormat << " == " << (int)WebGLImageConversion::DataFormatRGBA8 << ", " << format << " == " << GL_RGBA << ", " << (int)alphaOp << " == " << (int)WebGLImageConversion::AlphaDoNothing << ", " << (flipY ? "true" : "false");
//         if (!WebGLImageConversion::packImageData(
//                 image, imagePixelData, format, type, flipY, alphaOp, sourceDataFormat, 
//                 imageExtractor.imageWidth(), imageExtractor.imageHeight(), adjustedSourceImageRect, depth,
//                 imageExtractor.imageSourceUnpackAlignment(), data)) {
//             VLOG(0) << "VRWebGL: ERROR: texImage2D, packImage error";
//             return;
//         }
//     }

//     resetUnpackParameters();
//     texImage2DBase(target, level, internalformat, imageExtractor.imageWidth(), imageExtractor.imageHeight(), 0, format, type, needConversion ? data.data() : imagePixelData);
//     restoreUnpackParameters();
// }

PassRefPtr<Image> VRWebGLRenderingContext::drawImageIntoBuffer(PassRefPtr<Image> passImage,
    int width, int height, const char* functionName)
{
	// TODO
    // RefPtr<Image> image(passImage);
    // ASSERT(image);

    // IntSize size(width, height);
    // ImageBuffer* buf = m_generatedImageCache.imageBuffer(size);
    // if (!buf) {
    //     VLOG(0) << "VRWebGL: ERROR: " << functionName << ", out of memory";
    //     return nullptr;
    // }

    // if (!image->currentFrameKnownToBeOpaque())
    //     buf->canvas()->clear(SK_ColorTRANSPARENT);

    // IntRect srcRect(IntPoint(), image->size());
    // IntRect destRect(0, 0, size.width(), size.height());
    // SkPaint paint;
    // image->draw(buf->canvas(), paint, destRect, srcRect, DoNotRespectImageOrientation, Image::DoNotClampImageToSourceRect);
    // return buf->newImageSnapshot();
    return nullptr;
}

GLenum VRWebGLRenderingContext::convertTexInternalFormat(GLenum internalformat, GLenum type)
{
	// TODO
    // Convert to sized internal formats that are renderable with GL_CHROMIUM_color_buffer_float_rgb(a).
    if (type == GL_FLOAT && internalformat == GL_RGBA)
        // && extensionsUtil()->isExtensionEnabled("GL_CHROMIUM_color_buffer_float_rgba"))
        return GL_RGBA32F_EXT;
    if (type == GL_FLOAT && internalformat == GL_RGB)
        // && extensionsUtil()->isExtensionEnabled("GL_CHROMIUM_color_buffer_float_rgb"))
        return GL_RGB32F_EXT;
    return internalformat;
}

void VRWebGLRenderingContext::texParameter(GLenum target, GLenum pname, GLfloat paramf, GLint parami, bool isFloat)
{
    switch (pname) {
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
        break;
    case GL_TEXTURE_WRAP_R:
    	// TODO
        // fall through to WRAP_S and WRAP_T for WebGL 2 or higher
        // if (!isWebGL2OrHigher()) {
        //     VLOG(0) << "VRWebGL: ERROR: texParameter, invalid parameter name");
        //     return;
        // }
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
        if ((isFloat && paramf != GL_CLAMP_TO_EDGE && paramf != GL_MIRRORED_REPEAT && paramf != GL_REPEAT)
            || (!isFloat && parami != GL_CLAMP_TO_EDGE && parami != GL_MIRRORED_REPEAT && parami != GL_REPEAT)) {
            VLOG(0) << "VRWebGL: ERROR: texParameter, invalid parameter name";
            return;
        }
        break;
    case GL_TEXTURE_MAX_ANISOTROPY_EXT: // EXT_texture_filter_anisotropic
    	// TODO
        // if (!extensionEnabled(EXTTextureFilterAnisotropicName)) {
        //     VLOG(0) << "VRWebGL: ERROR: texParameter, invalid parameter, EXT_texture_filter_anisotropic not enabled";
        //     return;
        // }
        break;
    case GL_TEXTURE_COMPARE_FUNC:
    case GL_TEXTURE_COMPARE_MODE:
    case GL_TEXTURE_BASE_LEVEL:
    case GL_TEXTURE_MAX_LEVEL:
    case GL_TEXTURE_MAX_LOD:
    case GL_TEXTURE_MIN_LOD:
    	// TODO
        // if (!isWebGL2OrHigher()) {
        //     VLOG(0) << "VRWebGL: ERROR: texParameter, invalid parameter name";
        //     return;
        // }
        break;
    default:
        VLOG(0) << "VRWebGL: ERROR: texParameter, invalid parameter name";
        return;
    }

	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::texParameter begin";
    std::shared_ptr<VRWebGLCommand> vrWebGLCommand;
    if (isFloat) {
		vrWebGLCommand = VRWebGLCommand_texParameterf::newInstance(target, pname, paramf);
    } else {
        // Pass the texture to try to apply the correct target for external textures for filters. 
		vrWebGLCommand = VRWebGLCommand_texParameteri::newInstance(target, pname, parami, m_textureCurrentlyBound);
    }
	VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::texParameter end";
}

ScriptValue VRWebGLRenderingContext::getBooleanParameter(ScriptState* scriptState, GLenum pname)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getBooleanParameter begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getBooleanv::newInstance(pname);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getBooleanParameter end";
	GLboolean value = *((GLboolean*)result);
    return WebGLAny(scriptState, static_cast<bool>(value));
}

ScriptValue VRWebGLRenderingContext::getBooleanArrayParameter(ScriptState* scriptState, GLenum pname)
{
    if (pname != GL_COLOR_WRITEMASK) {
        NOTIMPLEMENTED();
        return WebGLAny(scriptState, 0, 0);
    }
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getBooleanParameter begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getBooleanv::newInstance(pname);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getBooleanParameter end";
	GLboolean* value = (GLboolean*)result;
    bool boolValue[4];
    for (int ii = 0; ii < 4; ++ii)
        boolValue[ii] = static_cast<bool>(value[ii]);
    return WebGLAny(scriptState, boolValue, 4);
}

ScriptValue VRWebGLRenderingContext::getFloatParameter(ScriptState* scriptState, GLenum pname)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getFloatParameter begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getFloatv::newInstance(pname);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getFloatParameter end";
	GLfloat value = *((GLfloat*)result);
    return WebGLAny(scriptState, value);
}

ScriptValue VRWebGLRenderingContext::getIntParameter(ScriptState* scriptState, GLenum pname)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getIntParameter begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getIntegerv::newInstance(pname);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getIntParameter end";
	GLint value = *((GLint*)result);
    switch (pname) {
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
    case GL_IMPLEMENTATION_COLOR_READ_TYPE:
        if (value == 0) {
            // This indicates read framebuffer is incomplete and an
            // INVALID_OPERATION has been generated.
            return ScriptValue::createNull(scriptState);
        }
        break;
    default:
        break;
    }
    return WebGLAny(scriptState, value);
}

// ScriptValue VRWebGLRenderingContext::getInt64Parameter(ScriptState* scriptState, GLenum pname)
// {
// 	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getInt64Parameter begin";
// 	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getInteger64v::newInstance(pname);
// 	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
// 	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
// 	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
// 	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getInt64Parameter end";
// 	GLint64 value = *((GLint64*)result);
//     return WebGLAny(scriptState, value);
// }

ScriptValue VRWebGLRenderingContext::getUnsignedIntParameter(ScriptState* scriptState, GLenum pname)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getUnsignedIntParameter begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getIntegerv::newInstance(pname);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getUnsignedIntParameter end";
	GLint value = *((GLint*)result);
    return WebGLAny(scriptState, static_cast<unsigned>(value));
}

ScriptValue VRWebGLRenderingContext::getWebGLFloatArrayParameter(ScriptState* scriptState, GLenum pname)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getWebGLFloatArrayParameter begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getFloatv::newInstance(pname);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getWebGLFloatArrayParameter end";
	GLfloat* value = (GLfloat*)result;
    unsigned length = 0;
    switch (pname) {
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_DEPTH_RANGE:
        length = 2;
        break;
    case GL_BLEND_COLOR:
    case GL_COLOR_CLEAR_VALUE:
        length = 4;
        break;
    default:
        NOTIMPLEMENTED();
    }
    return WebGLAny(scriptState, DOMFloat32Array::create(value, length));
}

ScriptValue VRWebGLRenderingContext::getWebGLIntArrayParameter(ScriptState* scriptState, GLenum pname)
{
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getWebGLIntArrayParameter begin";
	std::shared_ptr<VRWebGLCommand> vrWebGLCommand = VRWebGLCommand_getIntegerv::newInstance(pname);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name();
	void* result = VRWebGLCommandProcessor::getInstance()->queueVRWebGLCommandForProcessing(vrWebGLCommand);
	// VLOG(0) << "VRWebGL: " << VRWebGLCommandProcessor::getInstance()->getCurrentThreadName() << ": " << vrWebGLCommand->name() << " result = " << *((GLint*)result);
	// VLOG(0) << "VRWebGL: VRWebGLRenderingContext::getWebGLIntArrayParameter end";
	GLint* value = (GLint*)result;
    unsigned length = 0;
    switch (pname) {
    case GL_MAX_VIEWPORT_DIMS:
        length = 2;
        break;
    case GL_SCISSOR_BOX:
    case GL_VIEWPORT:
        length = 4;
        break;
    default:
        NOTIMPLEMENTED();
    }
    return WebGLAny(scriptState, DOMInt32Array::create(value, length));
}

bool VRWebGLRenderingContext::isPrefixReserved(const String& name)
{
    if (name.startsWith("gl_") || name.startsWith("webgl_") || name.startsWith("_webgl_"))
        return true;
    return false;
}

DEFINE_TRACE(VRWebGLRenderingContext)
{
    // visitor->trace(m_canvas);
    visitor->trace(m_vrPose);
    visitor->trace(m_vrEyeParametersLeft);
    visitor->trace(m_vrEyeParametersRight);
    visitor->trace(m_programCurrentlyInUse);
    visitor->trace(m_framebufferCurrentlyBound);
    visitor->trace(m_textureCurrentlyBound);
}

}