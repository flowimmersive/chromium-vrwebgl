#ifndef VRWebGLCommandProcessor_h
#define VRWebGLCommandProcessor_h

#include <pthread.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
// // #include <GLES3/gl3ext.h>

#include <string>
#include <deque>
#include <memory>

class VRWebGLCommand;
class VRWebGLPose;
class VRWebGLEyeParameters;

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

    // These methods will be implemented where they can provide the requested functionality. Most likely in the Oculus SDK implementation part.
    void getPose(VRWebGLPose& pose);
    void getEyeParameters(VRWebGLEyeParameters& eyeParameters);
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
};

#endif // VRWebGLCommandProcessor_h