#include "modules/vr/VRWebGLCommandProcessor.h"

#include "modules/vr/VRWebGLCommand.h"
#include "modules/vr/VRWebGLMath.h"

#include <android/log.h>
#include <algorithm>

#define LOG_TAG "VRWebGL"
#ifdef VRWEBGL_SHOW_LOG
    #define ALOGV(...) __android_log_print( ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__ )
#else
    #define ALOGV(...)
#endif
#define ALOGE(...) __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__ )

VRWebGLCommandProcessor::~VRWebGLCommandProcessor()
{
}

VRWebGLCommandProcessor* VRWebGLCommandProcessor::getInstance()
{
    return VRWebGLCommandProcessorImpl::getInstance();
}

VRWebGLCommandProcessorImpl::VRWebGLProgramAndUniformLocation::VRWebGLProgramAndUniformLocation()
{
}

VRWebGLCommandProcessorImpl::VRWebGLProgramAndUniformLocation::VRWebGLProgramAndUniformLocation(const GLuint program, const GLint location): m_program(program), m_location(location)
{
}

bool VRWebGLCommandProcessorImpl::VRWebGLProgramAndUniformLocation::operator==(const VRWebGLProgramAndUniformLocation& other) const
{
    return m_program == other.m_program && m_location == other.m_location;
}

VRWebGLCommandProcessorImpl::VRWebGLCommandProcessorImpl()
{
    pthread_mutexattr_t	attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&m_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&m_synchronousVRWebGLCommandProcessed, NULL);
    pthread_cond_init(&m_bothEyesRendered, NULL);

    reset();
}

VRWebGLCommandProcessor* VRWebGLCommandProcessorImpl::getInstance()
{
    static VRWebGLCommandProcessorImpl singleInstance;
    return &singleInstance;
}

VRWebGLCommandProcessorImpl::~VRWebGLCommandProcessorImpl()
{
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_synchronousVRWebGLCommandProcessed);
    pthread_cond_destroy(&m_bothEyesRendered);
}

void VRWebGLCommandProcessorImpl::startFrame()
{
    pthread_mutex_lock( &m_mutex );
    
    if (m_insideAFrame)
    {
        std::string message = "Calling startFrame while inside an existing frame!";
        ALOGE("VRWebGL: %s", message.c_str());
        //			throw new IllegalStateException(message);
    }
    
    m_insideAFrame = true;

    pthread_mutex_unlock( &m_mutex );
}

void* VRWebGLCommandProcessorImpl::queueVRWebGLCommandForProcessing(const std::shared_ptr<VRWebGLCommand>& vrWebGLCommand)
{
    void* result = 0;
    
    pthread_mutex_lock( &m_mutex );

    ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::queueVRWebGLCommandForProcessing(%s) begins", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
    
    if (vrWebGLCommand->isSynchronous())
    {
        if (vrWebGLCommand->canBeProcessedImmediately())
        {
            result = vrWebGLCommand->process();
        }
        else
        {
            // The command is synchronous, so:
            // 1.- Store the command so it can be executed in the OpenGL thread (check the update method).
            // 2.- Stack all the commands up until now to be called too!
            // 3.- Wait for the synchronous command to be processed.
            // 4.- Return the result of the call
            m_synchronousVRWebGLCommand = vrWebGLCommand;

            ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::queueVRWebGLCommandForProcessing %d commands in the batch before adding", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
            m_vrWebGLCommandQueueBatch.insert(m_vrWebGLCommandQueueBatch.end(), m_vrWebGLCommandQueue.begin(), m_vrWebGLCommandQueue.end());
            m_vrWebGLCommandQueue.clear();
            ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::queueVRWebGLCommandForProcessing %d commands in the batch after adding", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());

            if (m_insideAFrame)
            {
                ALOGE("VRWebGL: A synchronous call has been made inside a frame. Not a great idea. Many of these calls might slow down the JS process as they block the JS thread until the result of the call is provided from the OpenGL thread.");
            }
            
            m_synchronousVRWebGLCommandResult = 0;

            // while ( !m_synchronousVRWebGLCommandResult )
            // {
                pthread_cond_wait( &m_synchronousVRWebGLCommandProcessed, &m_mutex );
            // }
            
            result = m_synchronousVRWebGLCommandResult;
        }
    }
    else
    {
        vrWebGLCommand->setInsideAFrame(m_insideAFrame);
        m_vrWebGLCommandQueue.push_back(vrWebGLCommand);
    }
    
    ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::queueVRWebGLCommandForProcessing(%s) ends", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());

    pthread_mutex_unlock( &m_mutex );
    
    return result;
}

