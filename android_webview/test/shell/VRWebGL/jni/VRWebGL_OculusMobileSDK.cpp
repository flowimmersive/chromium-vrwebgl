/************************************************************************************

Filename	:	VrCubeWorld_SurfaceView.c
Content		:	This sample uses a plain Android SurfaceView and handles all
				Activity and Surface life cycle events in native code. This sample
				does not use the application framework and also does not use LibOVRKernel.
				This sample only uses the VrApi.
Created		:	March, 2015
Authors		:	J.M.P. van Waveren

Copyright	:	Copyright 2015 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include <android/native_window_jni.h>	// for native window JNI
#include <android/input.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "modules/vr/VRWebGLCommand.h"
#include "modules/vr/VRWebGLCommandProcessor.h"
#include "modules/vr/VRWebGLMath.h"
#include "modules/vr/VRWebGLPose.h"
#include "modules/vr/VRWebGLEyeParameters.h"

#if !defined( EGL_OPENGL_ES3_BIT_KHR )
#define EGL_OPENGL_ES3_BIT_KHR		0x0040
#endif

#if !defined( GL_EXT_multisampled_render_to_texture )
typedef void (GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);
#endif

#include "VrApi.h"
#include "VrApi_Helpers.h"

#include "SystemActivities.h"

#define DEBUG 1
#define LOG_TAG "VRWebGL"

#define ALOGE(...) __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__ )
#if DEBUG
#define ALOGV(...) __android_log_print( ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__ )
#else
#define ALOGV(...)
#endif

static const int CPU_LEVEL			= 2;
static const int GPU_LEVEL			= 3;

#define MULTI_THREADED			0
#define REDUCED_LATENCY			0
#define EXPLICIT_GL_OBJECTS		0

// VRWEBGL BEGIN
// #define RENDER_NATIVE_TRIANGLE  1
// #define USE_SCENE               1
// VRWEBGL END

// VRWEBGL BEGIN (forward declarations)

// ================================================================================
// Forward declarations of some structures so they can be used/referenced.
// Their corresponding functions can be found down below.
// This is the problem of having just one source file :(
// ================================================================================


// ================================================================================
// ovrMessageQueue
// ================================================================================
enum ovrMQWait
{
    MQ_WAIT_NONE,		// don't wait
    MQ_WAIT_RECEIVED,	// wait until the consumer thread has received the message
    MQ_WAIT_PROCESSED	// wait until the consumer thread has processed the message
};

#define MAX_MESSAGE_PARMS	8
#define MAX_MESSAGES		1024

struct ovrMessage
{
    int			Id;
    ovrMQWait	Wait;
    long long	Parms[MAX_MESSAGE_PARMS];
    // VRWEBGL BEGIN
    void*		Result;
    // VRWEBGL END
} ;

static void ovrMessage_Init( ovrMessage * message, const int id, const ovrMQWait wait )
{
    message->Id = id;
    message->Wait = wait;
    memset( message->Parms, 0, sizeof( message->Parms ) );
    // VRWEBGL BEGIN
    message->Result = 0;
    // VRWEBGL END
}

static void		ovrMessage_SetPointerParm( ovrMessage * message, int index, void * ptr ) { *(void **)&message->Parms[index] = ptr; }
static void *	ovrMessage_GetPointerParm( ovrMessage * message, int index ) { return *(void **)&message->Parms[index]; }
static void		ovrMessage_SetIntegerParm( ovrMessage * message, int index, int value ) { message->Parms[index] = value; }
static int		ovrMessage_GetIntegerParm( ovrMessage * message, int index ) { return (int)message->Parms[index]; }
static void		ovrMessage_SetGLuintParm( ovrMessage * message, int index, GLuint value ) { message->Parms[index] = value; }
static GLuint	ovrMessage_GetGLuintParm( ovrMessage * message, int index ) { return (GLuint)message->Parms[index]; }
static void		ovrMessage_SetFloatParm( ovrMessage * message, int index, float value ) { *(float *)&message->Parms[index] = value; }
static float	ovrMessage_GetFloatParm( ovrMessage * message, int index ) { return *(float *)&message->Parms[index]; }

// Cyclic queue with messages.
struct ovrMessageQueue
{
    ovrMessage	 		Messages[MAX_MESSAGES];
    volatile int		Head;	// dequeue at the head
    volatile int		Tail;	// enqueue at the tail
    ovrMQWait			Wait;
    volatile bool		EnabledFlag;
    volatile bool		PostedFlag;
    volatile bool		ReceivedFlag;
    volatile bool		ProcessedFlag;
    pthread_mutex_t		Mutex;
    pthread_cond_t		PostedCondition;
    pthread_cond_t		ReceivedCondition;
    pthread_cond_t		ProcessedCondition;
};

// ================================================================================
// ovrAppThread
// ================================================================================
struct ovrAppThread
{
    JavaVM *		JavaVm;
    jobject			ActivityObject;
    // VRWEBGL BEGIN
    jclass          activityObjectJClass;
    jmethodID       logHeapUsageMethodID;
    jmethodID       seekToMethodId;
    jmethodID       pauseVideoMethodId;
    jmethodID       stopVideoMethodId;
    jmethodID       playVideoOnUIThreadMethodId;
    jmethodID       setVideoSrcOnUIThreadMethodId;
    jmethodID       newVideoMethodId;
    jmethodID       deleteVideoMethodId;
    jmethodID		setVideoVolumeMethodId;
    jmethodID		setVideoLoopMethodId;
    jmethodID		getVideoDurationMethodId;
    jmethodID		getVideoWidthMethodId;
    jmethodID		getVideoHeightMethodId;
    jmethodID		getVideoCurrentTimeMethodId;
    GLfloat 		projectionMatrixTransposed[16];
    GLfloat 		viewMatrixTransposed[16];
    bool			renderEnabled;
    // VRWEBGL END
    pthread_t		Thread;
    ovrMessageQueue	MessageQueue;
    ANativeWindow * NativeWindow;
};

// VRWEBGL END (forward declarations)

// VRWEBGL BEGIN

// TODO: The following class has been copied from the Oculus SDK. For some reason
// I am not able to compile/link correctly against it because I get a runtime crash.
// It would be ideal to be able to remove this code and use the Oculus SDK directly.

// SurfaceTextures are used to get video frames, Camera
// previews, and Android canvas views.
//
// Note that these never have mipmaps, so you will often
// want to render them to another texture and generate mipmaps
// to avoid aliasing when drawing, unless you know it will
// always be magnified.
//
// Note that we do not get and use the TransformMatrix
// from java.  Presumably this was only necessary before
// non-power-of-two textures became ubiquitous.
class SurfaceTexture
{
public:
							SurfaceTexture( JNIEnv * jni_ );
							~SurfaceTexture();

	// For some java-side uses, you can set the size
	// of the buffer before it is used to control how
	// large it is.  Video decompression and camera preview
	// always override the size automatically.
	void					SetDefaultBufferSize( const int width, const int height );

	// This can only be called with an active GL context.
	// As a side effect, the textureId will be bound to the
	// GL_TEXTURE_EXTERNAL_OES target of the currently active
	// texture unit.
	void 					Update();

	unsigned 				GetTextureId();
	jobject					GetJavaObject();
	long long				GetNanoTimeStamp();

private:
	unsigned				textureId;
	jobject					javaObject;
	JNIEnv * 				jni;

	// Updated when Update() is called, can be used to
	// check if a new frame is available and ready
	// to be processed / mipmapped by other code.
	long long				nanoTimeStamp;

	jmethodID 				updateTexImageMethodId;
	jmethodID 				getTimestampMethodId;
	jmethodID 				setDefaultBufferSizeMethodId;
};

SurfaceTexture::SurfaceTexture( JNIEnv * jni_ ) :
	textureId( 0 ),
	javaObject( NULL ),
	jni( NULL ),
	nanoTimeStamp( 0 ),
	updateTexImageMethodId( NULL ),
	getTimestampMethodId( NULL ),
	setDefaultBufferSizeMethodId( NULL )
{
	jni = jni_;

	// Gen a gl texture id for the java SurfaceTexture to use.
	glGenTextures( 1, &textureId );
	glBindTexture( GL_TEXTURE_EXTERNAL_OES, textureId );
	glTexParameterf( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameterf( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );

	static const char * className = "android/graphics/SurfaceTexture";
	const jclass surfaceTextureClass = jni->FindClass(className);
	if ( surfaceTextureClass == 0 ) {
		ALOGE( "FindClass( %s ) failed", className );
	}

	// find the constructor that takes an int
	const jmethodID constructor = jni->GetMethodID( surfaceTextureClass, "<init>", "(I)V" );
	if ( constructor == 0 ) {
		ALOGE( "GetMethodID( <init> ) failed" );
	}

	jobject obj = jni->NewObject( surfaceTextureClass, constructor, textureId );
	if ( obj == 0 ) {
		ALOGE( "NewObject() failed" );
	}

	javaObject = jni->NewGlobalRef( obj );
	if ( javaObject == 0 ) {
		ALOGE( "NewGlobalRef() failed" );
	}

	// Now that we have a globalRef, we can free the localRef
	jni->DeleteLocalRef( obj );

    updateTexImageMethodId = jni->GetMethodID( surfaceTextureClass, "updateTexImage", "()V" );
    if ( !updateTexImageMethodId ) {
    	ALOGE( "couldn't get updateTexImageMethodId" );
    }

    getTimestampMethodId = jni->GetMethodID( surfaceTextureClass, "getTimestamp", "()J" );
    if ( !getTimestampMethodId ) {
    	ALOGE( "couldn't get getTimestampMethodId" );
    }

	setDefaultBufferSizeMethodId = jni->GetMethodID( surfaceTextureClass, "setDefaultBufferSize", "(II)V" );
    if ( !setDefaultBufferSizeMethodId ) {
    	ALOGE( "couldn't get setDefaultBufferSize" );
    }

	// jclass objects are localRefs that need to be freed
	jni->DeleteLocalRef( surfaceTextureClass );
}

SurfaceTexture::~SurfaceTexture()
{
	if ( textureId ) {
		glDeleteTextures( 1, &textureId );
		textureId = 0;
	}
	if ( javaObject ) {
		jni->DeleteGlobalRef( javaObject );
		javaObject = 0;
	}
}

void SurfaceTexture::SetDefaultBufferSize( const int width, const int height )
{
	jni->CallVoidMethod( javaObject, setDefaultBufferSizeMethodId, width, height );
//	OVR_UNUSED( width );
//	OVR_UNUSED( height );
}

void SurfaceTexture::Update()
{
    // latch the latest video frame to the texture
    if ( !javaObject ) {
    	return;
    }

   jni->CallVoidMethod( javaObject, updateTexImageMethodId );
   nanoTimeStamp = jni->CallLongMethod( javaObject, getTimestampMethodId );
}

unsigned SurfaceTexture::GetTextureId()
{
	return textureId;
}

jobject SurfaceTexture::GetJavaObject()
{
	return javaObject;
}

long long SurfaceTexture::GetNanoTimeStamp()
{
	return nanoTimeStamp;
}
// VRWEBGL END

/*
================================================================================

OpenGL-ES Utility Functions

================================================================================
*/

static const char * EglErrorString( const EGLint error )
{
	switch ( error )
	{
		case EGL_SUCCESS:				return "EGL_SUCCESS";
		case EGL_NOT_INITIALIZED:		return "EGL_NOT_INITIALIZED";
		case EGL_BAD_ACCESS:			return "EGL_BAD_ACCESS";
		case EGL_BAD_ALLOC:				return "EGL_BAD_ALLOC";
		case EGL_BAD_ATTRIBUTE:			return "EGL_BAD_ATTRIBUTE";
		case EGL_BAD_CONTEXT:			return "EGL_BAD_CONTEXT";
		case EGL_BAD_CONFIG:			return "EGL_BAD_CONFIG";
		case EGL_BAD_CURRENT_SURFACE:	return "EGL_BAD_CURRENT_SURFACE";
		case EGL_BAD_DISPLAY:			return "EGL_BAD_DISPLAY";
		case EGL_BAD_SURFACE:			return "EGL_BAD_SURFACE";
		case EGL_BAD_MATCH:				return "EGL_BAD_MATCH";
		case EGL_BAD_PARAMETER:			return "EGL_BAD_PARAMETER";
		case EGL_BAD_NATIVE_PIXMAP:		return "EGL_BAD_NATIVE_PIXMAP";
		case EGL_BAD_NATIVE_WINDOW:		return "EGL_BAD_NATIVE_WINDOW";
		case EGL_CONTEXT_LOST:			return "EGL_CONTEXT_LOST";
		default:						return "unknown";
	}
}

static const char * GlFrameBufferStatusString( GLenum status )
{
	switch ( status )
	{
		case GL_FRAMEBUFFER_UNDEFINED:						return "GL_FRAMEBUFFER_UNDEFINED";
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
		case GL_FRAMEBUFFER_UNSUPPORTED:					return "GL_FRAMEBUFFER_UNSUPPORTED";
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:			return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
		default:											return "unknown";
	}
}

#ifdef CHECK_GL_ERRORS

static const char * GlErrorString( GLenum error )
{
	switch ( error )
	{
		case GL_NO_ERROR:						return "GL_NO_ERROR";
		case GL_INVALID_ENUM:					return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:					return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:				return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION:	return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY:					return "GL_OUT_OF_MEMORY";
		default: return "unknown";
	}
}

static void GLCheckErrors()
{
	for ( int i = 0; i < 10; i++ )
	{
		const GLenum error = glGetError();
		if ( error == GL_NO_ERROR )
		{
			break;
		}
		ALOGE( "GL error: %s", GlErrorString( error ) );
	}
}

#define GL( func )		func; GLCheckErrors();

#else // CHECK_GL_ERRORS

#define GL( func )		func;

#endif // CHECK_GL_ERRORS

/*
================================================================================

ovrEgl

================================================================================
*/

struct ovrEgl
{
	EGLint		MajorVersion;
	EGLint		MinorVersion;
	EGLDisplay	Display;
	EGLConfig	Config;
	EGLSurface	TinySurface;
	EGLSurface	MainSurface;
	EGLContext	Context;
};

static void ovrEgl_Clear( ovrEgl * egl )
{
	egl->MajorVersion = 0;
	egl->MinorVersion = 0;
	egl->Display = 0;
	egl->Config = 0;
	egl->TinySurface = EGL_NO_SURFACE;
	egl->MainSurface = EGL_NO_SURFACE;
	egl->Context = EGL_NO_CONTEXT;
}

