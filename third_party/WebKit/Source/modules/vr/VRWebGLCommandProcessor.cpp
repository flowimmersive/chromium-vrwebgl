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

    ALOGV("VRWebGL: %s: VRWebGLCommandProcessorImpl::queueVRWebGLCommandForProcessing(%s)", getCurrentThreadName().c_str(), vrWebGLCommand->name().c_str());
    
    if (vrWebGLCommand->isSynchronous())
    {
        // The command is synchronous, so:
        // 1.- Store the command so it can be executed in the OpenGL thread (check update).
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
    else
    {
        vrWebGLCommand->setInsideAFrame(m_insideAFrame);
        m_vrWebGLCommandQueue.push_back(vrWebGLCommand);
    }
    
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

    // Increment the renderFrameCallCounter to measure when the 2 calls, one for each eye, are made so we can notify to whoever is waiting for this condition
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