void VRWebGLCommandProcessorImpl::endFrame()
{
    pthread_mutex_lock( &m_mutex );
    
    if (!m_insideAFrame)
    {
        std::string message = "Calling endFrame outside of a frame!";
        ALOGE("VRWebGL: %s", message.c_str());
        //			throw new IllegalStateException(message);
    }

    m_insideAFrame = false;
    
    while(!m_currentBatchRenderedForBothEyes || m_indexOfEyeBeingRendered == 0)
    {
        pthread_cond_wait( &m_bothEyesRendered, &m_mutex );
    }

    if (!m_vrWebGLCommandQueue.empty())
    {
        ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::endFrame %d commands in the batch", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
        m_vrWebGLCommandQueueBatch = m_vrWebGLCommandQueue;
        m_vrWebGLCommandQueue.clear();
        m_currentBatchRenderedForBothEyes = false;
    }

    m_synchronousVRWebGLCommandProcessedInUpdate = false;
    
    pthread_mutex_unlock( &m_mutex );
}

void VRWebGLCommandProcessorImpl::update()
{
    pthread_mutex_lock( &m_mutex );

    // Update all the surface textures
    m_surfaceTextures.update();

    // If there is a synchronous VRWebGLCommand, execute it, store the result and notify the waiting thread
    if (m_synchronousVRWebGLCommand)
    {
        // But before, execute all the commands that have been batched up until the execution of the synchronous command.
        if (!m_vrWebGLCommandQueueBatch.empty())
        {
            ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::update %d commands in the batch before processing", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
            for (unsigned int i = 0; i < m_vrWebGLCommandQueueBatch.size(); i++)
            {
                std::shared_ptr<VRWebGLCommand> vrWebGLCommand = m_vrWebGLCommandQueueBatch[i];

                ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::update processing command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
                vrWebGLCommand->process();
                ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::update processed command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());

                if (!vrWebGLCommand->insideAFrame())
                {
                    ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::update removing outside of a frame command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
                    m_vrWebGLCommandQueueBatch.erase(m_vrWebGLCommandQueueBatch.begin() + i);
                    i--;
                    ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::update removed outside of a frame command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
                }
            }
            ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::update %d commands in the batch after process", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
        }

        ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::update processing synchronous command %s", getCurrentThreadName().c_str(), m_synchronousVRWebGLCommand->name().c_str());
        m_synchronousVRWebGLCommandResult = m_synchronousVRWebGLCommand->process();
        ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::update processed synchronous command %s", getCurrentThreadName().c_str(), m_synchronousVRWebGLCommand->name().c_str());
        m_synchronousVRWebGLCommand.reset();
        pthread_cond_broadcast( &m_synchronousVRWebGLCommandProcessed );

        m_synchronousVRWebGLCommandProcessedInUpdate = true;
    }
    
    pthread_mutex_unlock( &m_mutex );
}

void VRWebGLCommandProcessorImpl::renderFrame(bool process)
{
    pthread_mutex_lock( &m_mutex );

    if (process && !m_synchronousVRWebGLCommand && !m_vrWebGLCommandQueueBatch.empty())
    {
        ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::renderFrame %d commands in the batch before processing", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
        for (unsigned int i = 0; i < m_vrWebGLCommandQueueBatch.size(); i++)
        {
            std::shared_ptr<VRWebGLCommand> vrWebGLCommand = m_vrWebGLCommandQueueBatch[i];

            ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::renderFrame processing command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
            vrWebGLCommand->process();
            ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::renderFrame processed command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());

            if (!vrWebGLCommand->insideAFrame() && m_indexOfEyeBeingRendered == -1)
            {
                ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::renderFrame removing outside of a frame command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
                m_vrWebGLCommandQueueBatch.erase(m_vrWebGLCommandQueueBatch.begin() + i);
                i--;
                ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::renderFrame removed outside of a frame command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
            }
        }
        ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::renderFrame %d commands in the batch after process", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
    }

    // Increment the indexOfEyeBeingRendered to measure when the 2 calls, one for each eye, are made so we can notify to whoever is waiting for this condition
    m_indexOfEyeBeingRendered++;
    if (m_indexOfEyeBeingRendered == 1)
    {        
        m_indexOfEyeBeingRendered = -1;
        m_currentBatchRenderedForBothEyes = true;
        pthread_cond_broadcast( &m_bothEyesRendered );
    }
    
    pthread_mutex_unlock( &m_mutex );
}

