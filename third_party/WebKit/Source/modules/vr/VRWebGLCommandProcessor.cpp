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

VRWebGLCommandProcessor::VRWebGLProgramAndUniformLocation::VRWebGLProgramAndUniformLocation()
{
}

VRWebGLCommandProcessor::VRWebGLProgramAndUniformLocation::VRWebGLProgramAndUniformLocation(const GLuint program, const GLint location): m_program(program), m_location(location)
{
}

bool VRWebGLCommandProcessor::VRWebGLProgramAndUniformLocation::operator==(const VRWebGLProgramAndUniformLocation& other) const
{
    return m_program == other.m_program && m_location == other.m_location;
}

VRWebGLCommandProcessor::VRWebGLCommandProcessor()
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

VRWebGLCommandProcessor* VRWebGLCommandProcessor::getInstance()
{
    static VRWebGLCommandProcessor singleInstance;
    return &singleInstance;
}

VRWebGLCommandProcessor::~VRWebGLCommandProcessor()
{
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_synchronousVRWebGLCommandProcessed);
    pthread_cond_destroy(&m_bothEyesRendered);
}

void VRWebGLCommandProcessor::startFrame()
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

void* VRWebGLCommandProcessor::queueVRWebGLCommandForProcessing(const std::shared_ptr<VRWebGLCommand>& vrWebGLCommand)
{
    void* result = 0;
    
    pthread_mutex_lock( &m_mutex );

    ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::queueVRWebGLCommandForProcessing(%s) begins", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
    
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

            ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::queueVRWebGLCommandForProcessing %d commands in the batch before adding", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
            m_vrWebGLCommandQueueBatch.insert(m_vrWebGLCommandQueueBatch.end(), m_vrWebGLCommandQueue.begin(), m_vrWebGLCommandQueue.end());
            m_vrWebGLCommandQueue.clear();
            ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::queueVRWebGLCommandForProcessing %d commands in the batch after adding", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());

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
        if (vrWebGLCommand->isForUpdate())
        {
            m_vrWebGLCommandForUpdateQueue.push_back(vrWebGLCommand);
        }
        else
        {
            vrWebGLCommand->setInsideAFrame(m_insideAFrame);
            m_vrWebGLCommandQueue.push_back(vrWebGLCommand);
        }
    }
    
    ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::queueVRWebGLCommandForProcessing(%s) ends", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());

    pthread_mutex_unlock( &m_mutex );
    
    return result;
}

void VRWebGLCommandProcessor::endFrame()
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
        ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::endFrame %d commands in the batch", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
        m_vrWebGLCommandQueueBatch = m_vrWebGLCommandQueue;
        m_vrWebGLCommandQueue.clear();
        m_currentBatchRenderedForBothEyes = false;
    }

    m_synchronousVRWebGLCommandProcessedInUpdate = false;
    
    pthread_mutex_unlock( &m_mutex );
}

void VRWebGLCommandProcessor::update()
{
    pthread_mutex_lock( &m_mutex );

    // Process all the commands that are for update only.
    for (std::shared_ptr<VRWebGLCommand> vrWebGLCommandForUpdate: m_vrWebGLCommandForUpdateQueue)
    {
        ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::update processing command for update %s", getCurrentThreadName().c_str(), vrWebGLCommandForUpdate->name().c_str());
        vrWebGLCommandForUpdate->process();
        ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::update processed command for update %s", getCurrentThreadName().c_str(), vrWebGLCommandForUpdate->name().c_str());
    }
    m_vrWebGLCommandForUpdateQueue.clear();

    if (m_reset)
    {
        m_resetEverything();
        m_reset = false;
    }

    // Update all the surface textures
    // No longer done here. Each texImage2D call will update its own surface texture with a command.
    // m_surfaceTextures.update();

    // If there is a synchronous VRWebGLCommand, execute it, store the result and notify the waiting thread
    if (m_synchronousVRWebGLCommand)
    {
        // But before, execute all the commands that have been batched up until the execution of the synchronous command.
        if (!m_vrWebGLCommandQueueBatch.empty())
        {
            ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::update %d commands in the batch before processing", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
            for (unsigned int i = 0; i < m_vrWebGLCommandQueueBatch.size(); i++)
            {
                std::shared_ptr<VRWebGLCommand> vrWebGLCommand = m_vrWebGLCommandQueueBatch[i];

                ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::update processing command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
                vrWebGLCommand->process();
                ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::update processed command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());

                if (!vrWebGLCommand->insideAFrame())
                {
                    ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::update removing outside of a frame command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
                    m_vrWebGLCommandQueueBatch.erase(m_vrWebGLCommandQueueBatch.begin() + i);
                    i--;
                    ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::update removed outside of a frame command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
                }
            }
            ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::update %d commands in the batch after process", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
        }

        ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::update processing synchronous command %s", getCurrentThreadName().c_str(), m_synchronousVRWebGLCommand->name().c_str());
        m_synchronousVRWebGLCommandResult = m_synchronousVRWebGLCommand->process();
        ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::update processed synchronous command %s", getCurrentThreadName().c_str(), m_synchronousVRWebGLCommand->name().c_str());
        m_synchronousVRWebGLCommand.reset();
        pthread_cond_broadcast( &m_synchronousVRWebGLCommandProcessed );

        m_synchronousVRWebGLCommandProcessedInUpdate = true;
    }

    pthread_mutex_unlock( &m_mutex );
}

void VRWebGLCommandProcessor::updateSurfaceTexture(GLuint textureId)
{
    m_surfaceTextures.update(textureId);
}