static void ovrEgl_CreateContext( ovrEgl * egl, const ovrEgl * shareEgl )
{
	if ( egl->Display != 0 )
	{
		return;
	}

	egl->Display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	ALOGV( "        eglInitialize( Display, &MajorVersion, &MinorVersion )" );
	eglInitialize( egl->Display, &egl->MajorVersion, &egl->MinorVersion );
	// Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
	// flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
	// settings, and that is completely wasted for our warp target.
	const int MAX_CONFIGS = 1024;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs = 0;
	if ( eglGetConfigs( egl->Display, configs, MAX_CONFIGS, &numConfigs ) == EGL_FALSE )
	{
		ALOGE( "        eglGetConfigs() failed: %s", EglErrorString( eglGetError() ) );
		return;
	}
	const EGLint configAttribs[] =
	{
		EGL_ALPHA_SIZE, 8, // need alpha for the multi-pass timewarp compositor
		EGL_BLUE_SIZE,  8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE,   8,
		EGL_DEPTH_SIZE, 0,
		EGL_SAMPLES,	0,
		EGL_NONE
	};
	egl->Config = 0;
	for ( int i = 0; i < numConfigs; i++ )
	{
		EGLint value = 0;

		eglGetConfigAttrib( egl->Display, configs[i], EGL_RENDERABLE_TYPE, &value );
		if ( ( value & EGL_OPENGL_ES3_BIT_KHR ) != EGL_OPENGL_ES3_BIT_KHR )
		{
			continue;
		}

		// The pbuffer config also needs to be compatible with normal window rendering
		// so it can share textures with the window context.
		eglGetConfigAttrib( egl->Display, configs[i], EGL_SURFACE_TYPE, &value );
		if ( ( value & ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) ) != ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) )
		{
			continue;
		}

		int	j = 0;
		for ( ; configAttribs[j] != EGL_NONE; j += 2 )
		{
			eglGetConfigAttrib( egl->Display, configs[i], configAttribs[j], &value );
			if ( value != configAttribs[j + 1] )
			{
				break;
			}
		}
		if ( configAttribs[j] == EGL_NONE )
		{
			egl->Config = configs[i];
			break;
		}
	}
	if ( egl->Config == 0 )
	{
		ALOGE( "        eglChooseConfig() failed: %s", EglErrorString( eglGetError() ) );
		return;
	}
	EGLint contextAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
	ALOGV( "        Context = eglCreateContext( Display, Config, EGL_NO_CONTEXT, contextAttribs )" );
	egl->Context = eglCreateContext( egl->Display, egl->Config, ( shareEgl != NULL ) ? shareEgl->Context : EGL_NO_CONTEXT, contextAttribs );
	if ( egl->Context == EGL_NO_CONTEXT )
	{
		ALOGE( "        eglCreateContext() failed: %s", EglErrorString( eglGetError() ) );
		return;
	}
	const EGLint surfaceAttribs[] =
	{
		EGL_WIDTH, 16,
		EGL_HEIGHT, 16,
		EGL_NONE
	};
	ALOGV( "        TinySurface = eglCreatePbufferSurface( Display, Config, surfaceAttribs )" );
	egl->TinySurface = eglCreatePbufferSurface( egl->Display, egl->Config, surfaceAttribs );
	if ( egl->TinySurface == EGL_NO_SURFACE )
	{
		ALOGE( "        eglCreatePbufferSurface() failed: %s", EglErrorString( eglGetError() ) );
		eglDestroyContext( egl->Display, egl->Context );
		egl->Context = EGL_NO_CONTEXT;
		return;
	}
	ALOGV( "        eglMakeCurrent( Display, TinySurface, TinySurface, Context )" );
	if ( eglMakeCurrent( egl->Display, egl->TinySurface, egl->TinySurface, egl->Context ) == EGL_FALSE )
	{
		ALOGE( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
		eglDestroySurface( egl->Display, egl->TinySurface );
		eglDestroyContext( egl->Display, egl->Context );
		egl->Context = EGL_NO_CONTEXT;
		return;
	}
}

static void ovrEgl_DestroyContext( ovrEgl * egl )
{
	if ( egl->Display != 0 )
	{
		ALOGE( "        eglMakeCurrent( Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )" );
		if ( eglMakeCurrent( egl->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) == EGL_FALSE )
		{
			ALOGE( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
		}
	}
	if ( egl->Context != EGL_NO_CONTEXT )
	{
		ALOGE( "        eglDestroyContext( Display, Context )" );
		if ( eglDestroyContext( egl->Display, egl->Context ) == EGL_FALSE )
		{
			ALOGE( "        eglDestroyContext() failed: %s", EglErrorString( eglGetError() ) );
		}
		egl->Context = EGL_NO_CONTEXT;
	}
	if ( egl->TinySurface != EGL_NO_SURFACE )
	{
		ALOGE( "        eglDestroySurface( Display, TinySurface )" );
		if ( eglDestroySurface( egl->Display, egl->TinySurface ) == EGL_FALSE )
		{
			ALOGE( "        eglDestroySurface() failed: %s", EglErrorString( eglGetError() ) );
		}
		egl->TinySurface = EGL_NO_SURFACE;
	}
	if ( egl->Display != 0 )
	{
		ALOGE( "        eglTerminate( Display )" );
		if ( eglTerminate( egl->Display ) == EGL_FALSE )
		{
			ALOGE( "        eglTerminate() failed: %s", EglErrorString( eglGetError() ) );
		}
		egl->Display = 0;
	}
}

static void ovrEgl_CreateSurface( ovrEgl * egl, ANativeWindow * nativeWindow )
{
	if ( egl->MainSurface != EGL_NO_SURFACE )
	{
		return;
	}
	ALOGV( "        MainSurface = eglCreateWindowSurface( Display, Config, nativeWindow, attribs )" );
	const EGLint surfaceAttribs[] = { EGL_NONE };
	egl->MainSurface = eglCreateWindowSurface( egl->Display, egl->Config, nativeWindow, surfaceAttribs );
	if ( egl->MainSurface == EGL_NO_SURFACE )
	{
		ALOGE( "        eglCreateWindowSurface() failed: %s", EglErrorString( eglGetError() ) );
		return;
	}
#if EXPLICIT_GL_OBJECTS == 0
	ALOGV( "        eglMakeCurrent( display, MainSurface, MainSurface, Context )" );
	if ( eglMakeCurrent( egl->Display, egl->MainSurface, egl->MainSurface, egl->Context ) == EGL_FALSE )
	{
		ALOGE( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
		return;
	}
#endif
}

static void ovrEgl_DestroySurface( ovrEgl * egl )
{
#if EXPLICIT_GL_OBJECTS == 0
	if ( egl->Context != EGL_NO_CONTEXT )
	{
		ALOGV( "        eglMakeCurrent( display, TinySurface, TinySurface, Context )" );
		if ( eglMakeCurrent( egl->Display, egl->TinySurface, egl->TinySurface, egl->Context ) == EGL_FALSE )
		{
			ALOGE( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
		}
	}
#endif
	if ( egl->MainSurface != EGL_NO_SURFACE )
	{
		ALOGV( "        eglDestroySurface( Display, MainSurface )" );
		if ( eglDestroySurface( egl->Display, egl->MainSurface ) == EGL_FALSE )
		{
			ALOGE( "        eglDestroySurface() failed: %s", EglErrorString( eglGetError() ) );
		}
		egl->MainSurface = EGL_NO_SURFACE;
	}
}

/*
================================================================================

ovrGeometry

================================================================================
*/

struct ovrVertexAttribPointer
{
	GLuint			Index;
 	GLint			Size;
 	GLenum			Type;
 	GLboolean		Normalized;
 	GLsizei			Stride;
 	const GLvoid *	Pointer;
};

#define MAX_VERTEX_ATTRIB_POINTERS		3

struct ovrGeometry
{
	GLuint					VertexBuffer;
	GLuint					IndexBuffer;
	GLuint					VertexArrayObject;
	int						VertexCount;
	int 					IndexCount;
	ovrVertexAttribPointer	VertexAttribs[MAX_VERTEX_ATTRIB_POINTERS];
};

enum ovrVertexAttributeLocation
{
	VERTEX_ATTRIBUTE_LOCATION_POSITION,
	VERTEX_ATTRIBUTE_LOCATION_COLOR,
	VERTEX_ATTRIBUTE_LOCATION_UV,
	VERTEX_ATTRIBUTE_LOCATION_TRANSFORM
};

struct ovrVertexAttribute
{
	ovrVertexAttributeLocation	location;
	const char *				name;
};

static ovrVertexAttribute ProgramVertexAttributes[] =
{
	{ VERTEX_ATTRIBUTE_LOCATION_POSITION,	"vertexPosition" },
	{ VERTEX_ATTRIBUTE_LOCATION_COLOR,		"vertexColor" },
	{ VERTEX_ATTRIBUTE_LOCATION_UV,			"vertexUv" },
	{ VERTEX_ATTRIBUTE_LOCATION_TRANSFORM,	"vertexTransform" }
};

static void ovrGeometry_Clear( ovrGeometry * geometry )
{
	geometry->VertexBuffer = 0;
	geometry->IndexBuffer = 0;
	geometry->VertexArrayObject = 0;
	geometry->VertexCount = 0;
	geometry->IndexCount = 0;
	for ( int i = 0; i < MAX_VERTEX_ATTRIB_POINTERS; i++ )
	{
		memset( &geometry->VertexAttribs[i], 0, sizeof( geometry->VertexAttribs[i] ) );
		geometry->VertexAttribs[i].Index = -1;
	}
}

static void ovrGeometry_CreateCube( ovrGeometry * geometry )
{
	struct ovrCubeVertices
	{
		char positions[8][4];
		unsigned char colors[8][4];
	};

	static const ovrCubeVertices cubeVertices =
	{
		// positions
		{
			{ static_cast<char>(-127), +127, static_cast<char>(-127), +127 }, { +127, +127, static_cast<char>(-127), +127 }, { +127, +127, +127, +127 }, { static_cast<char>(-127), +127, +127, +127 },	// top
			{ static_cast<char>(-127), static_cast<char>(-127), static_cast<char>(-127), +127 }, { static_cast<char>(-127), static_cast<char>(-127), +127, +127 }, { +127, static_cast<char>(-127), +127, +127 }, { +127, static_cast<char>(-127), static_cast<char>(-127), +127 }	// bottom
		},
		// colors
		{
			{ 255,   0, 255, 255 }, {   0, 255,   0, 255 }, {   0,   0, 255, 255 }, { 255,   0,   0, 255 },
			{   0,   0, 255, 255 }, {   0, 255,   0, 255 }, { 255,   0, 255, 255 }, { 255,   0,   0, 255 }
		},
	};

	static const unsigned short cubeIndices[36] =
	{
		0, 1, 2, 2, 3, 0,	// top
		4, 5, 6, 6, 7, 4,	// bottom
		2, 6, 7, 7, 1, 2,	// right
		0, 4, 5, 5, 3, 0,	// left
		3, 5, 6, 6, 2, 3,	// front
		0, 1, 7, 7, 4, 0	// back
	};

	geometry->VertexCount = 8;
	geometry->IndexCount = 36;

	geometry->VertexAttribs[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
 	geometry->VertexAttribs[0].Size = 4;
 	geometry->VertexAttribs[0].Type = GL_BYTE;
 	geometry->VertexAttribs[0].Normalized = true;
 	geometry->VertexAttribs[0].Stride = sizeof( cubeVertices.positions[0] );
 	geometry->VertexAttribs[0].Pointer = (const GLvoid *)offsetof( ovrCubeVertices, positions );

	geometry->VertexAttribs[1].Index = VERTEX_ATTRIBUTE_LOCATION_COLOR;
 	geometry->VertexAttribs[1].Size = 4;
 	geometry->VertexAttribs[1].Type = GL_UNSIGNED_BYTE;
 	geometry->VertexAttribs[1].Normalized = true;
 	geometry->VertexAttribs[1].Stride = sizeof( cubeVertices.colors[0] );
 	geometry->VertexAttribs[1].Pointer = (const GLvoid *)offsetof( ovrCubeVertices, colors );

	GL( glGenBuffers( 1, &geometry->VertexBuffer ) );
	GL( glBindBuffer( GL_ARRAY_BUFFER, geometry->VertexBuffer ) );
	GL( glBufferData( GL_ARRAY_BUFFER, sizeof( cubeVertices ), &cubeVertices, GL_STATIC_DRAW ) );
	GL( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );

	GL( glGenBuffers( 1, &geometry->IndexBuffer ) );
	GL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geometry->IndexBuffer ) );
	GL( glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( cubeIndices ), cubeIndices, GL_STATIC_DRAW ) );
	GL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ) );
}

static void ovrGeometry_Destroy( ovrGeometry * geometry )
{
	GL( glDeleteBuffers( 1, &geometry->IndexBuffer ) );
	GL( glDeleteBuffers( 1, &geometry->VertexBuffer ) );

	ovrGeometry_Clear( geometry );
}

static void ovrGeometry_CreateVAO( ovrGeometry * geometry )
{
	GL( glGenVertexArrays( 1, &geometry->VertexArrayObject ) );
	GL( glBindVertexArray( geometry->VertexArrayObject ) );

	GL( glBindBuffer( GL_ARRAY_BUFFER, geometry->VertexBuffer ) );

	for ( int i = 0; i < MAX_VERTEX_ATTRIB_POINTERS; i++ )
	{
		if ( geometry->VertexAttribs[i].Index != -1 )
		{
            
            ALOGV("VRWEBGL: glEnableVertexAttribArray(%d)", geometry->VertexAttribs[i].Index);
            
			GL( glEnableVertexAttribArray( geometry->VertexAttribs[i].Index ) );
			GL( glVertexAttribPointer( geometry->VertexAttribs[i].Index, geometry->VertexAttribs[i].Size,
					geometry->VertexAttribs[i].Type, geometry->VertexAttribs[i].Normalized,
					geometry->VertexAttribs[i].Stride, geometry->VertexAttribs[i].Pointer ) );
		}
	}

	GL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geometry->IndexBuffer ) );

	GL( glBindVertexArray( 0 ) );
}

static void ovrGeometry_DestroyVAO( ovrGeometry * geometry )
{
	GL( glDeleteVertexArrays( 1, &geometry->VertexArrayObject ) );
}

/*
================================================================================

ovrProgram

================================================================================
*/

#define MAX_PROGRAM_UNIFORMS	8
#define MAX_PROGRAM_TEXTURES	8

struct ovrProgram
{
	GLuint	Program;
	GLuint	VertexShader;
	GLuint	FragmentShader;
	// These will be -1 if not used by the program.
	GLint	Uniforms[MAX_PROGRAM_UNIFORMS];		// ProgramUniforms[].name
	GLint	Textures[MAX_PROGRAM_TEXTURES];		// Texture%i
};

enum ovrUniformIndex
{
	UNIFORM_MODEL_MATRIX,
	UNIFORM_VIEW_MATRIX,
	UNIFORM_PROJECTION_MATRIX
};

enum ovrUniformType
{
	UNIFORM_TYPE_VECTOR4,
	UNIFORM_TYPE_MATRIX4X4
};

struct ovrUniform
{
	ovrUniformIndex index;
	ovrUniformType	type;
	const char *	name;
};

static ovrUniform ProgramUniforms[] =
{
	{ UNIFORM_MODEL_MATRIX,			UNIFORM_TYPE_MATRIX4X4,	"ModelMatrix" },
	{ UNIFORM_VIEW_MATRIX,			UNIFORM_TYPE_MATRIX4X4,	"ViewMatrix" },
	{ UNIFORM_PROJECTION_MATRIX,	UNIFORM_TYPE_MATRIX4X4,	"ProjectionMatrix" }
};

static void ovrProgram_Clear( ovrProgram * program )
{
	program->Program = 0;
	program->VertexShader = 0;
	program->FragmentShader = 0;
	memset( program->Uniforms, 0, sizeof( program->Uniforms ) );
	memset( program->Textures, 0, sizeof( program->Textures ) );
}

