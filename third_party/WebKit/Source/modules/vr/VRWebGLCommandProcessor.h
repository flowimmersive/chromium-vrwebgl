#ifndef VRWebGLCommandProcessor_h
#define VRWebGLCommandProcessor_h

#include <pthread.h>

#include <GLES3/gl3.h>

#include <string>
#include <deque>
#include <memory>
#include <map>

#include "modules/vr/VRWebGLCommand.h"
#include "modules/vr/VRWebGLSurfaceTexture.h"

class VRWebGLPose;
class VRWebGLEyeParameters;

namespace blink
{
    class WebGamepad;
}

class VRWebGLCommandProcessor
{
public:
    static VRWebGLCommandProcessor* getInstance();

    virtual ~VRWebGLCommandProcessor();

    virtual void startFrame() = 0;

    virtual void* queueVRWebGLCommandForProcessing(const std::shared_ptr<VRWebGLCommand>& vrWebGLCommand) = 0;
    
    virtual void endFrame() = 0;
    
    virtual void update() = 0;
    
    virtual void renderFrame(bool process = true) = 0;

    // The following methods should only be called from the OpenGL thread, so no mutex lock is implemented.
    virtual void setMatrixUniformLocationForName(const GLuint program, const GLint location, const std::string& name) = 0;

    virtual bool isProjectionMatrixUniformLocation(const GLuint program, const GLint location) const = 0;

    virtual bool isModelViewMatrixUniformLocation(const GLuint program, const GLint location) const = 0;

    virtual bool isModelViewProjectionMatrixUniformLocation(const GLuint program, const GLint location) const = 0;

    virtual void setViewAndProjectionMatrices(const GLfloat* projectionMatrix, const GLfloat* modelViewMatrix) = 0;

    virtual const GLfloat* getProjectionMatrix() const = 0;

    virtual const GLfloat* getViewMatrix() const = 0;

    virtual const GLfloat* getViewProjectionMatrix() const = 0;

    virtual void setCurrentThreadName(const std::string& name) = 0;

    virtual const std::string& getCurrentThreadName() const = 0;

    virtual void setFramebuffer(const GLuint framebuffer) = 0;

    virtual GLuint getFramebuffer() const = 0;

    virtual void setViewport(GLint x, GLint y, GLsizei width, GLsizei height) = 0;

    virtual void getViewport(GLint& x, GLint& y, GLsizei& width, GLsizei& height) const = 0;

    virtual void setCameraWorldMatrix(const GLfloat* cameraWorldMatrix) = 0;

    virtual const GLfloat* getCameraWorldMatrix() const = 0;

    virtual const GLfloat* getCameraWorldMatrixWithTranslationOnly() const = 0;

    virtual void reset() = 0;

    virtual bool m_synchronousVRWebGLCommandBeenProcessedInUpdate() const = 0;

    virtual void setupJNI(JNIEnv* jniEnv, jobject mainActivityJObject) = 0;

    virtual JNIEnv* getJNIEnv() const = 0;

    virtual const jobject getMainActivityJObject() const = 0;

    virtual std::shared_ptr<VRWebGLSurfaceTexture> newSurfaceTexture() = 0;

    virtual void deleteSurfaceTexture(const std::shared_ptr<VRWebGLSurfaceTexture>& surfaceTexture) = 0;

    virtual std::shared_ptr<VRWebGLSurfaceTexture> findSurfaceTextureByTextureId(unsigned textureId) = 0;

    virtual jmethodID getNewWebViewMethodID() const = 0;

    virtual jmethodID getDeleteWebViewMethodID() const = 0;

    virtual jmethodID getSetWebViewSrcMethodID() const = 0;

    virtual jmethodID getDispatchWebViewTouchEventMethodID() const = 0;

    virtual jmethodID getDispatchWebViewNavigationEventMethodID() const = 0;

    virtual jmethodID getDispatchWebViewKeyboardEventMethodID() const = 0;

    // These methods will be implemented where they can provide the requested functionality. Most likely in the Oculus SDK implementation part.
    // TODO: Try to get rid of as many as possible and use VRWebGLCommands instead!
    void getPose(VRWebGLPose& pose);
    void getEyeParameters(const std::string& eye, VRWebGLEyeParameters& eyeParameters);
    void setCameraProjectionMatrix(GLfloat* cameraProjectionMatrix);
    void setRenderEnabled(bool flag);
    GLuint newVideoTexture();
    void deleteVideoTexture(GLuint videoTextureId);
    void setVideoSource(GLuint videoTextureId, const std::string& src);
    void playVideo(GLuint videoTextureId, double volume, bool loop);
    void pauseVideo(GLuint videoTextureId);
    void setVideoVolume(GLuint videoTextureId, double volume);
    void setVideoLoop(GLuint videoTextureId, bool loop);
    void setVideoCurrentTime(GLuint videoTextureId, double currentTime);
    double getVideoCurrentTime(GLuint videoTextureId);
    double getVideoDuration(GLuint videoTextureId);
    int getVideoWidth(GLuint videoTextureId);
    int getVideoHeight(GLuint videoTextureId);
    bool checkVideoPrepared(GLuint videoTextureId);
    bool checkVideoEnded(GLuint videoTextureId);
    std::shared_ptr<blink::WebGamepad> getGamepad();
};

