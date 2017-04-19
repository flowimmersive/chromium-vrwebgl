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
private:
    void m_resetEverything();  

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
    std::deque<std::shared_ptr<VRWebGLCommand>> m_vrWebGLCommandForUpdateQueue;
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
    jmethodID m_dispatchWebViewCursorEventMethodID;
    jmethodID m_setWebViewTransparentMethodID;
    jmethodID m_setWebViewScaleMethodID;
    jmethodID m_newSpeechRecognitionMethodID;
    jmethodID m_deleteSpeechRecognitionMethodID;
    jmethodID m_startSpeechRecognitionMethodID;
    jmethodID m_stopSpeechRecognitionMethodID;
    jmethodID m_newVideoMethodID;
    jmethodID m_deleteVideoMethodID;
    jmethodID m_setVideoSrcMethodID;
    jmethodID m_dispatchVideoPlaybackEventMethodID;
    jmethodID m_setVideoVolumeMethodID;
    jmethodID m_setVideoLoopMethodID;
    jmethodID m_setVideoCurrentTimeMethodID;
    jmethodID m_getVideoDurationMethodID;
    jmethodID m_getVideoCurrentTimeMethodID;
    jmethodID m_getVideoWidthMethodID;
    jmethodID m_getVideoHeightMethodID;

    bool m_reset = false;

    // Do not allow copy of instances.
    VRWebGLCommandProcessor(const VRWebGLCommandProcessor&) = delete;
    VRWebGLCommandProcessor& operator=(const VRWebGLCommandProcessor&) = delete;

    // This class is a singleton
    VRWebGLCommandProcessor();

public:
    static VRWebGLCommandProcessor* getInstance();

    ~VRWebGLCommandProcessor();
    
    void startFrame();

    void* queueVRWebGLCommandForProcessing(const std::shared_ptr<VRWebGLCommand>& vrWebGLCommand);
    
    void endFrame();
    
    void update();

    void updateSurfaceTexture(GLuint textureId);
    
    void renderFrame(bool process = true);

    // The following methods should only be called from the OpenGL thread, so no mutex lock is implemented.
    void setMatrixUniformLocationForName(const GLuint program, const GLint location, const std::string& name);

    bool isProjectionMatrixUniformLocation(const GLuint program, const GLint location) const;

    bool isModelViewMatrixUniformLocation(const GLuint program, const GLint location) const;

    bool isModelViewProjectionMatrixUniformLocation(const GLuint program, const GLint location) const;

    void setViewAndProjectionMatrices(const GLfloat* projectionMatrix, const GLfloat* modelViewMatrix);

    const GLfloat* getProjectionMatrix() const;

    const GLfloat* getViewMatrix() const;

    const GLfloat* getViewProjectionMatrix() const;

    void setCurrentThreadName(const std::string& name);

    const std::string& getCurrentThreadName() const;    

    void setFramebuffer(const GLuint framebuffer);

    GLuint getFramebuffer() const;

    void setViewport(GLint x, GLint y, GLsizei width, GLsizei height);

    void getViewport(GLint& x, GLint& y, GLsizei& width, GLsizei& height) const;    

    void setCameraWorldMatrix(const GLfloat* cameraWorldMatrix);

    const GLfloat* getCameraWorldMatrix() const;    

    const GLfloat* getCameraWorldMatrixWithTranslationOnly() const;

    void reset();  

    bool m_synchronousVRWebGLCommandBeenProcessedInUpdate() const;

    void setupJNI(JNIEnv* jniEnv, jobject mainActivityJObject);

    JNIEnv* getJNIEnv() const;

    const jobject getMainActivityJObject() const;

    std::shared_ptr<VRWebGLSurfaceTexture> newSurfaceTexture();

    void deleteSurfaceTexture(const std::shared_ptr<VRWebGLSurfaceTexture>& surfaceTexture);

    std::shared_ptr<VRWebGLSurfaceTexture> findSurfaceTextureByTextureId(unsigned textureId);   

    jmethodID getNewWebViewMethodID() const;

    jmethodID getDeleteWebViewMethodID() const;

    jmethodID getSetWebViewSrcMethodID() const;

    jmethodID getDispatchWebViewTouchEventMethodID() const;

    jmethodID getDispatchWebViewNavigationEventMethodID() const;

    jmethodID getDispatchWebViewKeyboardEventMethodID() const;

    jmethodID getDispatchWebViewCursorEventMethodID() const;

    jmethodID getSetWebViewTransparentMethodID() const;

    jmethodID getSetWebViewScaleMethodID() const;

    jmethodID getNewSpeechRecognitionMethodID() const;

    jmethodID getDeleteSpeechRecognitionMethodID() const;

    jmethodID getStartSpeechRecognitionMethodID() const;

    jmethodID getStopSpeechRecognitionMethodID() const;

    jmethodID getNewVideoMethodID() const;

    jmethodID getDeleteVideoMethodID() const;

    jmethodID getSetVideoSrcMethodID() const;

    jmethodID getDispatchVideoPlaybackEventMethodID() const;

    jmethodID getSetVideoVolumeMethodID() const;

    jmethodID getSetVideoLoopMethodID() const;

    jmethodID getSetVideoCurrentTimeMethodID() const;

    jmethodID getGetVideoDurationMethodID() const;

    jmethodID getGetVideoCurrentTimeMethodID() const;

    jmethodID getGetVideoWidthMethodID() const;
    
    jmethodID getGetVideoHeightMethodID() const;
    
    // These methods will be implemented where they can provide the requested functionality. Most likely in the Oculus SDK implementation part.
    // TODO: Try to get rid of as many as possible and use VRWebGLCommands instead!
    void getPose(VRWebGLPose& pose);
    void getEyeParameters(const std::string& eye, VRWebGLEyeParameters& eyeParameters);
    void setCameraProjectionMatrix(GLfloat* cameraProjectionMatrix);
    void setRenderEnabled(bool flag);
    std::shared_ptr<blink::WebGamepad> getGamepad();    
};