static bool ovrProgram_Create( ovrProgram * program, const char * vertexSource, const char * fragmentSource )
{
	GLint r;

	GL( program->VertexShader = glCreateShader( GL_VERTEX_SHADER ) );
	GL( glShaderSource( program->VertexShader, 1, &vertexSource, 0 ) );
	GL( glCompileShader( program->VertexShader ) );
	GL( glGetShaderiv( program->VertexShader, GL_COMPILE_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GL( glGetShaderInfoLog( program->VertexShader, sizeof( msg ), 0, msg ) );
		ALOGE( "%s\n%s\n", vertexSource, msg );
		return false;
	}

	GL( program->FragmentShader = glCreateShader( GL_FRAGMENT_SHADER ) );
	GL( glShaderSource( program->FragmentShader, 1, &fragmentSource, 0 ) );
	GL( glCompileShader( program->FragmentShader ) );
	GL( glGetShaderiv( program->FragmentShader, GL_COMPILE_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GL( glGetShaderInfoLog( program->FragmentShader, sizeof( msg ), 0, msg ) );
		ALOGE( "%s\n%s\n", fragmentSource, msg );
		return false;
	}

	GL( program->Program = glCreateProgram() );
	GL( glAttachShader( program->Program, program->VertexShader ) );
	GL( glAttachShader( program->Program, program->FragmentShader ) );

	// Bind the vertex attribute locations.
	for ( int i = 0; i < sizeof( ProgramVertexAttributes ) / sizeof( ProgramVertexAttributes[0] ); i++ )
	{
		GL( glBindAttribLocation( program->Program, ProgramVertexAttributes[i].location, ProgramVertexAttributes[i].name ) );
	}

	GL( glLinkProgram( program->Program ) );
	GL( glGetProgramiv( program->Program, GL_LINK_STATUS, &r ) );
	if ( r == GL_FALSE )
	{
		GLchar msg[4096];
		GL( glGetProgramInfoLog( program->Program, sizeof( msg ), 0, msg ) );
		ALOGE( "Linking program failed: %s\n", msg );
		return false;
	}

	// Get the uniform locations.
	memset( program->Uniforms, -1, sizeof( program->Uniforms ) );
	for ( int i = 0; i < sizeof( ProgramUniforms ) / sizeof( ProgramUniforms[0] ); i++ )
	{
		program->Uniforms[ProgramUniforms[i].index] = glGetUniformLocation( program->Program, ProgramUniforms[i].name );
	}

	GL( glUseProgram( program->Program ) );

	// Get the texture locations.
	for ( int i = 0; i < MAX_PROGRAM_TEXTURES; i++ )
	{
		char name[32];
		sprintf( name, "Texture%i", i );
		program->Textures[i] = glGetUniformLocation( program->Program, name );
		if ( program->Textures[i] != -1 )
		{
			GL( glUniform1i( program->Textures[i], i ) );
		}
	}

	GL( glUseProgram( 0 ) );

	return true;
}

static void ovrProgram_Destroy( ovrProgram * program )
{
	if ( program->Program != 0 )
	{
		GL( glDeleteProgram( program->Program ) );
		program->Program = 0;
	}
	if ( program->VertexShader != 0 )
	{
		GL( glDeleteShader( program->VertexShader ) );
		program->VertexShader = 0;
	}
	if ( program->FragmentShader != 0 )
	{
		GL( glDeleteShader( program->FragmentShader ) );
		program->FragmentShader = 0;
	}
}

/*
================================================================================

ovrFramebuffer

================================================================================
*/

struct ovrFramebuffer
{
	int						Width;
	int						Height;
	int						Multisamples;
	int						TextureSwapChainLength;
	int						TextureSwapChainIndex;
	ovrTextureSwapChain *	ColorTextureSwapChain;
	GLuint *				DepthBuffers;
	GLuint *				FrameBuffers;
};

static void ovrFramebuffer_Clear( ovrFramebuffer * frameBuffer )
{
	frameBuffer->Width = 0;
	frameBuffer->Height = 0;
	frameBuffer->Multisamples = 0;
	frameBuffer->TextureSwapChainLength = 0;
	frameBuffer->TextureSwapChainIndex = 0;
	frameBuffer->ColorTextureSwapChain = NULL;
	frameBuffer->DepthBuffers = NULL;
	frameBuffer->FrameBuffers = NULL;
}

static bool ovrFramebuffer_Create( ovrFramebuffer * frameBuffer, const ovrTextureFormat colorFormat, const int width, const int height, const int multisamples )
{
	frameBuffer->Width = width;
	frameBuffer->Height = height;
	frameBuffer->Multisamples = multisamples;

	frameBuffer->ColorTextureSwapChain = vrapi_CreateTextureSwapChain( VRAPI_TEXTURE_TYPE_2D, colorFormat, width, height, 1, true );
	frameBuffer->TextureSwapChainLength = vrapi_GetTextureSwapChainLength( frameBuffer->ColorTextureSwapChain );
	frameBuffer->DepthBuffers = (GLuint *)malloc( frameBuffer->TextureSwapChainLength * sizeof( GLuint ) ); 
	frameBuffer->FrameBuffers = (GLuint *)malloc( frameBuffer->TextureSwapChainLength * sizeof( GLuint ) );

	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
		(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)eglGetProcAddress( "glRenderbufferStorageMultisampleEXT" );
	PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
		(PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress( "glFramebufferTexture2DMultisampleEXT" );

	for ( int i = 0; i < frameBuffer->TextureSwapChainLength; i++ )
	{
		// Create the color buffer texture.
		const GLuint colorTexture = vrapi_GetTextureSwapChainHandle( frameBuffer->ColorTextureSwapChain, i );
		GL( glBindTexture( GL_TEXTURE_2D, colorTexture ) );
		GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
		GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );
		GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
		GL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
		GL( glBindTexture( GL_TEXTURE_2D, 0 ) );

		if ( multisamples > 1 && glRenderbufferStorageMultisampleEXT != NULL && glFramebufferTexture2DMultisampleEXT != NULL )
		{
			// Create multisampled depth buffer.
			GL( glGenRenderbuffers( 1, &frameBuffer->DepthBuffers[i] ) );
			GL( glBindRenderbuffer( GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
			GL( glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER, multisamples, GL_DEPTH_COMPONENT24, width, height ) );
			GL( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );

			// Create the frame buffer.
			GL( glGenFramebuffers( 1, &frameBuffer->FrameBuffers[i] ) );
			GL( glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer->FrameBuffers[i] ) );
			GL( glFramebufferTexture2DMultisampleEXT( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0, multisamples ) );
			GL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
			GL( GLenum renderFramebufferStatus = glCheckFramebufferStatus( GL_FRAMEBUFFER ) );
			GL( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
			if ( renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE )
			{
				ALOGE( "Incomplete frame buffer object: %s", GlFrameBufferStatusString( renderFramebufferStatus ) );
				return false;
			}
		}
		else
		{
			// Create depth buffer.
			GL( glGenRenderbuffers( 1, &frameBuffer->DepthBuffers[i] ) );
			GL( glBindRenderbuffer( GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
			GL( glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height ) );
			GL( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );

			// Create the frame buffer.
			GL( glGenFramebuffers( 1, &frameBuffer->FrameBuffers[i] ) );
			GL( glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer->FrameBuffers[i] ) );
			GL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
			GL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0 ) );
			GL( GLenum renderFramebufferStatus = glCheckFramebufferStatus( GL_FRAMEBUFFER ) );
			GL( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
			if ( renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE )
			{
				ALOGE( "Incomplete frame buffer object: %s", GlFrameBufferStatusString( renderFramebufferStatus ) );
				return false;
			}
		}
	}

	return true;
}

static void ovrFramebuffer_Destroy( ovrFramebuffer * frameBuffer )
{
	GL( glDeleteFramebuffers( frameBuffer->TextureSwapChainLength, frameBuffer->FrameBuffers ) );
	GL( glDeleteRenderbuffers( frameBuffer->TextureSwapChainLength, frameBuffer->DepthBuffers ) );
	vrapi_DestroyTextureSwapChain( frameBuffer->ColorTextureSwapChain );

	free( frameBuffer->DepthBuffers );
	free( frameBuffer->FrameBuffers );

	ovrFramebuffer_Clear( frameBuffer );
}

static void ovrFramebuffer_SetCurrent( ovrFramebuffer * frameBuffer )
{
	GL( glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer->FrameBuffers[frameBuffer->TextureSwapChainIndex] ) );
}

static GLuint ovrFramebuffer_GetCurrent(  ovrFramebuffer * frameBuffer )
{
	return frameBuffer->FrameBuffers[frameBuffer->TextureSwapChainIndex];
}

static void ovrFramebuffer_SetNone()
{
	GL( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
}

static void ovrFramebuffer_Resolve( ovrFramebuffer * frameBuffer )
{
	// Discard the depth buffer, so the tiler won't need to write it back out to memory.
	const GLenum depthAttachment[1] = { GL_DEPTH_ATTACHMENT };
	glInvalidateFramebuffer( GL_FRAMEBUFFER, 1, depthAttachment );

	// Flush this frame worth of commands.
	glFlush();
}

static void ovrFramebuffer_Advance( ovrFramebuffer * frameBuffer )
{
	// Advance to the next texture from the set.
	frameBuffer->TextureSwapChainIndex = ( frameBuffer->TextureSwapChainIndex + 1 ) % frameBuffer->TextureSwapChainLength;
}

#if USE_SCENE
/*
================================================================================

ovrScene

================================================================================
*/

#define NUM_INSTANCES		100 // 1500

struct ovrScene
{
	bool				CreatedScene;
	bool				CreatedVAOs;
	unsigned int		Random;
	ovrProgram			Program;
    // VRWEBGL BEGIN
    int                 programId;
    int                 fragmentShaderId;
    int                 vertexShaderId;
    int                 aVertexPositionId;
    int                 uMVMatrixId;
    int                 uPMatrixId;
    int                 bufferId;
    // VRWEBGL END
	ovrGeometry			Cube;
	GLuint				InstanceTransformBuffer;
	ovrVector3f			CubePositions[NUM_INSTANCES];
	ovrVector3f			CubeRotations[NUM_INSTANCES];
};

static const char VERTEX_SHADER[] =
	"#version 300 es\n"
	"in vec3 vertexPosition;\n"
	"in vec4 vertexColor;\n"
	"in mat4 vertexTransform;\n"
	"uniform mat4 ViewMatrix;\n"
	"uniform mat4 ProjectionMatrix;\n"
	"out vec4 fragmentColor;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = ProjectionMatrix * ( ViewMatrix * ( vertexTransform * vec4( vertexPosition, 1.0 ) ) );\n"
	"	fragmentColor = vertexColor;\n"
	"}\n";

static const char FRAGMENT_SHADER[] =
	"#version 300 es\n"
	"in lowp vec4 fragmentColor;\n"
	"out lowp vec4 outColor;\n"
	"void main()\n"
	"{\n"
	"	outColor = fragmentColor;\n"
	"}\n";

static const char VERTEX_SHADER_GLSL_CODE[] =
    "attribute vec3 aVertexPosition;"
    "uniform mat4 uMVMatrix;"
    "uniform mat4 uPMatrix;"
    "void main(void) {"
    "  gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0); "
    "}";
static const char FRAGMENT_SHADER_GLSL_CODE[] =
    "precision mediump float;"
    "void main(void) {"
    "  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);"
    "}";

static void ovrScene_Clear( ovrScene * scene )
{
	scene->CreatedScene = false;
	scene->CreatedVAOs = false;
	scene->Random = 2;
	scene->InstanceTransformBuffer = 0;

	ovrProgram_Clear( &scene->Program );
	ovrGeometry_Clear( &scene->Cube );
}

static bool ovrScene_IsCreated( ovrScene * scene )
{
	return scene->CreatedScene;
}

static void ovrScene_CreateVAOs( ovrScene * scene )
{
	if ( !scene->CreatedVAOs )
	{
		ovrGeometry_CreateVAO( &scene->Cube );

		// Modify the VAO to use the instance transform attributes.
		GL( glBindVertexArray( scene->Cube.VertexArrayObject ) );
		GL( glBindBuffer( GL_ARRAY_BUFFER, scene->InstanceTransformBuffer ) );
		for ( int i = 0; i < 4; i++ )
		{
			GL( glEnableVertexAttribArray( VERTEX_ATTRIBUTE_LOCATION_TRANSFORM + i ) );
			GL( glVertexAttribPointer( VERTEX_ATTRIBUTE_LOCATION_TRANSFORM + i, 4, GL_FLOAT,
										false, 4 * 4 * sizeof( float ), (void *)( i * 4 * sizeof( float ) ) ) );
			GL( glVertexAttribDivisor( VERTEX_ATTRIBUTE_LOCATION_TRANSFORM + i, 1 ) );
		}
		GL( glBindVertexArray( 0 ) );

		scene->CreatedVAOs = true;
	}
}

static void ovrScene_DestroyVAOs( ovrScene * scene )
{
	if ( scene->CreatedVAOs )
	{
		ovrGeometry_DestroyVAO( &scene->Cube );

		scene->CreatedVAOs = false;
	}
}

static float ovrScene_RandomFloat( ovrScene * scene )
{
	scene->Random = 1664525L * scene->Random + 1013904223L;
	unsigned int rf = 0x3F800000 | ( scene->Random & 0x007FFFFF );
	return (*(float *)&rf) - 1.0f;
}

static void ovrScene_Create( ovrScene * scene )
{
	ovrProgram_Create( &scene->Program, VERTEX_SHADER, FRAGMENT_SHADER );
    
	ovrGeometry_CreateCube( &scene->Cube );

	// Create the instance transform attribute buffer.
	GL( glGenBuffers( 1, &scene->InstanceTransformBuffer ) );
	GL( glBindBuffer( GL_ARRAY_BUFFER, scene->InstanceTransformBuffer ) );
	GL( glBufferData( GL_ARRAY_BUFFER, NUM_INSTANCES * 4 * 4 * sizeof( float ), NULL, GL_DYNAMIC_DRAW ) );
	GL( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );

	// Setup random cube positions and rotations.
	for ( int i = 0; i < NUM_INSTANCES; i++ )
	{
		// Using volatile keeps the compiler from optimizing away multiple calls to drand48().
		volatile float rx, ry, rz;
		for ( ; ; )
		{
			rx = ( ovrScene_RandomFloat( scene ) - 0.5f ) * ( 50.0f + sqrt( NUM_INSTANCES ) );
			ry = ( ovrScene_RandomFloat( scene ) - 0.5f ) * ( 50.0f + sqrt( NUM_INSTANCES ) );
			rz = ( ovrScene_RandomFloat( scene ) - 0.5f ) * ( 50.0f + sqrt( NUM_INSTANCES ) );
			// If too close to 0,0,0
			if ( fabsf( rx ) < 4.0f && fabsf( ry ) < 4.0f && fabsf( rz ) < 4.0f )
			{
				continue;
			}
			// Test for overlap with any of the existing cubes.
			bool overlap = false;
			for ( int j = 0; j < i; j++ )
			{
				if (	fabsf( rx - scene->CubePositions[j].x ) < 4.0f &&
						fabsf( ry - scene->CubePositions[j].y ) < 4.0f &&
						fabsf( rz - scene->CubePositions[j].z ) < 4.0f )
				{
					overlap = true;
					break;
				}
			}
			if ( !overlap )
			{
				break;
			}
		}

		// Insert into list sorted based on distance.
		int insert = 0;
		const float distSqr = rx * rx + ry * ry + rz * rz;
		for ( int j = i; j > 0; j-- )
		{
			const ovrVector3f * otherPos = &scene->CubePositions[j - 1];
			const float otherDistSqr = otherPos->x * otherPos->x + otherPos->y * otherPos->y + otherPos->z * otherPos->z;
			if ( distSqr > otherDistSqr )
			{
				insert = j;
				break;
			}
			scene->CubePositions[j] = scene->CubePositions[j - 1];
			scene->CubeRotations[j] = scene->CubeRotations[j - 1];
		}

		scene->CubePositions[insert].x = rx;
		scene->CubePositions[insert].y = ry;
		scene->CubePositions[insert].z = rz;

		scene->CubeRotations[insert].x = ovrScene_RandomFloat( scene );
		scene->CubeRotations[insert].y = ovrScene_RandomFloat( scene );
		scene->CubeRotations[insert].z = ovrScene_RandomFloat( scene );
	}
    
    // LUDEI BEGIN
#if RENDER_NATIVE_TRIANGLE
    GL( scene->fragmentShaderId = glCreateShader(35632) );
    const char* fragmentShaderGLSLCode = FRAGMENT_SHADER_GLSL_CODE;
    GL( glShaderSource(scene->fragmentShaderId, 1, &fragmentShaderGLSLCode, 0) );
    GL( glCompileShader(scene->fragmentShaderId) );
    GL( scene->vertexShaderId = glCreateShader(35633) );
    const char* vertexShaderGLSLCode = VERTEX_SHADER_GLSL_CODE;
    GL( glShaderSource(scene->vertexShaderId, 1, &vertexShaderGLSLCode, 0) );
    GL( glCompileShader(scene->vertexShaderId) );
    GL( scene->programId = glCreateProgram() );
    GL( glAttachShader(scene->programId, scene->fragmentShaderId) );
    GL( glAttachShader(scene->programId, scene->vertexShaderId) );
    GL( glLinkProgram(scene->programId) );
    GL( glUseProgram(scene->programId) );
    GL( scene->aVertexPositionId = glGetAttribLocation(scene->programId, "aVertexPosition") );
    GL( scene->uPMatrixId = glGetUniformLocation(scene->programId, "uPMatrix") );
    GL( scene-> uMVMatrixId = glGetUniformLocation(scene->programId, "uMVMatrix") );
    GLuint bufferIds[1];
    GL( glGenBuffers(1, bufferIds) );
    scene->bufferId = bufferIds[0];
    GL( glBindBuffer(34962, scene->bufferId) );
    float triangleCoords[] = {0.0f, 1.0f, -5.0f, -1.0f, -1.0f, -5.0f, 1.0f, -1.0f, -5.0f};
    GL( glBufferData(34962, sizeof(triangleCoords), &triangleCoords, 35044) );
    GL( glBindBuffer(34962, 0) );
    GL( glUseProgram(0) );
#endif
    // LUDEI END

	scene->CreatedScene = true;

#if !MULTI_THREADED
	ovrScene_CreateVAOs( scene );
#endif
}

static void ovrScene_Destroy( ovrScene * scene )
{
#if !MULTI_THREADED
	ovrScene_DestroyVAOs( scene );
#endif

	ovrProgram_Destroy( &scene->Program );
	ovrGeometry_Destroy( &scene->Cube );
	GL( glDeleteBuffers( 1, &scene->InstanceTransformBuffer ) );
	scene->CreatedScene = false;
}

/*
================================================================================

ovrSimulation

================================================================================
*/

struct ovrSimulation
{
	ovrVector3f			CurrentRotation;
};

static void ovrSimulation_Clear( ovrSimulation * simulation )
{
	simulation->CurrentRotation.x = 0.0f;
	simulation->CurrentRotation.y = 0.0f;
	simulation->CurrentRotation.z = 0.0f;
}

static void ovrSimulation_Advance( ovrSimulation * simulation, double predictedDisplayTime )
{
	// Update rotation.
	simulation->CurrentRotation.x = (float)( predictedDisplayTime );
	simulation->CurrentRotation.y = (float)( predictedDisplayTime );
	simulation->CurrentRotation.z = (float)( predictedDisplayTime );
}

#endif

/*
================================================================================

ovrRenderer

================================================================================
*/

#define NUM_MULTI_SAMPLES	4

struct ovrRenderer
{
	ovrFramebuffer	FrameBuffer[VRAPI_FRAME_LAYER_EYE_MAX];
	ovrMatrix4f		ProjectionMatrix;
	ovrMatrix4f		TexCoordsTanAnglesMatrix;

	// VRWEBGL BEGIN
	GLfloat 					Near;
	GLfloat  					Far;
	static pthread_mutex_t 		NewNearFarMutex;
	static GLfloat 				NewNear;
	static GLfloat 				NewFar;
	// VRWEBGL END
};

pthread_mutex_t		ovrRenderer::NewNearFarMutex;
GLfloat 			ovrRenderer::NewFar;
GLfloat 			ovrRenderer::NewNear;

static void ovrRenderer_Clear( ovrRenderer * renderer )
{
	for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
	{
		ovrFramebuffer_Clear( &renderer->FrameBuffer[eye] );
	}
	renderer->ProjectionMatrix = ovrMatrix4f_CreateIdentity();
	renderer->TexCoordsTanAnglesMatrix = ovrMatrix4f_CreateIdentity();
}

static void ovrRenderer_SetupProjectionMatrix( ovrRenderer * renderer, const ovrJava* java )
{
	renderer->ProjectionMatrix = ovrMatrix4f_CreateProjectionFov(
										vrapi_GetSystemPropertyFloat( java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X ),
										vrapi_GetSystemPropertyFloat( java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y ),
										0.0f, 0.0f, renderer->Near, renderer->Far );
	renderer->TexCoordsTanAnglesMatrix = ovrMatrix4f_TanAngleMatrixFromProjection( &renderer->ProjectionMatrix );
}

static void ovrRenderer_Create( ovrRenderer * renderer, const ovrJava * java, jobject ActivityObject )
{
	// Create the frame buffers.
	for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
	{
		ovrFramebuffer_Create( &renderer->FrameBuffer[eye],
								VRAPI_TEXTURE_FORMAT_8888,
								vrapi_GetSystemPropertyInt( java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH ),
								vrapi_GetSystemPropertyInt( java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT ),
								NUM_MULTI_SAMPLES );
	}

	// Setup the projection matrix.
	renderer->Near = ovrRenderer::NewNear = 0.01f;
	renderer->Far = ovrRenderer::NewFar = 0.0f;
	ovrRenderer_SetupProjectionMatrix(renderer, java);
}

static void ovrRenderer_Destroy( ovrRenderer * renderer )
{
	for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
	{
		ovrFramebuffer_Destroy( &renderer->FrameBuffer[eye] );
	}
	renderer->ProjectionMatrix = ovrMatrix4f_CreateIdentity();
	renderer->TexCoordsTanAnglesMatrix = ovrMatrix4f_CreateIdentity();
}

static ovrFrameParms ovrRenderer_RenderFrame( ovrRenderer * renderer, const ovrJava * java,
											long long frameIndex, int minimumVsyncs, const ovrPerformanceParms * perfParms,
#if USE_SCENE
											const ovrScene * scene,
                                            const ovrSimulation * simulation,
#endif
											const ovrTracking * tracking, ovrMobile * ovr, ovrAppThread* appThread )
{
	// VRWEBGL BEGIN
	bool projectionMatrixNeedsUpdate = false;
	pthread_mutex_lock( &ovrRenderer::NewNearFarMutex );
	if (ovrRenderer::NewNear != renderer->Near || ovrRenderer::NewFar != renderer->Far)
	{
		renderer->Near = ovrRenderer::NewNear;
		renderer->Far = ovrRenderer::NewFar;
		projectionMatrixNeedsUpdate = true;
	}
	pthread_mutex_unlock( &ovrRenderer::NewNearFarMutex );
	if (projectionMatrixNeedsUpdate)
	{
		ovrRenderer_SetupProjectionMatrix(renderer, java);
	}
	// VRWEBGL END

	ovrFrameParms parms = vrapi_DefaultFrameParms( java, VRAPI_FRAME_INIT_DEFAULT, vrapi_GetTimeInSeconds(), NULL );
	parms.FrameIndex = frameIndex;
	parms.MinimumVsyncs = minimumVsyncs;
	parms.PerformanceParms = *perfParms;
	parms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

	// In order to reduce the flickering, when a synchronous command has been executed in the update call, do not render anything.
	// Not perfect, as the render get stalled, but at least it is better than rendering with an undefined state of the opengl context.
	if (!appThread->renderEnabled || VRWebGLCommandProcessor::getInstance()->m_synchronousVRWebGLCommandBeenProcessedInUpdate())
	{
		VRWebGLCommandProcessor::getInstance()->renderFrame(false);
		return parms;
	}

#if USE_SCENE
	// Update the instance transform attributes.
	GL( glBindBuffer( GL_ARRAY_BUFFER, scene->InstanceTransformBuffer ) );
	GL( ovrMatrix4f * cubeTransforms = (ovrMatrix4f *) glMapBufferRange( GL_ARRAY_BUFFER, 0,
				NUM_INSTANCES * sizeof( ovrMatrix4f ), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT ) );
	for ( int i = 0; i < NUM_INSTANCES; i++ )
	{
		const ovrMatrix4f rotation = ovrMatrix4f_CreateRotation(
										scene->CubeRotations[i].x * simulation->CurrentRotation.x,
										scene->CubeRotations[i].y * simulation->CurrentRotation.y,
										scene->CubeRotations[i].z * simulation->CurrentRotation.z );
		const ovrMatrix4f translation = ovrMatrix4f_CreateTranslation(
											scene->CubePositions[i].x,
											scene->CubePositions[i].y,
											scene->CubePositions[i].z );
		const ovrMatrix4f transform = ovrMatrix4f_Multiply( &translation, &rotation );
		cubeTransforms[i] = ovrMatrix4f_Transpose( &transform );
	}
	GL( glUnmapBuffer( GL_ARRAY_BUFFER ) );
	GL( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
#endif

	// Calculate the center view matrix.
	const ovrHeadModelParms headModelParms = vrapi_DefaultHeadModelParms();

	// Render the eye images.
	for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
	{
		// updated sensor prediction for each eye (updates orientation, not position)
#if REDUCED_LATENCY
		ovrTracking updatedTracking = vrapi_GetPredictedTracking( ovr, tracking->HeadPose.TimeInSeconds );
		updatedTracking.HeadPose.Pose.Position = tracking->HeadPose.Pose.Position;
#else
		ovrTracking updatedTracking = *tracking;
#endif

		// Calculate the center view matrix.
		const ovrMatrix4f centerEyeViewMatrix = vrapi_GetCenterEyeViewMatrix( &headModelParms, &updatedTracking, NULL );
		const ovrMatrix4f eyeViewMatrix = vrapi_GetEyeViewMatrix( &headModelParms, &centerEyeViewMatrix, eye );

		ovrFramebuffer * frameBuffer = &renderer->FrameBuffer[eye];
		ovrFramebuffer_SetCurrent( frameBuffer );

		GL( glEnable( GL_SCISSOR_TEST ) );
		GL( glDepthMask( GL_TRUE ) );
		GL( glEnable( GL_DEPTH_TEST ) );
		GL( glDepthFunc( GL_LEQUAL ) );
		GL( glViewport( 0, 0, frameBuffer->Width, frameBuffer->Height ) );
		GL( glScissor( 0, 0, frameBuffer->Width, frameBuffer->Height ) );
		GL( glClearColor( 0.125f, 0.0f, 0.125f, 1.0f ) );
		GL( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );
#if USE_SCENE
		GL( glUseProgram( scene->Program.Program ) );
		GL( glUniformMatrix4fv( scene->Program.Uniforms[UNIFORM_VIEW_MATRIX], 1, GL_TRUE, (const GLfloat *)eyeViewMatrix.M[0] ) );
		GL( glUniformMatrix4fv( scene->Program.Uniforms[UNIFORM_PROJECTION_MATRIX], 1, GL_TRUE, (const GLfloat *)renderer->ProjectionMatrix.M[0] ) );
		GL( glBindVertexArray( scene->Cube.VertexArrayObject ) );
		GL( glDrawElementsInstanced( GL_TRIANGLES, scene->Cube.IndexCount, GL_UNSIGNED_SHORT, NULL, NUM_INSTANCES ) );
		GL( glBindVertexArray( 0 ) );
		GL( glUseProgram( 0 ) );
#endif
        
        
        // VRWEBGL BEGIN

        // Draw just a simple triangle. This code is just a proof of concept to fully understand how and where the rendering should happen
#if RENDER_NATIVE_TRIANGLE
        GL( glUseProgram(scene->programId) );
        GL( glBindBuffer(34962, scene->bufferId) );
        GL( glEnableVertexAttribArray(scene->aVertexPositionId) );
        GL( glVertexAttribPointer(scene->aVertexPositionId, 3, 5126, false, 0, 0) );
        GL( glUniformMatrix4fv(scene->uPMatrixId, 1, true, (const GLfloat *)renderer->ProjectionMatrix.M[0]) );
        GL( glUniformMatrix4fv(scene->uMVMatrixId, 1, true, (const GLfloat *)eyeViewMatrix.M[0]) );
        GL( glDrawArrays(4, 0, 3) );
        GL( glDisableVertexAttribArray(scene->aVertexPositionId) );
        GL( glBindBuffer(34962, 0) );
        GL( glUseProgram(0) );
#endif
      
#ifdef VRWEBGL_SHOW_LOG        
        java->Env->CallVoidMethod(appThread->ActivityObject, appThread->logHeapUsageMethodID);
#endif

        // Transpose oculus projection and view matrices to be opengl compatible.
        VRWebGL_transposeMatrix4((const GLfloat *)renderer->ProjectionMatrix.M[0], appThread->projectionMatrixTransposed);
        VRWebGL_transposeMatrix4((const GLfloat *)eyeViewMatrix.M[0], appThread->viewMatrixTransposed);
 		// Setup the information before rendering current eye's frame
        VRWebGLCommandProcessor::getInstance()->setViewAndProjectionMatrices(appThread->projectionMatrixTransposed, appThread->viewMatrixTransposed);
        VRWebGLCommandProcessor::getInstance()->setFramebuffer(ovrFramebuffer_GetCurrent(frameBuffer));
        VRWebGLCommandProcessor::getInstance()->setViewport(0, 0, frameBuffer->Width, frameBuffer->Height);
        // Render current eye's frame
        VRWebGLCommandProcessor::getInstance()->renderFrame();

        // VRWEBGL END
        
		// Explicitly clear the border texels to black because OpenGL-ES does not support GL_CLAMP_TO_BORDER.
		{
			// Clear to fully opaque black.
			GL( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
			// bottom
			GL( glScissor( 0, 0, frameBuffer->Width, 1 ) );
			GL( glClear( GL_COLOR_BUFFER_BIT ) );
			// top
			GL( glScissor( 0, frameBuffer->Height - 1, frameBuffer->Width, 1 ) );
			GL( glClear( GL_COLOR_BUFFER_BIT ) );
			// left
			GL( glScissor( 0, 0, 1, frameBuffer->Height ) );
			GL( glClear( GL_COLOR_BUFFER_BIT ) );
			// right
			GL( glScissor( frameBuffer->Width - 1, 0, 1, frameBuffer->Height ) );
			GL( glClear( GL_COLOR_BUFFER_BIT ) );
		}

		ovrFramebuffer_Resolve( frameBuffer );

		parms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].ColorTextureSwapChain = frameBuffer->ColorTextureSwapChain;
		parms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].TextureSwapChainIndex = frameBuffer->TextureSwapChainIndex;
		parms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].TexCoordsFromTanAngles = renderer->TexCoordsTanAnglesMatrix;
		parms.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].HeadPose = updatedTracking.HeadPose;

		ovrFramebuffer_Advance( frameBuffer );
	}

	ovrFramebuffer_SetNone();

	return parms;
}