void VRWebGLCommandProcessor::renderFrame(bool process)
{
    pthread_mutex_lock( &m_mutex );

    if (process && !m_synchronousVRWebGLCommand && !m_vrWebGLCommandQueueBatch.empty())
    {
        ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::renderFrame %d commands in the batch before processing", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
        for (unsigned int i = 0; i < m_vrWebGLCommandQueueBatch.size(); i++)
        {
            std::shared_ptr<VRWebGLCommand> vrWebGLCommand = m_vrWebGLCommandQueueBatch[i];

            ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::renderFrame processing command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
            vrWebGLCommand->process();
            ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::renderFrame processed command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());

            if (!vrWebGLCommand->insideAFrame() && m_indexOfEyeBeingRendered == -1)
            {
                ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::renderFrame removing outside of a frame command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
                m_vrWebGLCommandQueueBatch.erase(m_vrWebGLCommandQueueBatch.begin() + i);
                i--;
                ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::renderFrame removed outside of a frame command %s", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
            }
        }
        ALOGV("VRWebGL: %s: VRWebGLCommandProcessor::renderFrame %d commands in the batch after process", getCurrentThreadName().c_str(), m_vrWebGLCommandQueueBatch.size());
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
void VRWebGLCommandProcessor::setMatrixUniformLocationForName(const GLuint program, const GLint location, const std::string& name)
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

bool VRWebGLCommandProcessor::isProjectionMatrixUniformLocation(const GLuint program, const GLint location) const
{
    return std::find(m_projectionMatrixProgramAndUniformLocations.begin(), m_projectionMatrixProgramAndUniformLocations.end(), VRWebGLProgramAndUniformLocation(program, location)) != m_projectionMatrixProgramAndUniformLocations.end();
}

bool VRWebGLCommandProcessor::isModelViewMatrixUniformLocation(const GLuint program, const GLint location) const
{
    return std::find(m_modelViewMatrixProgramAndUniformLocations.begin(), m_modelViewMatrixProgramAndUniformLocations.end(), VRWebGLProgramAndUniformLocation(program, location)) != m_modelViewMatrixProgramAndUniformLocations.end();
}

bool VRWebGLCommandProcessor::isModelViewProjectionMatrixUniformLocation(const GLuint program, const GLint location) const
{
    return std::find(m_modelViewProjectionMatrixProgramAndUniformLocations.begin(), m_modelViewProjectionMatrixProgramAndUniformLocations.end(), VRWebGLProgramAndUniformLocation(program, location)) != m_modelViewProjectionMatrixProgramAndUniformLocations.end();
}

void VRWebGLCommandProcessor::setViewAndProjectionMatrices(const GLfloat* projectionMatrix, const GLfloat* viewMatrix)
{
    memcpy(m_projectionMatrix, projectionMatrix, sizeof(m_projectionMatrix));
    memcpy(m_viewMatrix, viewMatrix, sizeof(m_viewMatrix));
    VRWebGL_multiplyMatrices4(m_viewMatrix, m_projectionMatrix, m_viewProjectionMatrix);
}

const GLfloat* VRWebGLCommandProcessor::getProjectionMatrix() const
{
    return m_projectionMatrix;
}

const GLfloat* VRWebGLCommandProcessor::getViewMatrix() const
{
    return m_viewMatrix;
}

const GLfloat* VRWebGLCommandProcessor::getViewProjectionMatrix() const
{
    return m_viewProjectionMatrix;
}

void VRWebGLCommandProcessor::setCurrentThreadName(const std::string& name)
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

const std::string& VRWebGLCommandProcessor::getCurrentThreadName() const
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


void VRWebGLCommandProcessor::setFramebuffer(const GLuint framebuffer)
{
    m_framebuffer = framebuffer;
}

GLuint VRWebGLCommandProcessor::getFramebuffer() const
{
    return m_framebuffer;
}

void VRWebGLCommandProcessor::setViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

void VRWebGLCommandProcessor::getViewport(GLint& x, GLint& y, GLsizei& width, GLsizei& height) const 
{
    x = m_x;
    y = m_y;
    width = m_width;
    height = m_height;
}    

void VRWebGLCommandProcessor::setCameraWorldMatrix(const GLfloat* cameraWorldMatrix)
{
    memcpy(m_cameraWorldMatrix, cameraWorldMatrix, sizeof(m_cameraWorldMatrix));

    m_cameraWorldMatrixWithTranslationOnly[12] = -cameraWorldMatrix[12];
    m_cameraWorldMatrixWithTranslationOnly[13] = -cameraWorldMatrix[13];
    m_cameraWorldMatrixWithTranslationOnly[14] = -cameraWorldMatrix[14];
}

const GLfloat* VRWebGLCommandProcessor::getCameraWorldMatrix() const
{
    return m_cameraWorldMatrix;
}

const GLfloat* VRWebGLCommandProcessor::getCameraWorldMatrixWithTranslationOnly() const
{
    return m_cameraWorldMatrixWithTranslationOnly;
}

void VRWebGLCommandProcessor::m_resetEverything() 
{
    m_vrWebGLCommandQueue.clear();
    m_vrWebGLCommandQueueBatch.clear();
    m_vrWebGLCommandForUpdateQueue.clear();
    m_insideAFrame = false;
    m_currentBatchRenderedForBothEyes = true;
    m_synchronousVRWebGLCommand.reset();
    m_synchronousVRWebGLCommandResult = 0;
    m_indexOfEyeBeingRendered = -1;

    m_surfaceTextures.clear();

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
}

void VRWebGLCommandProcessor::reset() {
    pthread_mutex_lock( &m_mutex );
    m_reset = true;
    pthread_mutex_unlock( &m_mutex );
}

bool VRWebGLCommandProcessor::m_synchronousVRWebGLCommandBeenProcessedInUpdate() const
{
    return m_synchronousVRWebGLCommandProcessedInUpdate;
}

void VRWebGLCommandProcessor::setupJNI(JNIEnv* jniEnv, jobject mainActivityJObject)
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
    m_dispatchWebViewKeyboardEventMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "dispatchWebViewKeyboardEvent", "(Landroid/graphics/SurfaceTexture;II)V");
    m_dispatchWebViewCursorEventMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "dispatchWebViewCursorEvent", "(Landroid/graphics/SurfaceTexture;IFF)V");
    m_setWebViewTransparentMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "setWebViewTransparent", "(Landroid/graphics/SurfaceTexture;Z)V");
    m_setWebViewScaleMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "setWebViewScale", "(Landroid/graphics/SurfaceTexture;IZZ)V");
    m_newSpeechRecognitionMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "newSpeechRecognition", "(J)V");
    m_deleteSpeechRecognitionMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "deleteSpeechRecognition", "(J)V");
    m_startSpeechRecognitionMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "startSpeechRecognition", "(J)V");
    m_stopSpeechRecognitionMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "stopSpeechRecognition", "(J)V");
    m_newVideoMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "newVideo", "(Landroid/graphics/SurfaceTexture;I)V");
    m_deleteVideoMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "deleteVideo", "(Landroid/graphics/SurfaceTexture;)V");
    m_setVideoSrcMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "setVideoSrcOnUIThread", "(Landroid/graphics/SurfaceTexture;Ljava/lang/String;)V");
    m_dispatchVideoPlaybackEventMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "dispatchVideoPlaybackEvent", "(Landroid/graphics/SurfaceTexture;IFZ)V");
    m_setVideoVolumeMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "setVideoVolume", "(Landroid/graphics/SurfaceTexture;F)V");
    m_setVideoLoopMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "setVideoLoop", "(Landroid/graphics/SurfaceTexture;Z)V");
    m_setVideoCurrentTimeMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "setVideoCurrentTime", "(Landroid/graphics/SurfaceTexture;I)V");
    m_getVideoDurationMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "getVideoDuration", "(Landroid/graphics/SurfaceTexture;)I");
    m_getVideoCurrentTimeMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "getVideoCurrentTime", "(Landroid/graphics/SurfaceTexture;)I");
    m_getVideoWidthMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "getVideoWidth", "(Landroid/graphics/SurfaceTexture;)I");
    m_getVideoHeightMethodID = jniEnv->GetMethodID(m_mainActivityJClass, "getVideoHeight", "(Landroid/graphics/SurfaceTexture;)I");
}