// =====================================================================================
// =====================================================================================

class VRWebGLCommand_newWebView: public VRWebGLCommand
{
private:
    GLuint textureId;
    
    VRWebGLCommand_newWebView();

public:
    static std::shared_ptr<VRWebGLCommand_newWebView> newInstance();

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_deleteWebView: public VRWebGLCommand
{
private:
    GLuint textureId;
    
    VRWebGLCommand_deleteWebView(GLuint textureId);

public:
    static std::shared_ptr<VRWebGLCommand_deleteWebView> newInstance(GLuint textureId);

    bool isForUpdate() const;
    
    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_setWebViewSrc: public VRWebGLCommand
{
private:
    GLuint textureId;
    std::string src;
    
    VRWebGLCommand_setWebViewSrc(GLuint textureId, const std::string& src);

public:
    static std::shared_ptr<VRWebGLCommand_setWebViewSrc> newInstance(GLuint textureId, const std::string& src);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

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
    
    VRWebGLCommand_dispatchWebViewTouchEvent(GLuint textureId, Event event, float x, float y);

public:
    static std::shared_ptr<VRWebGLCommand_dispatchWebViewTouchEvent> newInstance(GLuint textureId, Event event, float x, float y);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

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
    
    VRWebGLCommand_dispatchWebViewNavigationEvent(GLuint textureId, Event event);

public:
    static std::shared_ptr<VRWebGLCommand_dispatchWebViewNavigationEvent> newInstance(GLuint textureId, Event event);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

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
        static std::map<int, int> jsKeyCodeToJavaKeyCode;

    VRWebGLCommand_dispatchWebViewKeyboardEvent(GLuint textureId, Event event, int keycode);

public:
    static std::shared_ptr<VRWebGLCommand_dispatchWebViewKeyboardEvent> newInstance(GLuint textureId, Event event, int keycode);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_dispatchWebViewCursorEvent: public VRWebGLCommand
{
public:
    enum Event
    {
        CURSOR_ENTER = 1,
        CURSOR_MOVE = 2,
        CURSOR_EXIT = 3
    };

private:
    GLuint textureId;
    Event event;
    float x;
    float y;
    
    VRWebGLCommand_dispatchWebViewCursorEvent(GLuint textureId, Event event, float x, float y);

public:
    static std::shared_ptr<VRWebGLCommand_dispatchWebViewCursorEvent> newInstance(GLuint textureId, Event event, float x, float y);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_setWebViewTransparent: public VRWebGLCommand
{
private:
    GLuint textureId;
    bool transparent;
    
    VRWebGLCommand_setWebViewTransparent(GLuint textureId, bool transparent);

public:
    static std::shared_ptr<VRWebGLCommand_setWebViewTransparent> newInstance(GLuint textureId, bool transparent);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_setWebViewScale: public VRWebGLCommand
{
private:
    GLuint textureId;
    int scale;
    bool loadWithOverviewMode;
    bool useWideViewPort;
    
    VRWebGLCommand_setWebViewScale(GLuint textureId, int scale, bool loadWithOverviewMode, bool useWideViewPort);

public:
    static std::shared_ptr<VRWebGLCommand_setWebViewScale> newInstance(GLuint textureId, int scale, bool loadWithOverviewMode, bool useWideViewPort);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_newSpeechRecognition: public VRWebGLCommand
{
private:
    long id;
    
    VRWebGLCommand_newSpeechRecognition(long id);

public:
    static std::shared_ptr<VRWebGLCommand_newSpeechRecognition> newInstance(long id);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_deleteSpeechRecognition: public VRWebGLCommand
{
private:
    long id;
    
    VRWebGLCommand_deleteSpeechRecognition(long id);

public:
    static std::shared_ptr<VRWebGLCommand_deleteSpeechRecognition> newInstance(long id);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_startSpeechRecognition: public VRWebGLCommand
{
private:
    long id;
    
    VRWebGLCommand_startSpeechRecognition(long id);

public:
    static std::shared_ptr<VRWebGLCommand_startSpeechRecognition> newInstance(long id);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_stopSpeechRecognition: public VRWebGLCommand
{
private:
    long id;
    
    VRWebGLCommand_stopSpeechRecognition(long id);

public:
    static std::shared_ptr<VRWebGLCommand_stopSpeechRecognition> newInstance(long id);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_newVideo: public VRWebGLCommand
{
private:
    GLuint textureId;
    
    VRWebGLCommand_newVideo();

public:
    static std::shared_ptr<VRWebGLCommand_newVideo> newInstance();

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_deleteVideo: public VRWebGLCommand
{
private:
    GLuint textureId;
    
    VRWebGLCommand_deleteVideo(GLuint textureId);

public:
    static std::shared_ptr<VRWebGLCommand_deleteVideo> newInstance(GLuint textureId);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_setVideoSrc: public VRWebGLCommand
{
private:
    GLuint textureId;
    std::string src;
    
    VRWebGLCommand_setVideoSrc(GLuint textureId, const std::string& src);

public:
    static std::shared_ptr<VRWebGLCommand_setVideoSrc> newInstance(GLuint textureId, const std::string& src);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_dispatchVideoPlaybackEvent: public VRWebGLCommand
{
public:
    enum Event
    {
        PLAY = 1,
        PAUSE = 2
    };

private:
    GLuint textureId;
    Event event;
    float volume;
    bool loop;
    
    VRWebGLCommand_dispatchVideoPlaybackEvent(GLuint textureId, Event event, float volume, bool loop);

public:
    static std::shared_ptr<VRWebGLCommand_dispatchVideoPlaybackEvent> newInstance(GLuint textureId, Event event, float volume, bool loop);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_setVideoVolume: public VRWebGLCommand
{
private:
    GLuint textureId;
    float volume;
    
    VRWebGLCommand_setVideoVolume(GLuint textureId, float volume);

public:
    static std::shared_ptr<VRWebGLCommand_setVideoVolume> newInstance(GLuint textureId, float volume);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_setVideoLoop: public VRWebGLCommand
{
private:
    GLuint textureId;
    bool loop;
    
    VRWebGLCommand_setVideoLoop(GLuint textureId, bool loop);

public:
    static std::shared_ptr<VRWebGLCommand_setVideoLoop> newInstance(GLuint textureId, bool loop);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_setVideoCurrentTime: public VRWebGLCommand
{
private:
    GLuint textureId;
    float currentTime;
    
    VRWebGLCommand_setVideoCurrentTime(GLuint textureId, float currentTime);

public:
    static std::shared_ptr<VRWebGLCommand_setVideoCurrentTime> newInstance(GLuint textureId, float currentTime);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_getVideoDuration: public VRWebGLCommand
{
private:
    GLuint textureId;
    float duration = 0;
    
    VRWebGLCommand_getVideoDuration(GLuint textureId);

public:
    static std::shared_ptr<VRWebGLCommand_getVideoDuration> newInstance(GLuint textureId);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_getVideoCurrentTime: public VRWebGLCommand
{
private:
    GLuint textureId;
    float currentTime = 0;
    
    VRWebGLCommand_getVideoCurrentTime(GLuint textureId);

public:
    static std::shared_ptr<VRWebGLCommand_getVideoCurrentTime> newInstance(GLuint textureId);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_getVideoWidth: public VRWebGLCommand
{
private:
    GLuint textureId;
    int width = 0;
    
    VRWebGLCommand_getVideoWidth(GLuint textureId);

public:
    static std::shared_ptr<VRWebGLCommand_getVideoWidth> newInstance(GLuint textureId);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};

// =====================================================================================

class VRWebGLCommand_getVideoHeight: public VRWebGLCommand
{
private:
    GLuint textureId;
    int height = 0;
    
    VRWebGLCommand_getVideoHeight(GLuint textureId);

public:
    static std::shared_ptr<VRWebGLCommand_getVideoHeight> newInstance(GLuint textureId);

    bool isForUpdate() const;

    bool isSynchronous() const;

    bool canBeProcessedImmediately() const;
    
    void* process();
    
    std::string name() const;
};


#endif // VRWebGLCommandProcessor_h