/*
================================================================================

ovrRenderThread

================================================================================
*/

#if MULTI_THREADED

enum ovrRenderType
{
	RENDER_FRAME,
	RENDER_LOADING_ICON,
	RENDER_BLACK_FLUSH,
	RENDER_BLACK_FINAL
};

struct ovrRenderThread
{
	JavaVM *			JavaVm;
	jobject				ActivityObject;
	const ovrEgl *		ShareEgl;
	pthread_t			Thread;
	int					Tid;
	// Synchronization
	bool				Exit;
	bool				WorkAvailableFlag;
	bool				WorkDoneFlag;
	pthread_cond_t		WorkAvailableCondition;
	pthread_cond_t		WorkDoneCondition;
	pthread_mutex_t		Mutex;
	// Latched data for rendering.
	ovrMobile *			Ovr;
	ovrRenderType		RenderType;
	long long			FrameIndex;
	int					MinimumVsyncs;
	ovrPerformanceParms	PerformanceParms;
#if USE_SCENE
	ovrScene *			Scene;
	ovrSimulation		Simulation;
#endif
	ovrTracking			Tracking;
};

void * RenderThreadFunction( void * parm )
{
	ovrRenderThread * renderThread = (ovrRenderThread *)parm;
	renderThread->Tid = gettid();

	ovrJava java;
	java.Vm = renderThread->JavaVm;
	(*java.Vm)->AttachCurrentThread( java.Vm, &java.Env, NULL );
	java.ActivityObject = renderThread->ActivityObject;

	ovrEgl egl;
	ovrEgl_CreateContext( &egl, renderThread->ShareEgl );

	ovrRenderer renderer;
	ovrRenderer_Create( &renderer, &java, renderThread->ActivityObject);

	ovrScene * lastScene = NULL;

	for( ; ; )
	{
		// Signal work completed.
		pthread_mutex_lock( &renderThread->Mutex );
		renderThread->WorkDoneFlag = true;
		pthread_cond_signal( &renderThread->WorkDoneCondition );
		pthread_mutex_unlock( &renderThread->Mutex );

		// Wait for work.
		pthread_mutex_lock( &renderThread->Mutex );
		while ( !renderThread->WorkAvailableFlag )
		{
			pthread_cond_wait( &renderThread->WorkAvailableCondition, &renderThread->Mutex );
		}
		renderThread->WorkAvailableFlag = false;
		pthread_mutex_unlock( &renderThread->Mutex );

		// Check for exit.
		if ( renderThread->Exit )
		{
			break;
		}

		// Make sure the scene has VAOs created for this context.
		if ( renderThread->Scene != NULL && renderThread->Scene != lastScene )
		{
			if ( lastScene != NULL )
			{
				ovrScene_DestroyVAOs( lastScene );
			}
			ovrScene_CreateVAOs( renderThread->Scene );
			lastScene = renderThread->Scene;
		}

		// Render.
		ovrFrameParms frameParms;
		if ( renderThread->RenderType == RENDER_FRAME )
		{
			frameParms = ovrRenderer_RenderFrame( &renderer, &java,
					renderThread->FrameIndex, renderThread->MinimumVsyncs, &renderThread->PerformanceParms,
					renderThread->Scene, &renderThread->Simulation,
					&renderThread->Tracking, renderThread->Ovr );
		}
		else if ( renderThread->RenderType == RENDER_LOADING_ICON )
		{
			frameParms = vrapi_DefaultFrameParms( &java, VRAPI_FRAME_INIT_LOADING_ICON_FLUSH, vrapi_GetTimeInSeconds(), NULL );
			frameParms.FrameIndex = renderThread->FrameIndex;
			frameParms.MinimumVsyncs = renderThread->MinimumVsyncs;
			frameParms.PerformanceParms = renderThread->PerformanceParms;
		}
		else if ( renderThread->RenderType == RENDER_BLACK_FLUSH )
		{
			frameParms = vrapi_DefaultFrameParms( &java, VRAPI_FRAME_INIT_BLACK_FLUSH, vrapi_GetTimeInSeconds(), NULL );
			frameParms.FrameIndex = renderThread->FrameIndex;
			frameParms.MinimumVsyncs = renderThread->MinimumVsyncs;
			frameParms.PerformanceParms = renderThread->PerformanceParms;
		}
		else if ( renderThread->RenderType == RENDER_BLACK_FINAL )
		{
			frameParms = vrapi_DefaultFrameParms( &java, VRAPI_FRAME_INIT_BLACK_FINAL, vrapi_GetTimeInSeconds(), NULL );
			frameParms.FrameIndex = renderThread->FrameIndex;
			frameParms.MinimumVsyncs = renderThread->MinimumVsyncs;
			frameParms.PerformanceParms = renderThread->PerformanceParms;
		}

		// Hand over the eye images to the time warp.
		vrapi_SubmitFrame( renderThread->Ovr, &frameParms );
	}

	if ( lastScene != NULL )
	{
		ovrScene_DestroyVAOs( lastScene );
	}

	ovrRenderer_Destroy( &renderer );
	ovrEgl_DestroyContext( &egl );

	(*java.Vm)->DetachCurrentThread( java.Vm );
}