JNIEnv* VRWebGLCommandProcessor::getJNIEnv() const
{
    return m_jniEnv;
}

std::shared_ptr<VRWebGLSurfaceTexture> VRWebGLCommandProcessor::newSurfaceTexture()
{
    return m_surfaceTextures.newSurfaceTexture(m_jniEnv);
}

void VRWebGLCommandProcessor::deleteSurfaceTexture(const std::shared_ptr<VRWebGLSurfaceTexture>& surfaceTexture)
{
    m_surfaceTextures.deleteSurfaceTexture(surfaceTexture);
}

std::shared_ptr<VRWebGLSurfaceTexture> VRWebGLCommandProcessor::findSurfaceTextureByTextureId(unsigned textureId)
{
    return m_surfaceTextures.findSurfaceTextureByTextureId(textureId);
}

const jobject VRWebGLCommandProcessor::getMainActivityJObject() const
{
    return m_mainActivityJObject;
}

jmethodID VRWebGLCommandProcessor::getNewWebViewMethodID() const
{
    return m_newWebViewMethodID;
}

jmethodID VRWebGLCommandProcessor::getDeleteWebViewMethodID() const
{
    return m_deleteWebViewMethodID;
}

jmethodID VRWebGLCommandProcessor::getSetWebViewSrcMethodID() const
{
    return m_setWebViewSrcMethodID;
}

jmethodID VRWebGLCommandProcessor::getDispatchWebViewTouchEventMethodID() const
{
    return m_dispatchWebViewTouchEventMethodID;
}

jmethodID VRWebGLCommandProcessor::getDispatchWebViewNavigationEventMethodID() const
{
    return m_dispatchWebViewNavigationEventMethodID;
}

jmethodID VRWebGLCommandProcessor::getDispatchWebViewKeyboardEventMethodID() const
{
    return m_dispatchWebViewKeyboardEventMethodID;
}

jmethodID VRWebGLCommandProcessor::getDispatchWebViewCursorEventMethodID() const
{
    return m_dispatchWebViewCursorEventMethodID;
}

jmethodID VRWebGLCommandProcessor::getSetWebViewTransparentMethodID() const
{
    return m_setWebViewTransparentMethodID;
}

jmethodID VRWebGLCommandProcessor::getSetWebViewScaleMethodID() const
{
    return m_setWebViewScaleMethodID;
}

jmethodID VRWebGLCommandProcessor::getNewSpeechRecognitionMethodID() const
{
    return m_newSpeechRecognitionMethodID;
}

jmethodID VRWebGLCommandProcessor::getDeleteSpeechRecognitionMethodID() const
{
    return m_deleteSpeechRecognitionMethodID;
}

jmethodID VRWebGLCommandProcessor::getStartSpeechRecognitionMethodID() const
{
    return m_startSpeechRecognitionMethodID;
}

jmethodID VRWebGLCommandProcessor::getStopSpeechRecognitionMethodID() const
{
    return m_stopSpeechRecognitionMethodID;
}

jmethodID VRWebGLCommandProcessor::getNewVideoMethodID() const
{
    return m_newVideoMethodID;
}

jmethodID VRWebGLCommandProcessor::getDeleteVideoMethodID() const
{
    return m_deleteVideoMethodID;
}

jmethodID VRWebGLCommandProcessor::getSetVideoSrcMethodID() const
{
    return m_setVideoSrcMethodID;
}

jmethodID VRWebGLCommandProcessor::getDispatchVideoPlaybackEventMethodID() const
{
    return m_dispatchVideoPlaybackEventMethodID;
}

jmethodID VRWebGLCommandProcessor::getSetVideoVolumeMethodID() const
{
    return m_setVideoVolumeMethodID;
}

jmethodID VRWebGLCommandProcessor::getSetVideoLoopMethodID() const
{
    return m_setVideoLoopMethodID;
}

jmethodID VRWebGLCommandProcessor::getSetVideoCurrentTimeMethodID() const
{
    return m_setVideoCurrentTimeMethodID;
}

jmethodID VRWebGLCommandProcessor::getGetVideoDurationMethodID() const
{
    return m_getVideoDurationMethodID;
}

jmethodID VRWebGLCommandProcessor::getGetVideoCurrentTimeMethodID() const
{
    return m_getVideoCurrentTimeMethodID;
}

jmethodID VRWebGLCommandProcessor::getGetVideoWidthMethodID() const
{
    return m_getVideoWidthMethodID;
}