class VRWebGLCommandProcessorImpl final: public VRWebGLCommandProcessor
{
private:
    class VRWebGLProgramAndUniformLocation
    {
    private:
        GLuint m_program;
        GLint m_location;
    public:
        VRWebGLProgramAndUniformLocation();
        VRWebGLProgramAndUniformLocation(const GLuint program, const GLint location);
        bool operator==(const VRWebGLProgramAndUniformLocation& other) const;
    };

    std::deque<std::shared_ptr<VRWebGLCommand>> m_vrWebGLCommandQueue;
    std::deque<std::shared_ptr<VRWebGLCommand>> m_vrWebGLCommandQueueBatch;
    bool m_insideAFrame = false;
    bool m_currentBatchRenderedForBothEyes = true;
    bool m_synchronousVRWebGLCommandProcessedInUpdate = false;
    std::shared_ptr<VRWebGLCommand> m_synchronousVRWebGLCommand;
    void* m_synchronousVRWebGLCommandResult = 0;
    pthread_mutex_t	m_mutex;
    pthread_cond_t m_synchronousVRWebGLCommandProcessed;
    pthread_cond_t m_bothEyesRendered;

    unsigned int m_indexOfEyeBeingRendered = -1;

    std::deque<std::string> m_projectionMatrixUniformNames;
    std::deque<std::string> m_modelViewMatrixUniformNames;
    std::deque<std::string> m_modelViewProjectionMatrixUniformNames;
    std::deque<VRWebGLProgramAndUniformLocation> m_projectionMatrixProgramAndUniformLocations;
    std::deque<VRWebGLProgramAndUniformLocation> m_modelViewMatrixProgramAndUniformLocations;
    std::deque<VRWebGLProgramAndUniformLocation> m_modelViewProjectionMatrixProgramAndUniformLocations;

    GLfloat m_projectionMatrix[16];
    GLfloat m_viewMatrix[16];
    GLfloat m_viewProjectionMatrix[16];
    GLfloat m_cameraWorldMatrix[16];
    GLfloat m_cameraWorldMatrixWithTranslationOnly[16];

    std::string m_threadNames[10];
    pthread_t m_threadIds[10];

    GLuint m_framebuffer = 0;

    GLint m_x;
    GLint m_y;
    GLsizei m_width;
    GLsizei m_height;

    JNIEnv* m_jniEnv;
    JavaVM* m_javaVM;
    VRWebGLSurfaceTextures m_surfaceTextures;
    jobject m_mainActivityJObject;
    jclass m_mainActivityJClass;
    jmethodID m_newWebViewMethodID;
    jmethodID m_deleteWebViewMethodID;
    jmethodID m_setWebViewSrcMethodID;
    jmethodID m_dispatchWebViewTouchEventMethodID;
    jmethodID m_dispatchWebViewNavigationEventMethodID;
    jmethodID m_dispatchWebViewKeyboardEventMethodID;

    // Do not allow copy of instances.
    VRWebGLCommandProcessorImpl(const VRWebGLCommandProcessorImpl&) = delete;
    VRWebGLCommandProcessorImpl& operator=(const VRWebGLCommandProcessorImpl&) = delete;

    // This class is a singleton
    VRWebGLCommandProcessorImpl();

public:
    static VRWebGLCommandProcessor* getInstance();

    virtual ~VRWebGLCommandProcessorImpl() override;
    
    virtual void startFrame() override;

    virtual void* queueVRWebGLCommandForProcessing(const std::shared_ptr<VRWebGLCommand>& vrWebGLCommand) override;
    
    virtual void endFrame() override;
    
    virtual void update() override;
    
    virtual void renderFrame(bool process = true) override;

    // The following methods should only be called from the OpenGL thread, so no mutex lock is implemented.
    virtual void setMatrixUniformLocationForName(const GLuint program, const GLint location, const std::string& name) override;

    virtual bool isProjectionMatrixUniformLocation(const GLuint program, const GLint location) const override;

    virtual bool isModelViewMatrixUniformLocation(const GLuint program, const GLint location) const override;

    virtual bool isModelViewProjectionMatrixUniformLocation(const GLuint program, const GLint location) const override;

    virtual void setViewAndProjectionMatrices(const GLfloat* projectionMatrix, const GLfloat* modelViewMatrix) override;

    virtual const GLfloat* getProjectionMatrix() const override;

    virtual const GLfloat* getViewMatrix() const override;

    virtual const GLfloat* getViewProjectionMatrix() const override;

    virtual void setCurrentThreadName(const std::string& name) override;

    virtual const std::string& getCurrentThreadName() const override;    

    virtual void setFramebuffer(const GLuint framebuffer) override;

    virtual GLuint getFramebuffer() const override;

    virtual void setViewport(GLint x, GLint y, GLsizei width, GLsizei height) override;