static void ovrRenderThread_Clear( ovrRenderThread * renderThread )
{
	renderThread->JavaVm = NULL;
	renderThread->ActivityObject = NULL;
	renderThread->ShareEgl = NULL;
	renderThread->Thread = 0;
	renderThread->Tid = 0;
	renderThread->Exit = false;
	renderThread->WorkAvailableFlag = false;
	renderThread->WorkDoneFlag = false;
	renderThread->Ovr = NULL;
	renderThread->RenderType = RENDER_FRAME;
	renderThread->FrameIndex = 1;
	renderThread->MinimumVsyncs = 1;
	renderThread->PerformanceParms = vrapi_DefaultPerformanceParms();
	renderThread->Scene = NULL;
	ovrSimulation_Clear( &renderThread->Simulation );
}

static void ovrRenderThread_Create( ovrRenderThread * renderThread, const ovrJava * java, const ovrEgl * shareEgl )
{
	renderThread->JavaVm = java->Vm;
	renderThread->ActivityObject = java->ActivityObject;
	renderThread->ShareEgl = shareEgl;
	renderThread->Thread = 0;
	renderThread->Tid = 0;
	renderThread->Exit = false;
	renderThread->WorkAvailableFlag = false;
	renderThread->WorkDoneFlag = false;
	pthread_cond_init( &renderThread->WorkAvailableCondition, NULL );
	pthread_cond_init( &renderThread->WorkDoneCondition, NULL );
	pthread_mutex_init( &renderThread->Mutex, NULL );

	const int createErr = pthread_create( &renderThread->Thread, NULL, RenderThreadFunction, renderThread );
	if ( createErr != 0 )
	{
		ALOGE( "pthread_create returned %i", createErr );
	}
}

static void ovrRenderThread_Destroy( ovrRenderThread * renderThread )
{
	pthread_mutex_lock( &renderThread->Mutex );
	renderThread->Exit = true;
	renderThread->WorkAvailableFlag = true;
	pthread_cond_signal( &renderThread->WorkAvailableCondition );
	pthread_mutex_unlock( &renderThread->Mutex );

	pthread_join( renderThread->Thread, NULL );
	pthread_cond_destroy( &renderThread->WorkAvailableCondition );
	pthread_cond_destroy( &renderThread->WorkDoneCondition );
	pthread_mutex_destroy( &renderThread->Mutex );
}

static void ovrRenderThread_Submit( ovrRenderThread * renderThread, ovrMobile * ovr,
		ovrRenderType type, long long frameIndex, int minimumVsyncs, const ovrPerformanceParms * perfParms,
		ovrScene * scene, const ovrSimulation * simulation, const ovrTracking * tracking )
{
	// Wait for the renderer thread to finish the last frame.
	pthread_mutex_lock( &renderThread->Mutex );
	while ( !renderThread->WorkDoneFlag )
	{
		pthread_cond_wait( &renderThread->WorkDoneCondition, &renderThread->Mutex );
	}
	renderThread->WorkDoneFlag = false;
	// Latch the render data.
	renderThread->Ovr = ovr;
	renderThread->RenderType = type;
	renderThread->FrameIndex = frameIndex;
	renderThread->MinimumVsyncs = minimumVsyncs;
	renderThread->PerformanceParms = *perfParms;
	renderThread->Scene = scene;
	if ( simulation != NULL )
	{
		renderThread->Simulation = *simulation;
	}
	if ( tracking != NULL )
	{
		renderThread->Tracking = *tracking;
	}
	// Signal work is available.
	renderThread->WorkAvailableFlag = true;
	pthread_cond_signal( &renderThread->WorkAvailableCondition );
	pthread_mutex_unlock( &renderThread->Mutex );
}

static void ovrRenderThread_Wait( ovrRenderThread * renderThread )
{
	// Wait for the renderer thread to finish the last frame.
	pthread_mutex_lock( &renderThread->Mutex );
	while ( !renderThread->WorkDoneFlag )
	{
		pthread_cond_wait( &renderThread->WorkDoneCondition, &renderThread->Mutex );
	}
	pthread_mutex_unlock( &renderThread->Mutex );
}

static int ovrRenderThread_GetTid( ovrRenderThread * renderThread )
{
	ovrRenderThread_Wait( renderThread );
	return renderThread->Tid;
}

#endif // MULTI_THREADED

/*
================================================================================

ovrApp

================================================================================
*/

enum ovrBackButtonState
{
	BACK_BUTTON_STATE_NONE,
	BACK_BUTTON_STATE_PENDING_DOUBLE_TAP,
	BACK_BUTTON_STATE_PENDING_SHORT_PRESS,
	BACK_BUTTON_STATE_SKIP_UP
};

struct ovrApp
{
	ovrJava				Java;
	ovrEgl				Egl;
	ANativeWindow *		NativeWindow;
	bool				Resumed;
	ovrMobile *			Ovr;
#if USE_SCENE
	ovrScene			Scene;
	ovrSimulation		Simulation;
#else
    bool                Initialized;
#endif
	long long			FrameIndex;
	int					MinimumVsyncs;
	ovrBackButtonState	BackButtonState;
	bool				BackButtonDown;
	double				BackButtonDownStartTime;
#if MULTI_THREADED
	ovrRenderThread		RenderThread;
#else
	ovrRenderer			Renderer;
#endif
	bool				WasMounted;

	// VRWEBGL BEGIN
	static pthread_mutex_t		HeadTrackingInfoMutex;
	static ovrRigidBodyPosef 	Pose;
	static VRWebGLEyeParameters EyeParameters;
	// It would be ideas to have a structure that holds all these variables instead of having 3 deques {VideoTexture, Ready, Ended}
    std::deque<SurfaceTexture*> VideoTextures;
    std::deque<bool> 			VideosReady;
    std::deque<bool> 			VideosEnded;
	// VRWEBGL END
};

pthread_mutex_t		ovrApp::HeadTrackingInfoMutex;
ovrRigidBodyPosef 	ovrApp::Pose;
VRWebGLEyeParameters ovrApp::EyeParameters;

static void ovrApp_Clear( ovrApp * app )
{
	app->Java.Vm = NULL;
	app->Java.Env = NULL;
	app->Java.ActivityObject = NULL;
	app->NativeWindow = NULL;
	app->Resumed = false;
	app->Ovr = NULL;
	app->FrameIndex = 1;
	app->MinimumVsyncs = 1;
	app->BackButtonState = BACK_BUTTON_STATE_NONE;
	app->BackButtonDown = false;
	app->BackButtonDownStartTime = 0.0;
	app->WasMounted = false;

	ovrEgl_Clear( &app->Egl );
#if USE_SCENE
	ovrScene_Clear( &app->Scene );
	ovrSimulation_Clear( &app->Simulation );
#else
    app->Initialized = false;
#endif
#if MULTI_THREADED
	ovrRenderThread_Clear( &app->RenderThread );
#else
	ovrRenderer_Clear( &app->Renderer );
#endif
}

static void ovrApp_PushBlackFinal( ovrApp * app, const ovrPerformanceParms * perfParms )
{
#if MULTI_THREADED
	ovrRenderThread_Submit( &app->RenderThread, app->Ovr,
			RENDER_BLACK_FINAL, app->FrameIndex, app->MinimumVsyncs, perfParms,
			NULL, NULL, NULL );
#else
	ovrFrameParms frameParms = vrapi_DefaultFrameParms( &app->Java, VRAPI_FRAME_INIT_BLACK_FINAL, vrapi_GetTimeInSeconds(), NULL );
	frameParms.FrameIndex = app->FrameIndex;
	frameParms.PerformanceParms = *perfParms;
	vrapi_SubmitFrame( app->Ovr, &frameParms );
#endif
}

static void ovrApp_HandleVrModeChanges( ovrApp * app )
{
	if ( app->NativeWindow != NULL && app->Egl.MainSurface == EGL_NO_SURFACE )
	{
		ovrEgl_CreateSurface( &app->Egl, app->NativeWindow );
	}

	if ( app->Resumed != false && app->NativeWindow != NULL )
	{
		if ( app->Ovr == NULL )
		{
			ovrModeParms parms = vrapi_DefaultModeParms( &app->Java );
			parms.Flags |= VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;	// Must reset the FLAG_FULLSCREEN window flag when using a SurfaceView

#if EXPLICIT_GL_OBJECTS == 1
			parms.Display = (size_t)app->Egl.Display;
			parms.WindowSurface = (size_t)app->Egl.MainSurface;
			parms.ShareContext = (size_t)app->Egl.Context;
#else
			ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
#endif

			app->Ovr = vrapi_EnterVrMode( &parms );

			ALOGV( "        vrapi_EnterVrMode()" );

#if EXPLICIT_GL_OBJECTS == 0
			ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
#endif
		}
	}
	else
	{
		if ( app->Ovr != NULL )
		{
#if MULTI_THREADED
			// Make sure the renderer thread is no longer using the ovrMobile.
			ovrRenderThread_Wait( &app->RenderThread );
#endif
			ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );

			vrapi_LeaveVrMode( app->Ovr );
			app->Ovr = NULL;

			ALOGV( "        vrapi_LeaveVrMode()" );
			ALOGV( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
		}
	}

	if ( app->NativeWindow == NULL && app->Egl.MainSurface != EGL_NO_SURFACE )
	{
		ovrEgl_DestroySurface( &app->Egl );
	}
}