jmethodID VRWebGLCommandProcessor::getGetVideoHeightMethodID() const
{
    return m_getVideoHeightMethodID;
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

bool VRWebGLCommand_newWebView::isForUpdate() const
{
    return true;
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

bool VRWebGLCommand_deleteWebView::isForUpdate() const
{
    return true;
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
    if (surfaceTexture) {
        jmethodID deleteWebViewMethodID = VRWebGLCommandProcessor::getInstance()->getDeleteWebViewMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jniEnv->CallVoidMethod( mainActivityJObject, deleteWebViewMethodID, surfaceTexture->getJavaObject() );
        VRWebGLCommandProcessor::getInstance()->deleteSurfaceTexture(surfaceTexture);
    }
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

bool VRWebGLCommand_setWebViewSrc::isForUpdate() const
{
    return true;
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
    if (surfaceTexture)
    {
        jmethodID setWebViewSrcMethodID = VRWebGLCommandProcessor::getInstance()->getSetWebViewSrcMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jstring srcJString = jniEnv->NewStringUTF( src.c_str() );
        jniEnv->CallVoidMethod( mainActivityJObject, setWebViewSrcMethodID, surfaceTexture->getJavaObject(), srcJString );
        jniEnv->DeleteLocalRef( srcJString );
    }
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

bool VRWebGLCommand_dispatchWebViewTouchEvent::isForUpdate() const
{
    return true;
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
    if (surfaceTexture)
    {
        jmethodID dispatchWebViewTouchEventMethodID = VRWebGLCommandProcessor::getInstance()->getDispatchWebViewTouchEventMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jniEnv->CallVoidMethod( mainActivityJObject, dispatchWebViewTouchEventMethodID, surfaceTexture->getJavaObject(), (jint)event, x, y );
    }
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

bool VRWebGLCommand_dispatchWebViewNavigationEvent::isForUpdate() const
{
    return true;
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
    if (surfaceTexture)
    {
        jmethodID dispatchWebViewNavigationEventMethodID = VRWebGLCommandProcessor::getInstance()->getDispatchWebViewNavigationEventMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jniEnv->CallVoidMethod( mainActivityJObject, dispatchWebViewNavigationEventMethodID, surfaceTexture->getJavaObject(), (jint)event);
    }
    return 0;
}

std::string VRWebGLCommand_dispatchWebViewNavigationEvent::name() const
{
    return "dispatchWebViewNavigationEvent";
}

// =====================================================================================

std::pair<int, int> jsKeyCodeToJavaKeyCode_[] =
{
    std::pair<int, int>(  8,   0), // backspace
    std::pair<int, int>(  9,   0), // tab
    std::pair<int, int>( 13,   0), // enter
    std::pair<int, int>( 16,  59), // shift
    std::pair<int, int>( 17,   0), // ctrl
    std::pair<int, int>( 18,   0), // alt
    std::pair<int, int>( 19,   0), // pause/break
    std::pair<int, int>( 20, 115), // caps lock
    std::pair<int, int>( 27,   0), // escape
    std::pair<int, int>( 33,   0), // page up
    std::pair<int, int>( 34,   0), // page down
    std::pair<int, int>( 35,   0), // end
    std::pair<int, int>( 36,   0), // home 
    std::pair<int, int>( 37,   0), // left arrow
    std::pair<int, int>( 38,   0), // up arrow
    std::pair<int, int>( 39,   0), // right arrow
    std::pair<int, int>( 40,   0), // down arrow
    std::pair<int, int>( 45,   0), // insert
    std::pair<int, int>( 46,   0), // delete
    std::pair<int, int>( 48,   0), // 0 
    std::pair<int, int>( 49,   0), // 1
    std::pair<int, int>( 50,   0), // 1
    std::pair<int, int>( 51,   0), // 1
    std::pair<int, int>( 52,   0), // 1
    std::pair<int, int>( 53,   0), // 1
    std::pair<int, int>( 54,   0), // 1
    std::pair<int, int>( 55,   0), // 1
    std::pair<int, int>( 56,   0), // 1
    std::pair<int, int>( 57,   0), // 1
    std::pair<int, int>( 65,  29), // a
    std::pair<int, int>( 66,   0), // b
    std::pair<int, int>( 67,   0), // c
    std::pair<int, int>( 68,   0), // d 
    std::pair<int, int>( 69,   0), // e
    std::pair<int, int>( 70,   0), // f
    std::pair<int, int>( 71,   0), // g
    std::pair<int, int>( 72,   0), // h
    std::pair<int, int>( 73,   0), // i
    std::pair<int, int>( 74,   0), // j
    std::pair<int, int>( 75,   0), // k
    std::pair<int, int>( 76,   0), // l
    std::pair<int, int>( 77,   0), // m
    std::pair<int, int>( 78,   0), // n
    std::pair<int, int>( 79,   0), // o
    std::pair<int, int>( 80,   0), // p
    std::pair<int, int>( 81,   0), // q
    std::pair<int, int>( 82,   0), // r
    std::pair<int, int>( 83,   0), // s
    std::pair<int, int>( 84,   0), // t
    std::pair<int, int>( 85,   0), // u
    std::pair<int, int>( 86,   0), // v
    std::pair<int, int>( 87,   0), // w
    std::pair<int, int>( 88,   0), // x
    std::pair<int, int>( 89,   0), // y
    std::pair<int, int>( 90,   0), // z
    std::pair<int, int>( 91,   0), // left window key
    std::pair<int, int>( 92,   0), // right window key
    std::pair<int, int>( 93,   0), // select key
    std::pair<int, int>( 96,   0), // numpad 0
    std::pair<int, int>( 97,   0), // numpad 1
    std::pair<int, int>( 98,   0), // numpad 2
    std::pair<int, int>( 99,   0), // numpad 3
    std::pair<int, int>(100,   0), // numpad 4
    std::pair<int, int>(101,   0), // numpad 5
    std::pair<int, int>(102,   0), // numpad 6
    std::pair<int, int>(103,   0), // numpad 7
    std::pair<int, int>(104,   0), // numpad 8
    std::pair<int, int>(105,   0), // numpad 9
    std::pair<int, int>(106,   0), // multiply
    std::pair<int, int>(107,   0), // add
    std::pair<int, int>(109,   0), // substract
    std::pair<int, int>(110,   0), // decimal point
    std::pair<int, int>(111,   0), // divide
    std::pair<int, int>(112,   0), // f1
    std::pair<int, int>(113,   0), // f2
    std::pair<int, int>(114,   0), // f3
    std::pair<int, int>(115,   0), // f4
    std::pair<int, int>(116,   0), // f5
    std::pair<int, int>(117,   0), // f6
    std::pair<int, int>(118,   0), // f7
    std::pair<int, int>(119,   0), // f8
    std::pair<int, int>(120,   0), // f9
    std::pair<int, int>(121,   0), // f10
    std::pair<int, int>(122,   0), // f11
    std::pair<int, int>(123,   0), // f12
    std::pair<int, int>(144,   0), // num lock
    std::pair<int, int>(145,   0), // scroll lock
    std::pair<int, int>(186,   0), // semi-colon
    std::pair<int, int>(187,   0), // equal sign
    std::pair<int, int>(188,   0), // comma
    std::pair<int, int>(189,   0), // dash
    std::pair<int, int>(190,   0), // period
    std::pair<int, int>(191,   0), // forward slash
    std::pair<int, int>(192,   0), // grvae accent
    std::pair<int, int>(219,   0), // open bracket
    std::pair<int, int>(220,   0), // back slash
    std::pair<int, int>(221,   0), // close bracket
    std::pair<int, int>(222,   0) // single quote
};

std::map<int, int> VRWebGLCommand_dispatchWebViewKeyboardEvent::jsKeyCodeToJavaKeyCode(&jsKeyCodeToJavaKeyCode_[0], &jsKeyCodeToJavaKeyCode_[sizeof(jsKeyCodeToJavaKeyCode_) / sizeof(jsKeyCodeToJavaKeyCode_[0])]);

VRWebGLCommand_dispatchWebViewKeyboardEvent::VRWebGLCommand_dispatchWebViewKeyboardEvent(GLuint textureId, Event event, int keycode): textureId(textureId), event(event), keycode(keycode)
{
}

std::shared_ptr<VRWebGLCommand_dispatchWebViewKeyboardEvent> VRWebGLCommand_dispatchWebViewKeyboardEvent::newInstance(GLuint textureId, Event event, int keycode)
{
    return std::shared_ptr<VRWebGLCommand_dispatchWebViewKeyboardEvent>(new VRWebGLCommand_dispatchWebViewKeyboardEvent(textureId, event, keycode));
}

bool VRWebGLCommand_dispatchWebViewKeyboardEvent::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_dispatchWebViewKeyboardEvent::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_dispatchWebViewKeyboardEvent::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_dispatchWebViewKeyboardEvent::process()
{
    if (jsKeyCodeToJavaKeyCode.find(keycode) != jsKeyCodeToJavaKeyCode.end())
    {
        std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
        if (surfaceTexture)
        {
            jmethodID dispatchWebViewKeyboardEventMethodID = VRWebGLCommandProcessor::getInstance()->getDispatchWebViewKeyboardEventMethodID();
            JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
            jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
            jniEnv->CallVoidMethod( mainActivityJObject, dispatchWebViewKeyboardEventMethodID, surfaceTexture->getJavaObject(), (jint)event, jsKeyCodeToJavaKeyCode[keycode]);
        }
    }
    else 
    {
        ALOGE("ERROR: The provided keycode '%d' has not been found in the js to java keycode conversion table.", keycode);
    }
    return 0;
}

std::string VRWebGLCommand_dispatchWebViewKeyboardEvent::name() const
{
    return "dispatchWebViewKeyboardEvent";
}

// =====================================================================================

VRWebGLCommand_dispatchWebViewCursorEvent::VRWebGLCommand_dispatchWebViewCursorEvent(GLuint textureId, Event event, float x, float y): textureId(textureId), event(event), x(x), y(y)
{
}

std::shared_ptr<VRWebGLCommand_dispatchWebViewCursorEvent> VRWebGLCommand_dispatchWebViewCursorEvent::newInstance(GLuint textureId, Event event, float x, float y)
{
    return std::shared_ptr<VRWebGLCommand_dispatchWebViewCursorEvent>(new VRWebGLCommand_dispatchWebViewCursorEvent(textureId, event, x, y));
}

bool VRWebGLCommand_dispatchWebViewCursorEvent::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_dispatchWebViewCursorEvent::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_dispatchWebViewCursorEvent::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_dispatchWebViewCursorEvent::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID dispatchWebViewCursorEventMethodID = VRWebGLCommandProcessor::getInstance()->getDispatchWebViewCursorEventMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jniEnv->CallVoidMethod( mainActivityJObject, dispatchWebViewCursorEventMethodID, surfaceTexture->getJavaObject(), (jint)event, x, y );
    }
    return 0;
}

std::string VRWebGLCommand_dispatchWebViewCursorEvent::name() const
{
    return "dispatchWebViewCursorEvent";
}

// =====================================================================================

VRWebGLCommand_setWebViewTransparent::VRWebGLCommand_setWebViewTransparent(GLuint textureId, bool transparent): textureId(textureId), transparent(transparent)
{
}

std::shared_ptr<VRWebGLCommand_setWebViewTransparent> VRWebGLCommand_setWebViewTransparent::newInstance(GLuint textureId, bool transparent)
{
    return std::shared_ptr<VRWebGLCommand_setWebViewTransparent>(new VRWebGLCommand_setWebViewTransparent(textureId, transparent));
}

bool VRWebGLCommand_setWebViewTransparent::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_setWebViewTransparent::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_setWebViewTransparent::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_setWebViewTransparent::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID setWebViewTransparentMethodID = VRWebGLCommandProcessor::getInstance()->getSetWebViewTransparentMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jniEnv->CallVoidMethod( mainActivityJObject, setWebViewTransparentMethodID, surfaceTexture->getJavaObject(), (jboolean)transparent);
    }
    return 0;
}

std::string VRWebGLCommand_setWebViewTransparent::name() const
{
    return "setWebViewTransparent";
}

// =====================================================================================

VRWebGLCommand_setWebViewScale::VRWebGLCommand_setWebViewScale(GLuint textureId, int scale, bool loadWithOverviewMode, bool useWideViewPort): textureId(textureId), scale(scale), loadWithOverviewMode(loadWithOverviewMode), useWideViewPort(useWideViewPort)
{
}

std::shared_ptr<VRWebGLCommand_setWebViewScale> VRWebGLCommand_setWebViewScale::newInstance(GLuint textureId, int scale, bool loadWithOverviewMode, bool useWideViewPort)
{
    return std::shared_ptr<VRWebGLCommand_setWebViewScale>(new VRWebGLCommand_setWebViewScale(textureId, scale, loadWithOverviewMode, useWideViewPort));
}

bool VRWebGLCommand_setWebViewScale::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_setWebViewScale::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_setWebViewScale::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_setWebViewScale::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID setWebViewScaleMethodID = VRWebGLCommandProcessor::getInstance()->getSetWebViewScaleMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jniEnv->CallVoidMethod( mainActivityJObject, setWebViewScaleMethodID, surfaceTexture->getJavaObject(), (jint)scale, (jboolean)loadWithOverviewMode, (jboolean)useWideViewPort);
    }
    return 0;
}