// The following methods should only be called from the OpenGL thread, so no mutex lock is implemented.
void VRWebGLCommandProcessorImpl::setMatrixUniformLocationForName(const GLuint program, const GLint location, const std::string& name)
{
    if (std::find(m_projectionMatrixUniformNames.begin(), m_projectionMatrixUniformNames.end(), name) != m_projectionMatrixUniformNames.end())
    {
        ALOGV("VRWebGL: projection matrix uniform '%s' found. Storing location '%d' for program '%d'", name.c_str(), location, program);
        m_projectionMatrixProgramAndUniformLocations.push_back(VRWebGLProgramAndUniformLocation(program, location));
    }
    else if (std::find(m_modelViewMatrixUniformNames.begin(), m_modelViewMatrixUniformNames.end(), name) != m_modelViewMatrixUniformNames.end())
    {
        ALOGV("VRWebGL: modelview matrix uniform '%s' found. Storing location '%d' for program '%d'", name.c_str(), location, program);
        m_modelViewMatrixProgramAndUniformLocations.push_back(VRWebGLProgramAndUniformLocation(program, location));
    }
    else if (std::find(m_modelViewProjectionMatrixUniformNames.begin(), m_modelViewProjectionMatrixUniformNames.end(), name) != m_modelViewProjectionMatrixUniformNames.end())
    {
        ALOGE("VRWebGL: modelviewprojection matrix uniform '%s' found. Storing location '%d' for program '%d'", name.c_str(), location, program);
        m_modelViewProjectionMatrixProgramAndUniformLocations.push_back(VRWebGLProgramAndUniformLocation(program, location));
    }
}

bool VRWebGLCommandProcessorImpl::isProjectionMatrixUniformLocation(const GLuint program, const GLint location) const
{
    return std::find(m_projectionMatrixProgramAndUniformLocations.begin(), m_projectionMatrixProgramAndUniformLocations.end(), VRWebGLProgramAndUniformLocation(program, location)) != m_projectionMatrixProgramAndUniformLocations.end();
}

bool VRWebGLCommandProcessorImpl::isModelViewMatrixUniformLocation(const GLuint program, const GLint location) const
{
    return std::find(m_modelViewMatrixProgramAndUniformLocations.begin(), m_modelViewMatrixProgramAndUniformLocations.end(), VRWebGLProgramAndUniformLocation(program, location)) != m_modelViewMatrixProgramAndUniformLocations.end();
}

bool VRWebGLCommandProcessorImpl::isModelViewProjectionMatrixUniformLocation(const GLuint program, const GLint location) const
{
    return std::find(m_modelViewProjectionMatrixProgramAndUniformLocations.begin(), m_modelViewProjectionMatrixProgramAndUniformLocations.end(), VRWebGLProgramAndUniformLocation(program, location)) != m_modelViewProjectionMatrixProgramAndUniformLocations.end();
}

void VRWebGLCommandProcessorImpl::setViewAndProjectionMatrices(const GLfloat* projectionMatrix, const GLfloat* viewMatrix)
{
    memcpy(m_projectionMatrix, projectionMatrix, sizeof(m_projectionMatrix));
    memcpy(m_viewMatrix, viewMatrix, sizeof(m_viewMatrix));
    VRWebGL_multiplyMatrices4(m_viewMatrix, m_projectionMatrix, m_viewProjectionMatrix);
}

const GLfloat* VRWebGLCommandProcessorImpl::getProjectionMatrix() const
{
    return m_projectionMatrix;
}

const GLfloat* VRWebGLCommandProcessorImpl::getViewMatrix() const
{
    return m_viewMatrix;
}

const GLfloat* VRWebGLCommandProcessorImpl::getViewProjectionMatrix() const
{
    return m_viewProjectionMatrix;
}

void VRWebGLCommandProcessorImpl::setCurrentThreadName(const std::string& name)
{
    pthread_t currentThread = pthread_self();
    for (unsigned int i = 0; i < 10; i++)
    {
        if (m_threadIds[i] == 0) {
            m_threadIds[i] = currentThread;
            m_threadNames[i] = name;
            break;
        }
    }
}

