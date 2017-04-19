#ifndef VRWebGLSurfaceTexture_h
#define VRWebGLSurfaceTexture_h

#include <jni.h>

#include <vector>
#include <memory>

class VRWebGLSurfaceTexture
{
public:
	VRWebGLSurfaceTexture( JNIEnv * jni );
	~VRWebGLSurfaceTexture();
	void setDefaultBufferSize( const int width, const int height );
	void update();
	unsigned getTextureId() const;
	jobject	getJavaObject() const;
	long long getNanoTimeStamp() const;

private:
	unsigned textureId;
	jobject javaObject;
	JNIEnv* jni;
	long long nanoTimeStamp;
	jmethodID updateTexImageMethodId;
	jmethodID getTimestampMethodId;
	jmethodID setDefaultBufferSizeMethodId;
};

class VRWebGLSurfaceTextures
{
private:
	std::vector<std::shared_ptr<VRWebGLSurfaceTexture>> surfaceTextures;
public:
	std::shared_ptr<VRWebGLSurfaceTexture> newSurfaceTexture(JNIEnv* jni);
	void deleteSurfaceTexture(const std::shared_ptr<VRWebGLSurfaceTexture>& surfaceTexture);
	std::shared_ptr<VRWebGLSurfaceTexture> findSurfaceTextureByTextureId(unsigned textureId) const;
	void update();
	void update(unsigned textureId);
	void clear();
};

#endif