    virtual void getViewport(GLint& x, GLint& y, GLsizei& width, GLsizei& height) const override;    

    virtual void setCameraWorldMatrix(const GLfloat* cameraWorldMatrix) override;

    virtual const GLfloat* getCameraWorldMatrix() const override;    

    virtual const GLfloat* getCameraWorldMatrixWithTranslationOnly() const override;

    virtual void reset() override;  

    virtual bool m_synchronousVRWebGLCommandBeenProcessedInUpdate() const override;

    virtual void setupJNI(JNIEnv* jniEnv, jobject mainActivityJObject) override;

    virtual JNIEnv* getJNIEnv() const override;

    virtual const jobject getMainActivityJObject() const override;

    virtual std::shared_ptr<VRWebGLSurfaceTexture> newSurfaceTexture() override;

    virtual void deleteSurfaceTexture(const std::shared_ptr<VRWebGLSurfaceTexture>& surfaceTexture) override;

    virtual std::shared_ptr<VRWebGLSurfaceTexture> findSurfaceTextureByTextureId(unsigned textureId) override;   

    virtual jmethodID getNewWebViewMethodID() const override;

    virtual jmethodID getDeleteWebViewMethodID() const override;

    virtual jmethodID getSetWebViewSrcMethodID() const override;

    virtual jmethodID getDispatchWebViewTouchEventMethodID() const override;

    virtual jmethodID getDispatchWebViewNavigationEventMethodID() const override;

    virtual jmethodID getDispatchWebViewKeyboardEventMethodID() const override;
};

// =====================================================================================
// =====================================================================================

class VRWebGLCommand_newWebView: public VRWebGLCommand
{
private:
    GLuint textureId;
    bool processed = false;

    VRWebGLCommand_newWebView();

public:
    static std::shared_ptr<VRWebGLCommand_newWebView> newInstance();

    virtual bool isSynchronous() const override;

    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

// =====================================================================================

class VRWebGLCommand_deleteWebView: public VRWebGLCommand
{
private:
    GLuint textureId;
    bool processed = false;

    VRWebGLCommand_deleteWebView(GLuint textureId);

public:
    static std::shared_ptr<VRWebGLCommand_deleteWebView> newInstance(GLuint textureId);

    virtual bool isSynchronous() const override;

    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

class VRWebGLCommand_setWebViewSrc: public VRWebGLCommand
{
private:
    GLuint textureId;
    std::string src;
    bool processed = false;

    VRWebGLCommand_setWebViewSrc(GLuint textureId, const std::string& src);

public:
    static std::shared_ptr<VRWebGLCommand_setWebViewSrc> newInstance(GLuint textureId, const std::string& src);

    virtual bool isSynchronous() const override;

    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

class VRWebGLCommand_checkWebViewLoaded: public VRWebGLCommand
{
public:
    virtual bool isSynchronous() const override;

    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

class VRWebGLCommand_dispatchWebViewTouchEvent: public VRWebGLCommand
{
public:
    enum Event
    {
        TOUCH_START = 1,
        TOUCH_MOVE = 2,
        TOUCH_END = 3
    };

private:
    GLuint textureId;
    Event event;
    float x;
    float y;
    bool processed = false;

    VRWebGLCommand_dispatchWebViewTouchEvent(GLuint textureId, Event event, float x, float y);

public:
    static std::shared_ptr<VRWebGLCommand_dispatchWebViewTouchEvent> newInstance(GLuint textureId, Event event, float x, float y);

    virtual bool isSynchronous() const override;

    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

class VRWebGLCommand_dispatchWebViewNavigationEvent: public VRWebGLCommand
{
public:
    enum Event
    {
        NAVIGATION_BACK = 1,
        NAVIGATION_FORWARD = 2,
        NAVIGATION_RELOAD = 3,
        NAVIGATION_VOICE_SEARCH = 4
    };

private:
    GLuint textureId;
    Event event;
    bool processed = false;

    VRWebGLCommand_dispatchWebViewNavigationEvent(GLuint textureId, Event event);

public:
    static std::shared_ptr<VRWebGLCommand_dispatchWebViewNavigationEvent> newInstance(GLuint textureId, Event event);

    virtual bool isSynchronous() const override;

    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

class VRWebGLCommand_dispatchWebViewKeyboardEvent: public VRWebGLCommand
{
public:
    enum Event
    {
        KEY_DOWN = 1,
        KEY_UP = 2
    };

private:
    GLuint textureId;
    Event event;
    int keycode;
    bool processed = false;
    static std::map<int, int> jsKeyCodeToJavaKeyCode;

    VRWebGLCommand_dispatchWebViewKeyboardEvent(GLuint textureId, Event event, int keycode);

public:
    static std::shared_ptr<VRWebGLCommand_dispatchWebViewKeyboardEvent> newInstance(GLuint textureId, Event event, int keycode);

    virtual bool isSynchronous() const override;

    virtual bool canBeProcessedImmediately() const override;
    
    virtual void* process() override;
    
    virtual std::string name() const override;
};

#endif // VRWebGLCommandProcessor_h