const std::string& VRWebGLCommandProcessorImpl::getCurrentThreadName() const
{
    static std::string emptyString = "";
    pthread_t currentThread = pthread_self();
    for (unsigned int i = 0; i < 10; i++)
    {
        if (m_threadIds[i] == currentThread) {
            return m_threadNames[i];
        }
    }
    return emptyString;
}


void VRWebGLCommandProcessorImpl::setFramebuffer(const GLuint framebuffer)
{
    m_framebuffer = framebuffer;
}

GLuint VRWebGLCommandProcessorImpl::getFramebuffer() const
{
    return m_framebuffer;
}

void VRWebGLCommandProcessorImpl::setViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

void VRWebGLCommandProcessorImpl::getViewport(GLint& x, GLint& y, GLsizei& width, GLsizei& height) const 
{
    x = m_x;
    y = m_y;
    width = m_width;
    height = m_height;
}    

void VRWebGLCommandProcessorImpl::setCameraWorldMatrix(const GLfloat* cameraWorldMatrix)
{
    memcpy(m_cameraWorldMatrix, cameraWorldMatrix, sizeof(m_cameraWorldMatrix));

    m_cameraWorldMatrixWithTranslationOnly[12] = -cameraWorldMatrix[12];
    m_cameraWorldMatrixWithTranslationOnly[13] = -cameraWorldMatrix[13];
    m_cameraWorldMatrixWithTranslationOnly[14] = -cameraWorldMatrix[14];
}

const GLfloat* VRWebGLCommandProcessorImpl::getCameraWorldMatrix() const
{
    return m_cameraWorldMatrix;
}

const GLfloat* VRWebGLCommandProcessorImpl::getCameraWorldMatrixWithTranslationOnly() const
{
    return m_cameraWorldMatrixWithTranslationOnly;
}

void VRWebGLCommandProcessorImpl::reset() 
{
    pthread_mutex_lock( &m_mutex );

    m_vrWebGLCommandQueue.clear();
    m_vrWebGLCommandQueueBatch.clear();
    m_insideAFrame = false;
    m_currentBatchRenderedForBothEyes = true;
    m_synchronousVRWebGLCommand.reset();
    m_synchronousVRWebGLCommandResult = 0;
    m_indexOfEyeBeingRendered = -1;

    m_projectionMatrixProgramAndUniformLocations.clear();
    m_modelViewMatrixProgramAndUniformLocations.clear();

    m_projectionMatrix[16];
    m_viewMatrix[16];
    m_cameraWorldMatrix[16];
    m_cameraWorldMatrixWithTranslationOnly[16];

    m_framebuffer = 0;

    // Add default projection and modelview matrix uniform names to look for.

    // PROJECTION
    m_projectionMatrixUniformNames.clear();
    
    // WebGLLessons
    m_projectionMatrixUniformNames.push_back("uProjectionMatrix");
    m_projectionMatrixUniformNames.push_back("uPMatrix");

    // ThreeJS
    m_projectionMatrixUniformNames.push_back("projectionMatrix");
    
    // PlayCanvas
    m_projectionMatrixUniformNames.push_back("matrix_projection"); 
    m_projectionMatrixUniformNames.push_back("matrix_viewProjection");
    
    // Sketchfab
    m_projectionMatrixUniformNames.push_back("ProjectionMatrix");
    
    // Goo
    m_projectionMatrixUniformNames.push_back("projectionMatrix"); 
    m_projectionMatrixUniformNames.push_back("viewProjectionMatrix"); 

    // Soft shadow example
    m_projectionMatrixUniformNames.push_back("camProj"); 


    // MODELVIEW
    m_modelViewMatrixUniformNames.clear();
    
    // WebGLLessons
    m_modelViewMatrixUniformNames.push_back("uMVMatrix");
    
    // ThreeJS
    m_modelViewMatrixUniformNames.push_back("modelViewMatrix"); 
    
    // PlayCanvas
    m_modelViewMatrixUniformNames.push_back("matrix_view"); 
    m_modelViewMatrixUniformNames.push_back("matrix_model");
    
    // Sketchfab
    m_modelViewMatrixUniformNames.push_back("ModelViewMatrix"); 
    
    // Goo
    m_modelViewMatrixUniformNames.push_back("viewMatrix"); 
    m_modelViewMatrixUniformNames.push_back("worldMatrix"); 

    // Soft shadow example
    m_modelViewMatrixUniformNames.push_back("camView"); 


    // MODELVIEWPROJECTION
    m_modelViewProjectionMatrixUniformNames.push_back("uMatMVP");

    // Reset the thread ids
    memset(m_threadIds, 0, sizeof(m_threadIds));

    // Initialize all the matrices to identity.
    memset(m_cameraWorldMatrixWithTranslationOnly, 0, sizeof(m_cameraWorldMatrixWithTranslationOnly));
    m_cameraWorldMatrixWithTranslationOnly[0] = m_cameraWorldMatrixWithTranslationOnly[5] = m_cameraWorldMatrixWithTranslationOnly[10] = m_cameraWorldMatrixWithTranslationOnly[15] = 1.0;
    memcpy(m_cameraWorldMatrix, m_cameraWorldMatrixWithTranslationOnly, sizeof(m_cameraWorldMatrixWithTranslationOnly));
    memcpy(m_projectionMatrix, m_cameraWorldMatrixWithTranslationOnly, sizeof(m_cameraWorldMatrixWithTranslationOnly));
    memcpy(m_viewMatrix, m_cameraWorldMatrixWithTranslationOnly, sizeof(m_cameraWorldMatrixWithTranslationOnly));
    memcpy(m_viewProjectionMatrix, m_cameraWorldMatrixWithTranslationOnly, sizeof(m_cameraWorldMatrixWithTranslationOnly));

    pthread_mutex_unlock( &m_mutex );
}