std::string VRWebGLCommand_setWebViewScale::name() const
{
    return "setWebViewScale";
}

// =====================================================================================

VRWebGLCommand_newSpeechRecognition::VRWebGLCommand_newSpeechRecognition(long id): id(id)
{
}

std::shared_ptr<VRWebGLCommand_newSpeechRecognition> VRWebGLCommand_newSpeechRecognition::newInstance(long id)
{
    return std::shared_ptr<VRWebGLCommand_newSpeechRecognition>(new VRWebGLCommand_newSpeechRecognition(id));
}

bool VRWebGLCommand_newSpeechRecognition::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_newSpeechRecognition::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_newSpeechRecognition::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_newSpeechRecognition::process()
{
    jmethodID newSpeechRecognitionMethodID = VRWebGLCommandProcessor::getInstance()->getNewSpeechRecognitionMethodID();
    JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
    jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
    jniEnv->CallVoidMethod( mainActivityJObject, newSpeechRecognitionMethodID, (jlong)id );
    return 0;
}

std::string VRWebGLCommand_newSpeechRecognition::name() const
{
    return "newSpeechRecognition";
}

// =====================================================================================

VRWebGLCommand_deleteSpeechRecognition::VRWebGLCommand_deleteSpeechRecognition(long id): id(id)
{
}