static void ovrApp_BackButtonAction( ovrApp * app, const ovrPerformanceParms * perfParms )
{
	if ( app->BackButtonState == BACK_BUTTON_STATE_PENDING_DOUBLE_TAP )
	{
		ALOGV( "back button double tap" );
		app->BackButtonState = BACK_BUTTON_STATE_SKIP_UP;
	}
	else if ( app->BackButtonState == BACK_BUTTON_STATE_PENDING_SHORT_PRESS && !app->BackButtonDown )
	{
		if ( ( vrapi_GetTimeInSeconds() - app->BackButtonDownStartTime ) > BUTTON_DOUBLE_TAP_TIME_IN_SECONDS )
		{
			ALOGV( "back button short press" );
			ALOGV( "        ovrApp_PushBlackFinal()" );
			ovrApp_PushBlackFinal( app, perfParms );
			ALOGV( "        SystemActivities_StartSystemActivity( %s )", PUI_CONFIRM_QUIT );
			SystemActivities_StartSystemActivity( &app->Java, PUI_CONFIRM_QUIT, NULL );
			app->BackButtonState = BACK_BUTTON_STATE_NONE;
		}
	}
	else if ( app->BackButtonState == BACK_BUTTON_STATE_NONE && app->BackButtonDown )
	{
		if ( ( vrapi_GetTimeInSeconds() - app->BackButtonDownStartTime ) > BACK_BUTTON_LONG_PRESS_TIME_IN_SECONDS )
		{
			ALOGV( "back button long press" );
			ALOGV( "        ovrApp_PushBlackFinal()" );
			ovrApp_PushBlackFinal( app, perfParms );
			ALOGV( "        SystemActivities_StartSystemActivity( %s )", PUI_GLOBAL_MENU );
			SystemActivities_StartSystemActivity( &app->Java, PUI_GLOBAL_MENU, NULL );
			app->BackButtonState = BACK_BUTTON_STATE_SKIP_UP;
		}
	}
}

static int ovrApp_HandleKeyEvent( ovrApp * app, const int keyCode, const int action )
{
	// Handle GearVR back button.
	if ( keyCode == AKEYCODE_BACK )
	{
		if ( action == AKEY_EVENT_ACTION_DOWN )
		{
			if ( !app->BackButtonDown )
			{
				if ( ( vrapi_GetTimeInSeconds() - app->BackButtonDownStartTime ) < BUTTON_DOUBLE_TAP_TIME_IN_SECONDS )
				{
					app->BackButtonState = BACK_BUTTON_STATE_PENDING_DOUBLE_TAP;
				}
				app->BackButtonDownStartTime = vrapi_GetTimeInSeconds();
			}
			app->BackButtonDown = true;
		}
		else if ( action == AKEY_EVENT_ACTION_UP )
		{
			if ( app->BackButtonState == BACK_BUTTON_STATE_NONE )
			{
				if ( ( vrapi_GetTimeInSeconds() - app->BackButtonDownStartTime ) < BUTTON_SHORT_PRESS_TIME_IN_SECONDS )
				{
					app->BackButtonState = BACK_BUTTON_STATE_PENDING_SHORT_PRESS;
				}
			}
			else if ( app->BackButtonState == BACK_BUTTON_STATE_SKIP_UP )
			{
				app->BackButtonState = BACK_BUTTON_STATE_NONE;
			}
			app->BackButtonDown = false;
		}
		return 1;
	}
	return 0;
}

static int ovrApp_HandleTouchEvent( ovrApp * app, const int action, const float x, const float y )
{
	// Handle GearVR touch pad.
	if ( app->Ovr != NULL && action == AMOTION_EVENT_ACTION_UP )
	{
#if 0
		// Cycle through 60Hz, 30Hz, 20Hz and 15Hz synthesis.
		app->MinimumVsyncs++;
		if ( app->MinimumVsyncs > 4 )
		{
			app->MinimumVsyncs = 1;
		}
		ALOGV( "        MinimumVsyncs = %d", app->MinimumVsyncs );
#endif
	}
	return 1;
}

static void ovrApp_HandleSystemEvents( ovrApp * app )
{
	SystemActivitiesAppEventList_t appEvents;
	SystemActivities_Update( app->Ovr, &app->Java, &appEvents );

	// if you want to reorient on mount, check mount state here
	const bool isMounted = ( vrapi_GetSystemStatusInt( &app->Java, VRAPI_SYS_STATUS_MOUNTED ) != VRAPI_FALSE );
	if ( isMounted && !app->WasMounted )
	{
		// We just mounted so push a reorient event to be handled at the app level (if desired)
		char reorientMessage[1024];
		SystemActivities_CreateSystemActivitiesCommand( "", SYSTEM_ACTIVITY_EVENT_REORIENT, "", "", reorientMessage, sizeof( reorientMessage ) );
		SystemActivities_AppendAppEvent( &appEvents, reorientMessage );
	}
	app->WasMounted = isMounted;

	// TODO: any app specific events should be handled right here by looping over appEvents list

	SystemActivities_PostUpdate( app->Ovr, &app->Java, &appEvents );
}

static int ovrApp_GetVideoTextureIndexByTextureId(ovrApp* app, GLuint textureId)
{
	int index = -1;
	for (int i = 0; index == -1 && i < app->VideoTextures.size(); i++) 
	{
		if (app->VideoTextures[i]->GetTextureId() == textureId)
		{
			index = i;
		}
	}
	return index;
}

/*
================================================================================

ovrMessageQueue

================================================================================
*/

static void ovrMessageQueue_Create( ovrMessageQueue * messageQueue )
{
	messageQueue->Head = 0;
	messageQueue->Tail = 0;
	messageQueue->Wait = MQ_WAIT_NONE;
	messageQueue->EnabledFlag = false;
	messageQueue->PostedFlag = false;
	messageQueue->ReceivedFlag = false;
	messageQueue->ProcessedFlag = false;

	pthread_mutexattr_t	attr;
	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
	pthread_mutex_init( &messageQueue->Mutex, &attr );
	pthread_mutexattr_destroy( &attr );
	pthread_cond_init( &messageQueue->PostedCondition, NULL );
	pthread_cond_init( &messageQueue->ReceivedCondition, NULL );
	pthread_cond_init( &messageQueue->ProcessedCondition, NULL );
}

static void ovrMessageQueue_Destroy( ovrMessageQueue * messageQueue )
{
	pthread_mutex_destroy( &messageQueue->Mutex );
	pthread_cond_destroy( &messageQueue->PostedCondition );
	pthread_cond_destroy( &messageQueue->ReceivedCondition );
	pthread_cond_destroy( &messageQueue->ProcessedCondition );
}

static void ovrMessageQueue_Enable( ovrMessageQueue * messageQueue, const bool set )
{
	messageQueue->EnabledFlag = set;
}

static void ovrMessageQueue_PostMessage( ovrMessageQueue * messageQueue, const ovrMessage * message )
{
	if ( !messageQueue->EnabledFlag )
	{
		return;
	}
	while ( messageQueue->Tail - messageQueue->Head >= MAX_MESSAGES )
	{
		usleep( 1000 );
	}
	pthread_mutex_lock( &messageQueue->Mutex );
	messageQueue->Messages[messageQueue->Tail & ( MAX_MESSAGES - 1 )] = *message;
	messageQueue->Tail++;
	messageQueue->PostedFlag = true;
	pthread_cond_broadcast( &messageQueue->PostedCondition );
	if ( message->Wait == MQ_WAIT_RECEIVED )
	{
		while ( !messageQueue->ReceivedFlag )
		{
			pthread_cond_wait( &messageQueue->ReceivedCondition, &messageQueue->Mutex );
		}
		messageQueue->ReceivedFlag = false;
	}
	else if ( message->Wait == MQ_WAIT_PROCESSED )
	{
		while ( !messageQueue->ProcessedFlag )
		{
			pthread_cond_wait( &messageQueue->ProcessedCondition, &messageQueue->Mutex );
		}
		messageQueue->ProcessedFlag = false;
	}
	pthread_mutex_unlock( &messageQueue->Mutex );
}

static void ovrMessageQueue_SleepUntilMessage( ovrMessageQueue * messageQueue )
{
	if ( messageQueue->Wait == MQ_WAIT_PROCESSED )
	{
		messageQueue->ProcessedFlag = true;
		pthread_cond_broadcast( &messageQueue->ProcessedCondition );
		messageQueue->Wait = MQ_WAIT_NONE;
	}
	pthread_mutex_lock( &messageQueue->Mutex );
	if ( messageQueue->Tail > messageQueue->Head )
	{
		pthread_mutex_unlock( &messageQueue->Mutex );
		return;
	}
	while ( !messageQueue->PostedFlag )
	{
		pthread_cond_wait( &messageQueue->PostedCondition, &messageQueue->Mutex );
	}
	messageQueue->PostedFlag = false;
	pthread_mutex_unlock( &messageQueue->Mutex );
}

static bool ovrMessageQueue_GetNextMessage( ovrMessageQueue * messageQueue, ovrMessage * message, bool waitForMessages )
{
	if ( messageQueue->Wait == MQ_WAIT_PROCESSED )
	{
		messageQueue->ProcessedFlag = true;
		pthread_cond_broadcast( &messageQueue->ProcessedCondition );
		messageQueue->Wait = MQ_WAIT_NONE;
	}
	if ( waitForMessages )
	{
		ovrMessageQueue_SleepUntilMessage( messageQueue );
	}
	pthread_mutex_lock( &messageQueue->Mutex );
	if ( messageQueue->Tail <= messageQueue->Head )
	{
		pthread_mutex_unlock( &messageQueue->Mutex );
		return false;
	}
	*message = messageQueue->Messages[messageQueue->Head & ( MAX_MESSAGES - 1 )];
	messageQueue->Head++;
	pthread_mutex_unlock( &messageQueue->Mutex );
	if ( message->Wait == MQ_WAIT_RECEIVED )
	{
		messageQueue->ReceivedFlag = true;
		pthread_cond_broadcast( &messageQueue->ReceivedCondition );
	}
	else if ( message->Wait == MQ_WAIT_PROCESSED )
	{
		messageQueue->Wait = MQ_WAIT_PROCESSED;
	}
	return true;
}

/*
================================================================================

ovrAppThread

================================================================================
*/

enum
{
	MESSAGE_ON_CREATE,
	MESSAGE_ON_START,
	MESSAGE_ON_RESUME,
	MESSAGE_ON_PAUSE,
	MESSAGE_ON_STOP,
	MESSAGE_ON_DESTROY,
	MESSAGE_ON_SURFACE_CREATED,
	MESSAGE_ON_SURFACE_DESTROYED,
	MESSAGE_ON_KEY_EVENT,
	MESSAGE_ON_TOUCH_EVENT,
	MESSAGE_ON_NEW_VIDEO,
	MESSAGE_ON_DELETE_VIDEO,
	MESSAGE_ON_PLAY_VIDEO,
	MESSAGE_ON_PAUSE_VIDEO,
	MESSAGE_ON_SET_VIDEO_SRC,
	MESSAGE_ON_SET_VIDEO_VOLUME,
	MESSAGE_ON_SET_VIDEO_LOOP,
	MESSAGE_ON_SET_VIDEO_CURRENT_TIME,
	MESSAGE_ON_GET_VIDEO_CURRENT_TIME,
	MESSAGE_ON_GET_VIDEO_DURATION,
	MESSAGE_ON_GET_VIDEO_WIDTH,
	MESSAGE_ON_GET_VIDEO_HEIGHT,
	MESSAGE_ON_VIDEO_PREPARED,
	MESSAGE_ON_CHECK_VIDEO_PREPARED,
	MESSAGE_ON_VIDEO_ENDED,
	MESSAGE_ON_CHECK_VIDEO_ENDED,
	MESSAGE_ON_SET_RENDER_ENABLED
};