bool VRWebGLCommandProcessorImpl::m_synchronousVRWebGLCommandBeenProcessedInUpdate() const
{
    return m_synchronousVRWebGLCommandProcessedInUpdate;
}

void VRWebGLCommandProcessorImpl::setupJNI(JNIEnv* jniEnv, jobject mainActivityJObject)
{
    m_jniEnv = jniEnv;
    m_jniEnv->GetJavaVM( &m_javaVM );
    m_javaVM->AttachCurrentThread( &m_jniEnv, NULL );
    m_mainActivityJObject = m_jniEnv->NewGlobalRef( mainActivityJObject );
    m_mainActivityJClass = m_jniEnv->GetObjectClass(m_mainActivityJObject);
    
    m_newWebViewMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "newWebView", "(Landroid/graphics/SurfaceTexture;I)V");
    m_deleteWebViewMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "deleteWebView", "(Landroid/graphics/SurfaceTexture;)V");
    m_setWebViewSrcMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "setWebViewSrc", "(Landroid/graphics/SurfaceTexture;Ljava/lang/String;)V");
    m_dispatchWebViewTouchEventMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "dispatchWebViewTouchEvent", "(Landroid/graphics/SurfaceTexture;IFF)V");
    m_dispatchWebViewNavigationEventMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "dispatchWebViewNavigationEvent", "(Landroid/graphics/SurfaceTexture;I)V");

    // appThread->logHeapUsageMethodID = env->GetMethodID(appThread->activityObjectJClass, "logHeapUsageFromNative", "()V");    
    // appThread->seekToMethodId = env->GetMethodID(appThread->activityObjectJClass, "seekTo", "(Landroid/graphics/SurfaceTexture;I)V");
    // appThread->pauseVideoMethodId = env->GetMethodID(appThread->activityObjectJClass, "pauseVideo", "(Landroid/graphics/SurfaceTexture;)V");
    // appThread->stopVideoMethodId = env->GetMethodID(appThread->activityObjectJClass, "stopVideo", "(Landroid/graphics/SurfaceTexture;)V");
    // appThread->playVideoOnUIThreadMethodId = env->GetMethodID(appThread->activityObjectJClass, "playVideoOnUIThread", "(Landroid/graphics/SurfaceTexture;FZ)V");
    // appThread->setVideoSrcOnUIThreadMethodId = env->GetMethodID(appThread->activityObjectJClass, "setVideoSrcOnUIThread", "(Landroid/graphics/SurfaceTexture;Ljava/lang/String;)V");
    // appThread->newVideoMethodId = env->GetMethodID(appThread->activityObjectJClass, "newVideo", "(Landroid/graphics/SurfaceTexture;I)V");
    // appThread->deleteVideoMethodId = env->GetMethodID(appThread->activityObjectJClass, "deleteVideo", "(Landroid/graphics/SurfaceTexture;)V");
    // appThread->setVideoVolumeMethodId = env->GetMethodID(appThread->activityObjectJClass, "setVideoVolume", "(Landroid/graphics/SurfaceTexture;F)V");
    // appThread->setVideoLoopMethodId = env->GetMethodID(appThread->activityObjectJClass, "setVideoLoop", "(Landroid/graphics/SurfaceTexture;Z)V");
    // appThread->getVideoDurationMethodId = env->GetMethodID(appThread->activityObjectJClass, "getVideoDuration", "(Landroid/graphics/SurfaceTexture;)I");
    // appThread->getVideoWidthMethodId = env->GetMethodID(appThread->activityObjectJClass, "getVideoWidth", "(Landroid/graphics/SurfaceTexture;)I");
    // appThread->getVideoHeightMethodId = env->GetMethodID(appThread->activityObjectJClass, "getVideoHeight", "(Landroid/graphics/SurfaceTexture;)I");
    // appThread->getVideoCurrentTimeMethodId = env->GetMethodID(appThread->activityObjectJClass, "getVideoCurrentTime", "(Landroid/graphics/SurfaceTexture;)I");

    // TODO: When to destroy all this? It should be during     
}