std::shared_ptr<VRWebGLCommand_deleteSpeechRecognition> VRWebGLCommand_deleteSpeechRecognition::newInstance(long id)
{
    return std::shared_ptr<VRWebGLCommand_deleteSpeechRecognition>(new VRWebGLCommand_deleteSpeechRecognition(id));
}

bool VRWebGLCommand_deleteSpeechRecognition::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_deleteSpeechRecognition::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_deleteSpeechRecognition::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_deleteSpeechRecognition::process()
{
    jmethodID deleteSpeechRecognitionMethodID = VRWebGLCommandProcessor::getInstance()->getDeleteSpeechRecognitionMethodID();
    JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
    jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
    jniEnv->CallVoidMethod( mainActivityJObject, deleteSpeechRecognitionMethodID, (jlong)id );
    return 0;
}

std::string VRWebGLCommand_deleteSpeechRecognition::name() const
{
    return "deleteSpeechRecognition";
}

// =====================================================================================

VRWebGLCommand_startSpeechRecognition::VRWebGLCommand_startSpeechRecognition(long id): id(id)
{
}

std::shared_ptr<VRWebGLCommand_startSpeechRecognition> VRWebGLCommand_startSpeechRecognition::newInstance(long id)
{
    return std::shared_ptr<VRWebGLCommand_startSpeechRecognition>(new VRWebGLCommand_startSpeechRecognition(id));
}

bool VRWebGLCommand_startSpeechRecognition::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_startSpeechRecognition::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_startSpeechRecognition::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_startSpeechRecognition::process()
{
    jmethodID startSpeechRecognitionMethodID = VRWebGLCommandProcessor::getInstance()->getStartSpeechRecognitionMethodID();
    JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
    jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
    jniEnv->CallVoidMethod( mainActivityJObject, startSpeechRecognitionMethodID, (jlong)id );
    return 0;
}

std::string VRWebGLCommand_startSpeechRecognition::name() const
{
    return "startSpeechRecognition";
}

// =====================================================================================

VRWebGLCommand_stopSpeechRecognition::VRWebGLCommand_stopSpeechRecognition(long id): id(id)
{
}

std::shared_ptr<VRWebGLCommand_stopSpeechRecognition> VRWebGLCommand_stopSpeechRecognition::newInstance(long id)
{
    return std::shared_ptr<VRWebGLCommand_stopSpeechRecognition>(new VRWebGLCommand_stopSpeechRecognition(id));
}

bool VRWebGLCommand_stopSpeechRecognition::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_stopSpeechRecognition::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_stopSpeechRecognition::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_stopSpeechRecognition::process()
{
    jmethodID stopSpeechRecognitionMethodID = VRWebGLCommandProcessor::getInstance()->getStopSpeechRecognitionMethodID();
    JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
    jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
    jniEnv->CallVoidMethod( mainActivityJObject, stopSpeechRecognitionMethodID, (jlong)id );
    return 0;
}

std::string VRWebGLCommand_stopSpeechRecognition::name() const
{
    return "stopSpeechRecognition";
}

// =====================================================================================

VRWebGLCommand_newVideo::VRWebGLCommand_newVideo(): textureId(0)
{
}

std::shared_ptr<VRWebGLCommand_newVideo> VRWebGLCommand_newVideo::newInstance()
{
    return std::shared_ptr<VRWebGLCommand_newVideo>(new VRWebGLCommand_newVideo());
}

bool VRWebGLCommand_newVideo::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_newVideo::isSynchronous() const
{
    return true;
}

bool VRWebGLCommand_newVideo::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_newVideo::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->newSurfaceTexture();
    textureId = surfaceTexture->getTextureId();
    jmethodID newVideoMethodID = VRWebGLCommandProcessor::getInstance()->getNewVideoMethodID();
    JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
    jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
    jniEnv->CallVoidMethod( mainActivityJObject, newVideoMethodID, surfaceTexture->getJavaObject(), textureId );
    return &textureId;
}