void * AppThreadFunction( void * parm )
{
	VRWebGLCommandProcessor::getInstance()->setCurrentThreadName("GL");

	ovrAppThread * appThread = (ovrAppThread *)parm;

	ovrJava java;
	java.Vm = appThread->JavaVm;
	java.Vm->AttachCurrentThread( &java.Env, NULL );
	java.ActivityObject = appThread->ActivityObject;

	SystemActivities_Init( &java );

	const ovrInitParms initParms = vrapi_DefaultInitParms( &java );
	int32_t initResult = vrapi_Initialize( &initParms );
	if ( initResult != VRAPI_INITIALIZE_SUCCESS )
	{
		char const * msg = initResult == VRAPI_INITIALIZE_PERMISSIONS_ERROR ? 
										"Thread priority security exception. Make sure the APK is signed." :
										"VrApi initialization error.";
		SystemActivities_DisplayError( &java, SYSTEM_ACTIVITIES_FATAL_ERROR_OSIG, __FILE__, msg );
	}

	ovrApp appState;
	ovrApp_Clear( &appState );
	appState.Java = java;
	
	ovrEgl_CreateContext( &appState.Egl, NULL );

	ovrPerformanceParms perfParms = vrapi_DefaultPerformanceParms();
	perfParms.CpuLevel = CPU_LEVEL;
	perfParms.GpuLevel = GPU_LEVEL;
	perfParms.MainThreadTid = gettid();

    // VRWEBGL BEGIN
    pthread_mutexattr_t	attr;
    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
    pthread_mutex_init( &ovrApp::HeadTrackingInfoMutex, &attr );
    pthread_mutex_init( &ovrRenderer::NewNearFarMutex, &attr );
    pthread_mutexattr_destroy( &attr );
    // VRWEBGL END    


#if MULTI_THREADED
	ovrRenderThread_Create( &appState.RenderThread, &appState.Java, &appState.Egl );
	// Also set the renderer thread to SCHED_FIFO.
	perfParms.RenderThreadTid = ovrRenderThread_GetTid( &appState.RenderThread );
#else
	ovrRenderer_Create( &appState.Renderer, &java, appThread->ActivityObject );
#endif

	for ( bool destroyed = false; destroyed == false; )
	{
		for ( ; ; )
		{
			ovrMessage message;
			const bool waitForMessages = ( appState.Ovr == NULL && destroyed == false );
			if ( !ovrMessageQueue_GetNextMessage( &appThread->MessageQueue, &message, waitForMessages ) )
			{
				break;
			}

			switch ( message.Id )
			{
				case MESSAGE_ON_CREATE:				{ break; }
				case MESSAGE_ON_START:				{ break; }
				case MESSAGE_ON_RESUME:				{ appState.Resumed = true; break; }
				case MESSAGE_ON_PAUSE:				{ appState.Resumed = false; break; }
				case MESSAGE_ON_STOP:				{ break; }
				case MESSAGE_ON_DESTROY:			{ appState.NativeWindow = NULL; destroyed = true; break; }
				case MESSAGE_ON_SURFACE_CREATED:	{ appState.NativeWindow = (ANativeWindow *)ovrMessage_GetPointerParm( &message, 0 ); break; }
				case MESSAGE_ON_SURFACE_DESTROYED:	{ appState.NativeWindow = NULL; break; }
				case MESSAGE_ON_KEY_EVENT:			{ ovrApp_HandleKeyEvent( &appState,
															ovrMessage_GetIntegerParm( &message, 0 ),
															ovrMessage_GetIntegerParm( &message, 1 ) ); break; }
				case MESSAGE_ON_TOUCH_EVENT:		{ ovrApp_HandleTouchEvent( &appState,
															ovrMessage_GetIntegerParm( &message, 0 ),
															ovrMessage_GetFloatParm( &message, 1 ),
															ovrMessage_GetFloatParm( &message, 2 ) ); break; }
				case MESSAGE_ON_NEW_VIDEO:
				{
					SurfaceTexture* videoTexture = new SurfaceTexture( appState.Java.Env );
					int textureId = videoTexture->GetTextureId();
					*((GLuint*)(message.Result)) = textureId;
					appState.Java.Env->CallVoidMethod( appThread->ActivityObject, appThread->newVideoMethodId, videoTexture->GetJavaObject(), textureId );
					appState.VideoTextures.push_back(videoTexture);
					appState.VideosReady.push_back(false);
					appState.VideosEnded.push_back(false);
					break;
				}
				case MESSAGE_ON_DELETE_VIDEO:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_DELETE_VIDEO", textureId);
					}
					else 
					{
						SurfaceTexture* videoTexture = appState.VideoTextures[i];
						appState.Java.Env->CallVoidMethod( appThread->ActivityObject, appThread->deleteVideoMethodId, videoTexture->GetJavaObject() );
						delete videoTexture;
						appState.VideoTextures.erase(appState.VideoTextures.begin() + i);
						appState.VideosReady.erase(appState.VideosReady.begin() + i);
						appState.VideosEnded.erase(appState.VideosEnded.begin() + i);
					}
					break;
				}
				case MESSAGE_ON_PLAY_VIDEO:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					float volume = ovrMessage_GetFloatParm(&message, 1);
					int loop = ovrMessage_GetIntegerParm(&message, 2);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_PLAY_VIDEO", textureId);
					}
					else 
					{
						appState.Java.Env->CallVoidMethod( appThread->ActivityObject, appThread->playVideoOnUIThreadMethodId, appState.VideoTextures[i]->GetJavaObject(), volume, loop != 0);
					}
					break;
				}
				case MESSAGE_ON_PAUSE_VIDEO:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_PAUSE_VIDEO", textureId);
					}
					else 
					{
						appState.Java.Env->CallVoidMethod( appThread->ActivityObject, appThread->pauseVideoMethodId, appState.VideoTextures[i]->GetJavaObject());
					}
					break;
				}
				case MESSAGE_ON_SET_VIDEO_SRC:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					std::string* srcPtr = (std::string*)ovrMessage_GetPointerParm(&message, 1);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_SET_VIDEO_SRC", textureId);
					}
					else 
					{
						ALOGV("VRWebGL: MESSAGE_ON_SET_VIDEO_SRC: %s", srcPtr->c_str());
						jstring jstr = appState.Java.Env->NewStringUTF( srcPtr->c_str() );
						appState.Java.Env->CallVoidMethod( appThread->ActivityObject, appThread->setVideoSrcOnUIThreadMethodId, appState.VideoTextures[i]->GetJavaObject(), jstr );
						appState.Java.Env->DeleteLocalRef( jstr );
					}
					delete srcPtr;
					srcPtr = 0;
					break;
				}
				case MESSAGE_ON_SET_VIDEO_VOLUME:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					float volume = ovrMessage_GetFloatParm(&message, 1);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_SET_VIDEO_VOLUME", textureId);
					}
					else 
					{
						appState.Java.Env->CallVoidMethod( appThread->ActivityObject, appThread->setVideoVolumeMethodId, appState.VideoTextures[i]->GetJavaObject(), volume);
					}
					break;
				}
				case MESSAGE_ON_SET_VIDEO_LOOP:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int loop = ovrMessage_GetIntegerParm(&message, 1);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_SET_VIDEO_LOOP", textureId);
					}
					else 
					{
						appState.Java.Env->CallVoidMethod( appThread->ActivityObject, appThread->setVideoLoopMethodId, appState.VideoTextures[i]->GetJavaObject(), loop != 0);
					}
					break;
				}
				case MESSAGE_ON_SET_VIDEO_CURRENT_TIME:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int currentTime = ovrMessage_GetIntegerParm(&message, 1);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_SET_VIDEO_CURRENT_TIME", textureId);
					}
					else 
					{
						appState.Java.Env->CallVoidMethod( appThread->ActivityObject, appThread->seekToMethodId, appState.VideoTextures[i]->GetJavaObject(), currentTime);
					}
					break;
				}
				case MESSAGE_ON_GET_VIDEO_CURRENT_TIME:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_GET_VIDEO_CURRENT_TIME", textureId);
					}
					else 
					{
						*((int*)(message.Result)) = appState.Java.Env->CallIntMethod( appThread->ActivityObject, appThread->getVideoCurrentTimeMethodId, appState.VideoTextures[i]->GetJavaObject());
					}
					break;
				}
				case MESSAGE_ON_GET_VIDEO_DURATION:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_GET_VIDEO_DURATION", textureId);
					}
					else 
					{
						*((int*)(message.Result)) = appState.Java.Env->CallIntMethod( appThread->ActivityObject, appThread->getVideoDurationMethodId, appState.VideoTextures[i]->GetJavaObject());
					}
					break;
				}
				case MESSAGE_ON_GET_VIDEO_WIDTH:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_GET_VIDEO_WIDTH", textureId);
					}
					else 
					{
						*((int*)(message.Result)) = appState.Java.Env->CallIntMethod( appThread->ActivityObject, appThread->getVideoWidthMethodId, appState.VideoTextures[i]->GetJavaObject());
					}
					break;
				}
				case MESSAGE_ON_GET_VIDEO_HEIGHT:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_GET_VIDEO_HEIGHT", textureId);
					}
					else 
					{
						*((int*)(message.Result)) = appState.Java.Env->CallIntMethod( appThread->ActivityObject, appThread->getVideoHeightMethodId, appState.VideoTextures[i]->GetJavaObject());
					}
					break;
				}
				case MESSAGE_ON_VIDEO_PREPARED:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_VIDEO_PREPARED", textureId);
					}
					else 
					{
						appState.VideosReady[i] = true;
					}
					break;
				}
				case MESSAGE_ON_CHECK_VIDEO_PREPARED:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_CHECK_VIDEO_PREPARED", textureId);
					}
					else 
					{
						*((bool*)(message.Result)) = appState.VideosReady[i];
					}
					break;
				}
				case MESSAGE_ON_VIDEO_ENDED:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_VIDEO_ENDED", textureId);
					}
					else 
					{
						appState.VideosEnded[i] = true;
					}
					break;
				}
				case MESSAGE_ON_CHECK_VIDEO_ENDED:
				{
					GLuint textureId = ovrMessage_GetGLuintParm(&message, 0);
					int i = ovrApp_GetVideoTextureIndexByTextureId(&appState, textureId);
					if (i < 0)
					{
						ALOGE("VRWebGL: ERROR, could not find the video texture corresponding to the texture id '%d' in MESSAGE_ON_CHECK_VIDEO_ENDED", textureId);
					}
					else 
					{
						*((bool*)(message.Result)) = appState.VideosEnded[i];
					}
					break;
				}
				case MESSAGE_ON_SET_RENDER_ENABLED:
				{
					int flag = ovrMessage_GetIntegerParm(&message, 0);
					appThread->renderEnabled = flag != 0;
					break;
				}
			}

			ovrApp_HandleVrModeChanges( &appState );
		}

		ovrApp_BackButtonAction( &appState, &perfParms );
		ovrApp_HandleSystemEvents( &appState );

		if ( appState.Ovr == NULL )
		{
			continue;
		}
        
		// Create the scene if not yet created.
		// The scene is created here to be able to show a loading icon.
#if USE_SCENE
		if ( !ovrScene_IsCreated( &appState.Scene ) )
#else 
        if ( !appState.Initialized )
#endif
		{
#if MULTI_THREADED
			// Show a loading icon.
			ovrRenderThread_Submit( &appState.RenderThread, appState.Ovr,
					RENDER_LOADING_ICON, appState.FrameIndex, appState.MinimumVsyncs, &perfParms,
					NULL, NULL, NULL );
#else
			// Show a loading icon.
			ovrFrameParms frameParms = vrapi_DefaultFrameParms( &appState.Java, VRAPI_FRAME_INIT_LOADING_ICON_FLUSH, vrapi_GetTimeInSeconds(), NULL );
			frameParms.FrameIndex = appState.FrameIndex;
			frameParms.PerformanceParms = perfParms;
			vrapi_SubmitFrame( appState.Ovr, &frameParms );
#endif

			// VRWebGL BEGIN
			const ovrHeadModelParms headModelParms = vrapi_DefaultHeadModelParms();
		    pthread_mutex_lock( &ovrApp::HeadTrackingInfoMutex );
		    ovrApp::EyeParameters.xFOV = vrapi_GetSystemPropertyFloat(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X);
		    ovrApp::EyeParameters.yFOV = vrapi_GetSystemPropertyFloat(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y);
		    ovrApp::EyeParameters.width = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
		    ovrApp::EyeParameters.height = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);
		    ovrApp::EyeParameters.interpupillaryDistance = headModelParms.InterpupillaryDistance;
		    pthread_mutex_unlock( &ovrApp::HeadTrackingInfoMutex );	    
		    // VRWebGL END


#if USE_SCENE
			// Create the scene.
			ovrScene_Create( &appState.Scene );
#else
            appState.Initialized = true;
#endif
		}

		// This is the only place the frame index is incremented, right before
		// calling vrapi_GetPredictedDisplayTime().
		appState.FrameIndex++;

		// Get the HMD pose, predicted for the middle of the time period during which
		// the new eye images will be displayed. The number of frames predicted ahead
		// depends on the pipeline depth of the engine and the synthesis rate.
		// The better the prediction, the less black will be pulled in at the edges.
		const double predictedDisplayTime = vrapi_GetPredictedDisplayTime( appState.Ovr, appState.FrameIndex );
		const ovrTracking baseTracking = vrapi_GetPredictedTracking( appState.Ovr, predictedDisplayTime );

		// Apply the head-on-a-stick model if there is no positional tracking.
		const ovrHeadModelParms headModelParms = vrapi_DefaultHeadModelParms();
		const ovrTracking tracking = vrapi_ApplyHeadModel( &headModelParms, &baseTracking );

        // VRWEBGL BEGIN
        // Update all the video textures...
	    for (unsigned int i = 0; i < appState.VideoTextures.size(); i++)
	    {
	    	appState.VideoTextures[i]->Update();
	    }

        VRWebGLCommandProcessor::getInstance()->update();
        // ALOGV("VRWebGL: %s: update", VRWebGLCommandProcessor::threadNames[pthread_self()].c_str());

	    pthread_mutex_lock( &ovrApp::HeadTrackingInfoMutex );
	    ovrApp::Pose = tracking.HeadPose;
	    pthread_mutex_unlock( &ovrApp::HeadTrackingInfoMutex );	    
        // VRWEBGL END
        
#if USE_SCENE
		// Advance the simulation based on the predicted display time.
		ovrSimulation_Advance( &appState.Simulation, predictedDisplayTime );
#endif

#if MULTI_THREADED
		// Render the eye images on a separate thread.
		ovrRenderThread_Submit( &appState.RenderThread, appState.Ovr,
				RENDER_FRAME, appState.FrameIndex, appState.MinimumVsyncs, &perfParms,
#if USE_SCENE
				&appState.Scene, &appState.Simulation,
#endif
                &tracking );
#else
		// Render eye images and setup ovrFrameParms using ovrTracking.
		const ovrFrameParms frameParms = ovrRenderer_RenderFrame( &appState.Renderer, &appState.Java,
				appState.FrameIndex, appState.MinimumVsyncs, &perfParms,
#if USE_SCENE
                &appState.Scene, &appState.Simulation,
#endif
                &tracking, appState.Ovr, appThread );

		// Hand over the eye images to the time warp.
		vrapi_SubmitFrame( appState.Ovr, &frameParms );
#endif
	}

#if MULTI_THREADED
	ovrRenderThread_Destroy( &appState.RenderThread );
#else
	ovrRenderer_Destroy( &appState.Renderer );
#endif

#if USE_SCENE
	ovrScene_Destroy( &appState.Scene );
#endif

    // VRWEBGL BEGIN
    for (unsigned int i = 0; i < appState.VideoTextures.size(); i++)
    {
    	delete appState.VideoTextures[i];
    }
    appState.VideoTextures.clear();

    pthread_mutex_destroy( &ovrApp::HeadTrackingInfoMutex );
    pthread_mutex_destroy( &ovrRenderer::NewNearFarMutex );
    // VRWEBGL END

	ovrEgl_DestroyContext( &appState.Egl );

	vrapi_Shutdown();

	SystemActivities_Shutdown( &java );

	java.Vm->DetachCurrentThread( );

	return NULL;
}

static void ovrAppThread_Create( ovrAppThread * appThread, JNIEnv * env, jobject activityObject )
{
	env->GetJavaVM( &appThread->JavaVm );
	appThread->ActivityObject = env->NewGlobalRef( activityObject );
	appThread->Thread = 0;
	appThread->NativeWindow = NULL;
	ovrMessageQueue_Create( &appThread->MessageQueue );

    // VRWEBGL BEGIN
    // Get the references to the java methods that might be called from the native side
    appThread->renderEnabled = true;
    appThread->activityObjectJClass = env->GetObjectClass(appThread->ActivityObject);
    appThread->logHeapUsageMethodID = env->GetMethodID(appThread->activityObjectJClass, "logHeapUsageFromNative", "()V");    
    appThread->seekToMethodId = env->GetMethodID(appThread->activityObjectJClass, "seekTo", "(Landroid/graphics/SurfaceTexture;I)V");
    appThread->pauseVideoMethodId = env->GetMethodID(appThread->activityObjectJClass, "pauseVideo", "(Landroid/graphics/SurfaceTexture;)V");
    appThread->stopVideoMethodId = env->GetMethodID(appThread->activityObjectJClass, "stopVideo", "(Landroid/graphics/SurfaceTexture;)V");
    appThread->playVideoOnUIThreadMethodId = env->GetMethodID(appThread->activityObjectJClass, "playVideoOnUIThread", "(Landroid/graphics/SurfaceTexture;FZ)V");
    appThread->setVideoSrcOnUIThreadMethodId = env->GetMethodID(appThread->activityObjectJClass, "setVideoSrcOnUIThread", "(Landroid/graphics/SurfaceTexture;Ljava/lang/String;)V");
    appThread->newVideoMethodId = env->GetMethodID(appThread->activityObjectJClass, "newVideo", "(Landroid/graphics/SurfaceTexture;I)V");
    appThread->deleteVideoMethodId = env->GetMethodID(appThread->activityObjectJClass, "deleteVideo", "(Landroid/graphics/SurfaceTexture;)V");
    appThread->setVideoVolumeMethodId = env->GetMethodID(appThread->activityObjectJClass, "setVideoVolume", "(Landroid/graphics/SurfaceTexture;F)V");
    appThread->setVideoLoopMethodId = env->GetMethodID(appThread->activityObjectJClass, "setVideoLoop", "(Landroid/graphics/SurfaceTexture;Z)V");
    appThread->getVideoDurationMethodId = env->GetMethodID(appThread->activityObjectJClass, "getVideoDuration", "(Landroid/graphics/SurfaceTexture;)I");
    appThread->getVideoWidthMethodId = env->GetMethodID(appThread->activityObjectJClass, "getVideoWidth", "(Landroid/graphics/SurfaceTexture;)I");
    appThread->getVideoHeightMethodId = env->GetMethodID(appThread->activityObjectJClass, "getVideoHeight", "(Landroid/graphics/SurfaceTexture;)I");
    appThread->getVideoCurrentTimeMethodId = env->GetMethodID(appThread->activityObjectJClass, "getVideoCurrentTime", "(Landroid/graphics/SurfaceTexture;)I");
    // VRWEBGL END

	const int createErr = pthread_create( &appThread->Thread, NULL, AppThreadFunction, appThread );
	if ( createErr != 0 )
	{
		ALOGE( "pthread_create returned %i", createErr );
	}
}