JNIEnv* VRWebGLCommandProcessorImpl::getJNIEnv() const
{
    return m_jniEnv;
}

std::shared_ptr<VRWebGLSurfaceTexture> VRWebGLCommandProcessorImpl::newSurfaceTexture()
{
    return m_surfaceTextures.newSurfaceTexture(m_jniEnv);
}

void VRWebGLCommandProcessorImpl::deleteSurfaceTexture(const std::shared_ptr<VRWebGLSurfaceTexture>& surfaceTexture)
{
    m_surfaceTextures.deleteSurfaceTexture(surfaceTexture);
}

std::shared_ptr<VRWebGLSurfaceTexture> VRWebGLCommandProcessorImpl::findSurfaceTextureByTextureId(unsigned textureId)
{
    return m_surfaceTextures.findSurfaceTextureByTextureId(textureId);
}

const jobject VRWebGLCommandProcessorImpl::getMainActivityJObject() const
{
    return m_mainActivityJObject;
}

jmethodID VRWebGLCommandProcessorImpl::getNewWebViewMethodID() const
{
    return m_newWebViewMethodID;
}

jmethodID VRWebGLCommandProcessorImpl::getDeleteWebViewMethodID() const
{
    return m_deleteWebViewMethodID;
}

jmethodID VRWebGLCommandProcessorImpl::getSetWebViewSrcMethodID() const
{
    return m_setWebViewSrcMethodID;
}

jmethodID VRWebGLCommandProcessorImpl::getDispatchWebViewTouchEventMethodID() const
{
    return m_dispatchWebViewTouchEventMethodID;
}

jmethodID VRWebGLCommandProcessorImpl::getDispatchWebViewNavigationEventMethodID() const
{
    return m_dispatchWebViewNavigationEventMethodID;
}

// =====================================================================================
// =====================================================================================

VRWebGLCommand_newWebView::VRWebGLCommand_newWebView(): textureId(0)
{
}

std::shared_ptr<VRWebGLCommand_newWebView> VRWebGLCommand_newWebView::newInstance()
{
    return std::shared_ptr<VRWebGLCommand_newWebView>(new VRWebGLCommand_newWebView());
}

bool VRWebGLCommand_newWebView::isSynchronous() const
{
    return true;
}

bool VRWebGLCommand_newWebView::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_newWebView::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->newSurfaceTexture();
    textureId = surfaceTexture->getTextureId();
    jmethodID newWebViewMethodID = VRWebGLCommandProcessor::getInstance()->getNewWebViewMethodID();
    JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
    jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
    jniEnv->CallVoidMethod( mainActivityJObject, newWebViewMethodID, surfaceTexture->getJavaObject(), textureId );
    return &textureId;
}

std::string VRWebGLCommand_newWebView::name() const
{
    return "newWebView";
}

// =====================================================================================

VRWebGLCommand_deleteWebView::VRWebGLCommand_deleteWebView(GLuint textureId): textureId(textureId)
{
}

std::shared_ptr<VRWebGLCommand_deleteWebView> VRWebGLCommand_deleteWebView::newInstance(GLuint textureId)
{
    return std::shared_ptr<VRWebGLCommand_deleteWebView>(new VRWebGLCommand_deleteWebView(textureId));
}