std::string VRWebGLCommand_newVideo::name() const
{
    return "newVideo";
}

// =====================================================================================

VRWebGLCommand_deleteVideo::VRWebGLCommand_deleteVideo(GLuint textureId): textureId(textureId)
{
}

std::shared_ptr<VRWebGLCommand_deleteVideo> VRWebGLCommand_deleteVideo::newInstance(GLuint textureId)
{
    return std::shared_ptr<VRWebGLCommand_deleteVideo>(new VRWebGLCommand_deleteVideo(textureId));
}

bool VRWebGLCommand_deleteVideo::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_deleteVideo::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_deleteVideo::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_deleteVideo::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture) {
        jmethodID deleteVideoMethodID = VRWebGLCommandProcessor::getInstance()->getDeleteVideoMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jniEnv->CallVoidMethod( mainActivityJObject, deleteVideoMethodID, surfaceTexture->getJavaObject() );
        VRWebGLCommandProcessor::getInstance()->deleteSurfaceTexture(surfaceTexture);
    }
    return 0;
}

std::string VRWebGLCommand_deleteVideo::name() const
{
    return "deleteVideo";
}

// =====================================================================================

VRWebGLCommand_setVideoSrc::VRWebGLCommand_setVideoSrc(GLuint textureId, const std::string& src): textureId(textureId), src(src)
{
    ALOGE("JudaX: VRWebGLCommand_setVideoSrc::constructor(%d, %s)", textureId, src.c_str());
}

std::shared_ptr<VRWebGLCommand_setVideoSrc> VRWebGLCommand_setVideoSrc::newInstance(GLuint textureId, const std::string& src)
{
    return std::shared_ptr<VRWebGLCommand_setVideoSrc>(new VRWebGLCommand_setVideoSrc(textureId, src));
}

bool VRWebGLCommand_setVideoSrc::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_setVideoSrc::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_setVideoSrc::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_setVideoSrc::process()
{
    ALOGE("JudaX: VRWebGLCommand_setVideoSrc::process(%d, %s)", textureId, src.c_str());
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID setVideoSrcMethodID = VRWebGLCommandProcessor::getInstance()->getSetVideoSrcMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jstring srcJString = jniEnv->NewStringUTF( src.c_str() );
        jniEnv->CallVoidMethod( mainActivityJObject, setVideoSrcMethodID, surfaceTexture->getJavaObject(), srcJString );
        jniEnv->DeleteLocalRef( srcJString );
    }
    return 0;
}

std::string VRWebGLCommand_setVideoSrc::name() const
{
    return "setVideoSrc";
}

// =====================================================================================

VRWebGLCommand_dispatchVideoPlaybackEvent::VRWebGLCommand_dispatchVideoPlaybackEvent(GLuint textureId, Event event, float volume, bool loop): textureId(textureId), event(event), volume(volume), loop(loop)
{
}

std::shared_ptr<VRWebGLCommand_dispatchVideoPlaybackEvent> VRWebGLCommand_dispatchVideoPlaybackEvent::newInstance(GLuint textureId, Event event, float volume, bool loop)
{
    return std::shared_ptr<VRWebGLCommand_dispatchVideoPlaybackEvent>(new VRWebGLCommand_dispatchVideoPlaybackEvent(textureId, event, volume, loop));
}

bool VRWebGLCommand_dispatchVideoPlaybackEvent::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_dispatchVideoPlaybackEvent::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_dispatchVideoPlaybackEvent::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_dispatchVideoPlaybackEvent::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID dispatchVideoPlaybackEventMethodID = VRWebGLCommandProcessor::getInstance()->getDispatchVideoPlaybackEventMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jniEnv->CallVoidMethod( mainActivityJObject, dispatchVideoPlaybackEventMethodID, surfaceTexture->getJavaObject(), (jint)event, (jfloat)volume, loop);
    }
    return 0;
}

std::string VRWebGLCommand_dispatchVideoPlaybackEvent::name() const
{
    return "dispatchVideoPlaybackEvent";
}

// =====================================================================================

VRWebGLCommand_setVideoVolume::VRWebGLCommand_setVideoVolume(GLuint textureId, float volume): textureId(textureId), volume(volume)
{
}

std::shared_ptr<VRWebGLCommand_setVideoVolume> VRWebGLCommand_setVideoVolume::newInstance(GLuint textureId, float volume)
{
    return std::shared_ptr<VRWebGLCommand_setVideoVolume>(new VRWebGLCommand_setVideoVolume(textureId, volume));
}

bool VRWebGLCommand_setVideoVolume::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_setVideoVolume::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_setVideoVolume::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_setVideoVolume::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID setVideoVolumeMethodID = VRWebGLCommandProcessor::getInstance()->getSetVideoVolumeMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jniEnv->CallVoidMethod( mainActivityJObject, setVideoVolumeMethodID, surfaceTexture->getJavaObject(), (jfloat)volume );
    }
    return 0;
}

std::string VRWebGLCommand_setVideoVolume::name() const
{
    return "setVideoVolume";
}

// =====================================================================================

VRWebGLCommand_setVideoLoop::VRWebGLCommand_setVideoLoop(GLuint textureId, bool loop): textureId(textureId), loop(loop)
{
}

std::shared_ptr<VRWebGLCommand_setVideoLoop> VRWebGLCommand_setVideoLoop::newInstance(GLuint textureId, bool loop)
{
    return std::shared_ptr<VRWebGLCommand_setVideoLoop>(new VRWebGLCommand_setVideoLoop(textureId, loop));
}

bool VRWebGLCommand_setVideoLoop::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_setVideoLoop::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_setVideoLoop::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_setVideoLoop::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID setVideoLoopMethodID = VRWebGLCommandProcessor::getInstance()->getSetVideoLoopMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jniEnv->CallVoidMethod( mainActivityJObject, setVideoLoopMethodID, surfaceTexture->getJavaObject(), loop );
    }
    return 0;
}

std::string VRWebGLCommand_setVideoLoop::name() const
{
    return "setVideoLoop";
}

