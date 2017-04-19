#include "modules/vr/VRWebGLSurfaceTexture.h"
#include "modules/vr/VRWebGLCommand.h" // Just needed to define VRWEBGL_SHOW_LOG

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <android/log.h>

#include <algorithm>

#define LOG_TAG "VRWebGL"

#define ALOGE(...) __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__ )
#ifdef VRWEBGL_SHOW_LOG
#define ALOGV(...) __android_log_print( ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__ )
#else
#define ALOGV(...)
#endif

VRWebGLSurfaceTexture::VRWebGLSurfaceTexture(JNIEnv* jni) :
	textureId(0),
	javaObject(NULL),
	jni(NULL),
	nanoTimeStamp(0),
	updateTexImageMethodId(NULL),
	getTimestampMethodId(NULL),
	setDefaultBufferSizeMethodId(NULL)
{
	this->jni = jni;

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

VRWebGLSurfaceTexture::~VRWebGLSurfaceTexture()
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

void VRWebGLSurfaceTexture::setDefaultBufferSize( const int width, const int height )
{
	jni->CallVoidMethod( javaObject, setDefaultBufferSizeMethodId, width, height );
}

void VRWebGLSurfaceTexture::update()
{
  if ( !javaObject ) {
  	return;
  }

  // TODO: There should not be an error, but clean it before the call so the logcat is clearer (the call shows a log message stating that is cleaning a GL error).
  glGetError();
	jni->CallVoidMethod( javaObject, updateTexImageMethodId );
	nanoTimeStamp = jni->CallLongMethod( javaObject, getTimestampMethodId );
}

unsigned VRWebGLSurfaceTexture::getTextureId() const
{
	return textureId;
}

jobject VRWebGLSurfaceTexture::getJavaObject() const
{
	return javaObject;
}

long long VRWebGLSurfaceTexture::getNanoTimeStamp() const
{
	return nanoTimeStamp;
}

std::shared_ptr<VRWebGLSurfaceTexture> VRWebGLSurfaceTextures::newSurfaceTexture(JNIEnv* jni)
{
	std::shared_ptr<VRWebGLSurfaceTexture> surfaceTexture(new VRWebGLSurfaceTexture(jni));
	surfaceTextures.push_back(surfaceTexture);
	return surfaceTexture;
}

void VRWebGLSurfaceTextures::deleteSurfaceTexture(const std::shared_ptr<VRWebGLSurfaceTexture>& surfaceTexture)
{
	if (!surfaceTexture) return;
	std::vector<std::shared_ptr<VRWebGLSurfaceTexture>>::iterator it = std::find(surfaceTextures.begin(), surfaceTextures.end(), surfaceTexture);
	if (it != surfaceTextures.end())
	{
		surfaceTextures.erase(it);
	}
}

std::shared_ptr<VRWebGLSurfaceTexture> VRWebGLSurfaceTextures::findSurfaceTextureByTextureId(unsigned textureId) const
{
	std::vector<std::shared_ptr<VRWebGLSurfaceTexture>>::const_iterator it;
	for (it = surfaceTextures.begin(); it != surfaceTextures.end() && (*it)->getTextureId() != textureId; it++);
	return it != surfaceTextures.end() ? *it : std::shared_ptr<VRWebGLSurfaceTexture>();
}

void VRWebGLSurfaceTextures::update()
{
	std::vector<std::shared_ptr<VRWebGLSurfaceTexture>>::iterator it;
	for (it = surfaceTextures.begin(); it != surfaceTextures.end(); it++)
	{
		(*it)->update();
	}
}

void VRWebGLSurfaceTextures::update(unsigned textureId)
{
  std::vector<std::shared_ptr<VRWebGLSurfaceTexture>>::iterator it;
  for (it = surfaceTextures.begin(); it != surfaceTextures.end(); it++)
  {
    if ((*it)->getTextureId() == textureId)
    {
      (*it)->update();
      break;
    }
  }
}

void VRWebGLSurfaceTextures::clear()
{
  surfaceTextures.clear();
}