bool VRWebGLCommand_deleteWebView::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_deleteWebView::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_deleteWebView::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    jmethodID deleteWebViewMethodID = VRWebGLCommandProcessor::getInstance()->getDeleteWebViewMethodID();
    JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
    jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
    jniEnv->CallVoidMethod( mainActivityJObject, deleteWebViewMethodID, surfaceTexture->getJavaObject() );
    return 0;
}

std::string VRWebGLCommand_deleteWebView::name() const
{
    return "deleteWebView";
}

// =====================================================================================

VRWebGLCommand_setWebViewSrc::VRWebGLCommand_setWebViewSrc(GLuint textureId, const std::string& src): textureId(textureId), src(src)
{
}

std::shared_ptr<VRWebGLCommand_setWebViewSrc> VRWebGLCommand_setWebViewSrc::newInstance(GLuint textureId, const std::string& src)
{
    return std::shared_ptr<VRWebGLCommand_setWebViewSrc>(new VRWebGLCommand_setWebViewSrc(textureId, src));
}

bool VRWebGLCommand_setWebViewSrc::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_setWebViewSrc::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_setWebViewSrc::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    jmethodID setWebViewSrcMethodID = VRWebGLCommandProcessor::getInstance()->getSetWebViewSrcMethodID();
    JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
    jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
    jstring srcJString = jniEnv->NewStringUTF( src.c_str() );
    jniEnv->CallVoidMethod( mainActivityJObject, setWebViewSrcMethodID, surfaceTexture->getJavaObject(), srcJString );
    jniEnv->DeleteLocalRef( srcJString );
    return 0;
}

std::string VRWebGLCommand_setWebViewSrc::name() const
{
    return "setWebViewSrc";
}

// =====================================================================================

VRWebGLCommand_dispatchWebViewTouchEvent::VRWebGLCommand_dispatchWebViewTouchEvent(GLuint textureId, Event event, float x, float y): textureId(textureId), event(event), x(x), y(y)
{
}

std::shared_ptr<VRWebGLCommand_dispatchWebViewTouchEvent> VRWebGLCommand_dispatchWebViewTouchEvent::newInstance(GLuint textureId, Event event, float x, float y)
{
    return std::shared_ptr<VRWebGLCommand_dispatchWebViewTouchEvent>(new VRWebGLCommand_dispatchWebViewTouchEvent(textureId, event, x, y));
}

bool VRWebGLCommand_dispatchWebViewTouchEvent::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_dispatchWebViewTouchEvent::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_dispatchWebViewTouchEvent::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    jmethodID dispatchWebViewTouchEventMethodID = VRWebGLCommandProcessor::getInstance()->getDispatchWebViewTouchEventMethodID();
    JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
    jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
    jniEnv->CallVoidMethod( mainActivityJObject, dispatchWebViewTouchEventMethodID, surfaceTexture->getJavaObject(), (jint)event, x, y );
    return 0;
}

std::string VRWebGLCommand_dispatchWebViewTouchEvent::name() const
{
    return "dispatchWebViewTouchEvent";
}

// =====================================================================================

VRWebGLCommand_dispatchWebViewNavigationEvent::VRWebGLCommand_dispatchWebViewNavigationEvent(GLuint textureId, Event event): textureId(textureId), event(event)
{
}

std::shared_ptr<VRWebGLCommand_dispatchWebViewNavigationEvent> VRWebGLCommand_dispatchWebViewNavigationEvent::newInstance(GLuint textureId, Event event)
{
    return std::shared_ptr<VRWebGLCommand_dispatchWebViewNavigationEvent>(new VRWebGLCommand_dispatchWebViewNavigationEvent(textureId, event));
}

bool VRWebGLCommand_dispatchWebViewNavigationEvent::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_dispatchWebViewNavigationEvent::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_dispatchWebViewNavigationEvent::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    jmethodID dispatchWebViewNavigationEventMethodID = VRWebGLCommandProcessor::getInstance()->getDispatchWebViewNavigationEventMethodID();
    JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
    jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
    jniEnv->CallVoidMethod( mainActivityJObject, dispatchWebViewNavigationEventMethodID, surfaceTexture->getJavaObject(), (jint)event);
    return 0;
}

std::string VRWebGLCommand_dispatchWebViewNavigationEvent::name() const
{
    return "dispatchWebViewNavigationEvent";
}