// =====================================================================================

VRWebGLCommand_setVideoCurrentTime::VRWebGLCommand_setVideoCurrentTime(GLuint textureId, float currentTime): textureId(textureId), currentTime(currentTime)
{
}

std::shared_ptr<VRWebGLCommand_setVideoCurrentTime> VRWebGLCommand_setVideoCurrentTime::newInstance(GLuint textureId, float currentTime)
{
    return std::shared_ptr<VRWebGLCommand_setVideoCurrentTime>(new VRWebGLCommand_setVideoCurrentTime(textureId, currentTime));
}

bool VRWebGLCommand_setVideoCurrentTime::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_setVideoCurrentTime::isSynchronous() const
{
    return false;
}

bool VRWebGLCommand_setVideoCurrentTime::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_setVideoCurrentTime::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID setVideoCurrentTimeMethodID = VRWebGLCommandProcessor::getInstance()->getSetVideoCurrentTimeMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        jniEnv->CallVoidMethod( mainActivityJObject, setVideoCurrentTimeMethodID, surfaceTexture->getJavaObject(), (jint)(currentTime * 1000.0f) );
    }
    return 0;
}

std::string VRWebGLCommand_setVideoCurrentTime::name() const
{
    return "setVideoCurrentTime";
}

// =====================================================================================

VRWebGLCommand_getVideoDuration::VRWebGLCommand_getVideoDuration(GLuint textureId): textureId(textureId)
{
}

std::shared_ptr<VRWebGLCommand_getVideoDuration> VRWebGLCommand_getVideoDuration::newInstance(GLuint textureId)
{
    return std::shared_ptr<VRWebGLCommand_getVideoDuration>(new VRWebGLCommand_getVideoDuration(textureId));
}

bool VRWebGLCommand_getVideoDuration::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_getVideoDuration::isSynchronous() const
{
    return true;
}

bool VRWebGLCommand_getVideoDuration::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_getVideoDuration::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID getVideoDurationMethodID = VRWebGLCommandProcessor::getInstance()->getGetVideoDurationMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        duration = jniEnv->CallIntMethod( mainActivityJObject, getVideoDurationMethodID, surfaceTexture->getJavaObject() ) / 1000.0f;
    }
    return &duration;
}

std::string VRWebGLCommand_getVideoDuration::name() const
{
    return "getVideoDuration";
}

// =====================================================================================

VRWebGLCommand_getVideoCurrentTime::VRWebGLCommand_getVideoCurrentTime(GLuint textureId): textureId(textureId)
{
}

std::shared_ptr<VRWebGLCommand_getVideoCurrentTime> VRWebGLCommand_getVideoCurrentTime::newInstance(GLuint textureId)
{
    return std::shared_ptr<VRWebGLCommand_getVideoCurrentTime>(new VRWebGLCommand_getVideoCurrentTime(textureId));
}

bool VRWebGLCommand_getVideoCurrentTime::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_getVideoCurrentTime::isSynchronous() const
{
    return true;
}

bool VRWebGLCommand_getVideoCurrentTime::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_getVideoCurrentTime::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID getVideoCurrentTimeMethodID = VRWebGLCommandProcessor::getInstance()->getGetVideoCurrentTimeMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        currentTime = jniEnv->CallIntMethod( mainActivityJObject, getVideoCurrentTimeMethodID, surfaceTexture->getJavaObject() ) / 1000.0f;
    }
    return &currentTime;
}

std::string VRWebGLCommand_getVideoCurrentTime::name() const
{
    return "getVideoCurrentTime";
}

// =====================================================================================

VRWebGLCommand_getVideoWidth::VRWebGLCommand_getVideoWidth(GLuint textureId): textureId(textureId)
{
}

std::shared_ptr<VRWebGLCommand_getVideoWidth> VRWebGLCommand_getVideoWidth::newInstance(GLuint textureId)
{
    return std::shared_ptr<VRWebGLCommand_getVideoWidth>(new VRWebGLCommand_getVideoWidth(textureId));
}

bool VRWebGLCommand_getVideoWidth::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_getVideoWidth::isSynchronous() const
{
    return true;
}

bool VRWebGLCommand_getVideoWidth::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_getVideoWidth::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID getVideoWidthMethodID = VRWebGLCommandProcessor::getInstance()->getGetVideoWidthMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        width = jniEnv->CallIntMethod( mainActivityJObject, getVideoWidthMethodID, surfaceTexture->getJavaObject() );
    }
    return &width;
}

std::string VRWebGLCommand_getVideoWidth::name() const
{
    return "getVideowidth";
}

// =====================================================================================

VRWebGLCommand_getVideoHeight::VRWebGLCommand_getVideoHeight(GLuint textureId): textureId(textureId)
{
}

std::shared_ptr<VRWebGLCommand_getVideoHeight> VRWebGLCommand_getVideoHeight::newInstance(GLuint textureId)
{
    return std::shared_ptr<VRWebGLCommand_getVideoHeight>(new VRWebGLCommand_getVideoHeight(textureId));
}

bool VRWebGLCommand_getVideoHeight::isForUpdate() const
{
    return true;
}

bool VRWebGLCommand_getVideoHeight::isSynchronous() const
{
    return true;
}

bool VRWebGLCommand_getVideoHeight::canBeProcessedImmediately() const
{
    return false;
}

void* VRWebGLCommand_getVideoHeight::process()
{
    std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture = VRWebGLCommandProcessor::getInstance()->findSurfaceTextureByTextureId(textureId);
    if (surfaceTexture)
    {
        jmethodID getVideoHeightMethodID = VRWebGLCommandProcessor::getInstance()->getGetVideoHeightMethodID();
        JNIEnv* jniEnv = VRWebGLCommandProcessor::getInstance()->getJNIEnv();
        jobject mainActivityJObject = VRWebGLCommandProcessor::getInstance()->getMainActivityJObject();
        height = jniEnv->CallIntMethod( mainActivityJObject, getVideoHeightMethodID, surfaceTexture->getJavaObject() );
    }
    return &height;
}

std::string VRWebGLCommand_getVideoHeight::name() const
{
    return "getVideoHeight";
}