static void ovrAppThread_Destroy( ovrAppThread * appThread, JNIEnv * env )
{
	ALOGV("VRWebGL: Waiting for the app thread to finish...");
	pthread_join( appThread->Thread, NULL );
	env->DeleteGlobalRef( appThread->ActivityObject );
    // (*env)->DeleteGlobalRef( env, appThread->matrixFloatArray );
	ovrMessageQueue_Destroy( &appThread->MessageQueue );
	ALOGV("VRWebGL: App thread finished!");
}

// VRWEBGL BEGIN
// TODO: HACK: Using this global static variable sucks but right now I can't think of a better solution... :(
// Maybe some of the static variables I am using for pose acquisition and such could benefit with message handling... maybe?
static ovrAppThread * appThread = 0;

void VRWebGLCommandProcessor::getPose(VRWebGLPose& pose)
{
    // ==============================================
    // THIS CODE IS JUST FOR REFERENCE PURPOSES! BEGIN
    //            // Position and orientation together.
    //            typedef struct ovrPosef_
    //            {
    //                ovrQuatf	Orientation;
    //                ovrVector3f	Position;
    //            } ovrPosef;
    //            typedef struct ovrRigidBodyPosef_
    //            {
    //                ovrPosef	Pose;
    //                ovrVector3f	AngularVelocity;
    //                ovrVector3f	LinearVelocity;
    //                ovrVector3f	AngularAcceleration;
    //                ovrVector3f	LinearAcceleration;
    //                double		TimeInSeconds;			// Absolute time of this pose.
    //                double		PredictionInSeconds;	// Seconds this pose was predicted ahead.
    //            } ovrRigidBodyPosef;
    //
    //            // Bit flags describing the current status of sensor tracking.
    //            typedef enum
    //            {
    //                VRAPI_TRACKING_STATUS_ORIENTATION_TRACKED	= 0x0001,	// Orientation is currently tracked.
    //                VRAPI_TRACKING_STATUS_POSITION_TRACKED		= 0x0002,	// Position is currently tracked.
    //                VRAPI_TRACKING_STATUS_HMD_CONNECTED			= 0x0080	// HMD is available & connected.
    //            } ovrTrackingStatus;
    //
    //            // Tracking state at a given absolute time.
    //            typedef struct ovrTracking_
    //            {
    //                // Sensor status described by ovrTrackingStatus flags.
    //                unsigned int		Status;
    //                // Predicted head configuration at the requested absolute time.
    //                // The pose describes the head orientation and center eye position.
    //                ovrRigidBodyPosef	HeadPose;
    //            } ovrTracking;
    // THIS CODE IS JUST FOR REFERENCE PURPOSES! END
    // ==============================================    
    pthread_mutex_lock( &ovrApp::HeadTrackingInfoMutex );
    pose.orientation[0] = ovrApp::Pose.Pose.Orientation.x;
    pose.orientation[1] = ovrApp::Pose.Pose.Orientation.y;
    pose.orientation[2] = ovrApp::Pose.Pose.Orientation.z;
    pose.orientation[3] = ovrApp::Pose.Pose.Orientation.w;
    pthread_mutex_unlock( &ovrApp::HeadTrackingInfoMutex );	    
}

void VRWebGLCommandProcessor::getEyeParameters(VRWebGLEyeParameters& eyeParameters)
{
    pthread_mutex_lock( &ovrApp::HeadTrackingInfoMutex );
    eyeParameters.xFOV = ovrApp::EyeParameters.xFOV;
    eyeParameters.yFOV = ovrApp::EyeParameters.yFOV;
    eyeParameters.width = ovrApp::EyeParameters.width;
    eyeParameters.height = ovrApp::EyeParameters.height;
    eyeParameters.interpupillaryDistance = ovrApp::EyeParameters.interpupillaryDistance;
    pthread_mutex_unlock( &ovrApp::HeadTrackingInfoMutex );	    	
}

void VRWebGLCommandProcessor::setCameraProjectionMatrix(GLfloat* cameraProjectionMatrix)
{
	GLfloat far = (cameraProjectionMatrix[14]) / (1 + cameraProjectionMatrix[10]);
	GLfloat near = (far + far * cameraProjectionMatrix[10]) / (cameraProjectionMatrix[10] - 1);
    pthread_mutex_lock( &ovrRenderer::NewNearFarMutex );
    ovrRenderer::NewNear = near;
    ovrRenderer::NewFar = far;
    pthread_mutex_unlock( &ovrRenderer::NewNearFarMutex );	    
}

void VRWebGLCommandProcessor::setRenderEnabled(bool flag)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_SET_RENDER_ENABLED, MQ_WAIT_RECEIVED );
    ovrMessage_SetIntegerParm(&message, 0, (int)(flag ? 1 : 0));
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

GLuint VRWebGLCommandProcessor::newVideoTexture()
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_NEW_VIDEO, MQ_WAIT_PROCESSED );
    GLuint videoTextureId = 0;
    message.Result = &videoTextureId;
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
	return videoTextureId;
}

void VRWebGLCommandProcessor::deleteVideoTexture(GLuint videoTextureId)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_NEW_VIDEO, MQ_WAIT_NONE );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::setVideoSource(GLuint videoTextureId, const std::string& src)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_SET_VIDEO_SRC, MQ_WAIT_NONE );
    std::string* newSrc = new std::string(src);
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    ovrMessage_SetPointerParm(&message, 1, newSrc);
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::playVideo(GLuint videoTextureId, double volume, bool loop)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_PLAY_VIDEO, MQ_WAIT_NONE );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    ovrMessage_SetFloatParm(&message, 1, (float)volume);
    ovrMessage_SetIntegerParm(&message, 2, (int)(loop ? 1 : 0));
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::pauseVideo(GLuint videoTextureId)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_PAUSE_VIDEO, MQ_WAIT_NONE );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::setVideoVolume(GLuint videoTextureId, double volume)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_SET_VIDEO_VOLUME, MQ_WAIT_NONE );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    ovrMessage_SetFloatParm(&message, 1, (float)volume);
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::setVideoLoop(GLuint videoTextureId, bool loop)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_SET_VIDEO_LOOP, MQ_WAIT_NONE );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    ovrMessage_SetIntegerParm(&message, 1, (int)(loop ? 1 : 0));
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

void VRWebGLCommandProcessor::setVideoCurrentTime(GLuint videoTextureId, double currentTime)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_SET_VIDEO_CURRENT_TIME, MQ_WAIT_NONE );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    ovrMessage_SetIntegerParm(&message, 1, (int)(currentTime * 1000.0));
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

double VRWebGLCommandProcessor::getVideoCurrentTime(GLuint videoTextureId)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_GET_VIDEO_CURRENT_TIME, MQ_WAIT_PROCESSED );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    int currentTime = 0;
    message.Result = &currentTime;
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
	return (double)currentTime / 1000.0;
}

double VRWebGLCommandProcessor::getVideoDuration(GLuint videoTextureId)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_GET_VIDEO_DURATION, MQ_WAIT_PROCESSED );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    int duration = 0;
    message.Result = &duration;
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
	return (double)duration / 1000.0;
}

int VRWebGLCommandProcessor::getVideoWidth(GLuint videoTextureId)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_GET_VIDEO_WIDTH, MQ_WAIT_PROCESSED );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    int width = 0;
    message.Result = &width;
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
	return width;
}

int VRWebGLCommandProcessor::getVideoHeight(GLuint videoTextureId)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_GET_VIDEO_HEIGHT, MQ_WAIT_PROCESSED );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    int height = 0;
    message.Result = &height;
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
	return height;
}

bool VRWebGLCommandProcessor::checkVideoPrepared(GLuint videoTextureId)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_CHECK_VIDEO_PREPARED, MQ_WAIT_PROCESSED );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    bool prepared = false;
    message.Result = &prepared;
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
	return prepared;
}

bool VRWebGLCommandProcessor::checkVideoEnded(GLuint videoTextureId)
{
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_CHECK_VIDEO_ENDED, MQ_WAIT_PROCESSED );
    ovrMessage_SetGLuintParm(&message, 0, videoTextureId);
    bool ended = false;
    message.Result = &ended;
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
	return ended;
}

// VRWEBGL END


/*
================================================================================

Activity lifecycle

================================================================================
*/

extern "C"
{
    JNIEXPORT jlong JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnCreate( JNIEnv * env, jobject obj, jobject activity )
    {
        ALOGV( "VRWebGL: onCreate()" );

        appThread = (ovrAppThread *) malloc( sizeof( ovrAppThread ) );
        ovrAppThread_Create( appThread, env, activity );

        ovrMessageQueue_Enable( &appThread->MessageQueue, true );
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_CREATE, MQ_WAIT_PROCESSED );
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );

        return (jlong)((size_t)appThread);
    }

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnStart( JNIEnv * env, jobject obj, jlong handle )
    {
        ALOGV( "VRWebGL: onStart()" );
        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_START, MQ_WAIT_PROCESSED );
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    }

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnResume( JNIEnv * env, jobject obj, jlong handle )
    {
        ALOGV( "VRWebGL: onResume()" );
        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_RESUME, MQ_WAIT_PROCESSED );
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    }

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnPause( JNIEnv * env, jobject obj, jlong handle )
    {
        ALOGV( "VRWebGL: onPause()" );
        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_PAUSE, MQ_WAIT_PROCESSED );
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    }

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnStop( JNIEnv * env, jobject obj, jlong handle )
    {
        ALOGV( "VRWebGL: onStop()" );
        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_STOP, MQ_WAIT_PROCESSED );
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    }

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnDestroy( JNIEnv * env, jobject obj, jlong handle )
    {
        ALOGV( "VRWebGL: onDestroy()" );
        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_DESTROY, MQ_WAIT_PROCESSED );
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
        ovrMessageQueue_Enable( &appThread->MessageQueue, false );

        ovrAppThread_Destroy( appThread, env );
        free( appThread );
    }

    /*
    ================================================================================

    Surface lifecycle

    ================================================================================
    */

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnSurfaceCreated( JNIEnv * env, jobject obj, jlong handle, jobject surface )
    {
        ALOGV( "    onSurfaceCreated()" );
        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);

        ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );
        if ( ANativeWindow_getWidth( newNativeWindow ) < ANativeWindow_getHeight( newNativeWindow ) )
        {
            // An app that is relaunched after pressing the home button gets an initial surface with
            // the wrong orientation even though android:screenOrientation="landscape" is set in the
            // manifest. The choreographer callback will also never be called for this surface because
            // the surface is immediately replaced with a new surface with the correct orientation.
            ALOGE( "        Surface not in landscape mode!" );
        }

        ALOGV( "        NativeWindow = ANativeWindow_fromSurface( env, surface )" );
        appThread->NativeWindow = newNativeWindow;
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED );
        ovrMessage_SetPointerParm( &message, 0, appThread->NativeWindow );
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    }

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnSurfaceChanged( JNIEnv * env, jobject obj, jlong handle, jobject surface )
    {
        ALOGV( "    onSurfaceChanged()" );
        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);

        ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );
        if ( ANativeWindow_getWidth( newNativeWindow ) < ANativeWindow_getHeight( newNativeWindow ) )
        {
            // An app that is relaunched after pressing the home button gets an initial surface with
            // the wrong orientation even though android:screenOrientation="landscape" is set in the
            // manifest. The choreographer callback will also never be called for this surface because
            // the surface is immediately replaced with a new surface with the correct orientation.
            ALOGE( "        Surface not in landscape mode!" );
        }

        if ( newNativeWindow != appThread->NativeWindow )
        {
            if ( appThread->NativeWindow != NULL )
            {
                ovrMessage message;
                ovrMessage_Init( &message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED );
                ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
                ALOGV( "        ANativeWindow_release( NativeWindow )" );
                ANativeWindow_release( appThread->NativeWindow );
                appThread->NativeWindow = NULL;
            }
            if ( newNativeWindow != NULL )
            {
                ALOGV( "        NativeWindow = ANativeWindow_fromSurface( env, surface )" );
                appThread->NativeWindow = newNativeWindow;
                ovrMessage message;
                ovrMessage_Init( &message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED );
                ovrMessage_SetPointerParm( &message, 0, appThread->NativeWindow );
                ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
            }
        }
        else if ( newNativeWindow != NULL )
        {
            ANativeWindow_release( newNativeWindow );
        }
    }

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnSurfaceDestroyed( JNIEnv * env, jobject obj, jlong handle )
    {
        ALOGV( "    onSurfaceDestroyed()" );
        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED );
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
        ALOGV( "        ANativeWindow_release( NativeWindow )" );
        ANativeWindow_release( appThread->NativeWindow );
        appThread->NativeWindow = NULL;
    }

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnPageStarted( JNIEnv * env, jobject obj )
    {
        ALOGV( "    onPageStarted()" );
        VRWebGLCommandProcessor::getInstance()->reset();
    }

    /*
    ================================================================================

    Input

    ================================================================================
    */

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnKeyEvent( JNIEnv * env, jobject obj, jlong handle, int keyCode, int action )
    {
        if ( action == AKEY_EVENT_ACTION_UP )
        {
            ALOGV( "    onKeyEvent( %d, %d )", keyCode, action );
        }
        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_KEY_EVENT, MQ_WAIT_NONE );
        ovrMessage_SetIntegerParm( &message, 0, keyCode );
        ovrMessage_SetIntegerParm( &message, 1, action );
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    }

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeOnTouchEvent( JNIEnv * env, jobject obj, jlong handle, int action, float x, float y )
    {
        if ( action == AMOTION_EVENT_ACTION_UP )
        {
            ALOGV( "    onTouchEvent( %d, %1.0f, %1.0f )", action, x, y );
        }
        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_TOUCH_EVENT, MQ_WAIT_NONE );
        ovrMessage_SetIntegerParm( &message, 0, action );
        ovrMessage_SetFloatParm( &message, 1, x );
        ovrMessage_SetFloatParm( &message, 2, y );
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    }


   /*
    ================================================================================

    Video

    ================================================================================
    */

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeVideoPrepared( JNIEnv *jni, jobject obj, jlong handle, jint textureId )
    {
    	ALOGV( "nativeVideoPrepared" );

        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_VIDEO_PREPARED, MQ_WAIT_NONE );
        ovrMessage_SetIntegerParm(&message, 0, textureId);
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    }

    JNIEXPORT void JNICALL Java_org_chromium_android_1webview_shell_AwShellActivity_nativeVideoEnded( JNIEnv *jni, jobject obj, jlong handle, jint textureId )
    {
    	ALOGV( "nativeVideoPrepared" );

        ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_ON_VIDEO_ENDED, MQ_WAIT_NONE );
        ovrMessage_SetIntegerParm(&message, 0, textureId);
        ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    }

}




