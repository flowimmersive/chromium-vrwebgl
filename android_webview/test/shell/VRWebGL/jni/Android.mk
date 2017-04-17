MY_LOCAL_PATH := $(call my-dir)

# ==========================================================
# gvr
# ==========================================================
# LOCAL_PATH := ../../../../../third_party/gvr-android-sdk
LOCAL_PATH := ./3rdparty/gvr_sdk_mobile_1.40.0/armeabi-v7a
include $(CLEAR_VARS)
LOCAL_MODULE := gvr
LOCAL_SRC_FILES := \
	libgvr.so
#	libgvr_shim_static_arm.a
# include $(PREBUILT_STATIC_LIBRARY)
include $(PREBUILT_SHARED_LIBRARY)

# ==========================================================
# VRWebGL_GVRMobileSDK
# ==========================================================
LOCAL_PATH := .
include $(CLEAR_VARS)
LOCAL_MODULE := VRWebGL_GVRMobileSDK
LOCAL_C_INCLUDES := \
	. \
	./3rdparty/gvr_sdk_mobile_1.40.0/include \
	../../../../../third_party/WebKit/Source \
	../../../../../third_party/WebKit
#	../../../../../third_party/gvr-android-sdk/src/libraries/headers
LOCAL_SRC_FILES := \
	./VRWebGL_GVRMobileSDK.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLCommandProcessor.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLCommand.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLMath.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLSurfaceTexture.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGL_gl.cpp 	
LOCAL_CFLAGS := -std=gnu++11 -Werror 
# LOCAL_STATIC_LIBRARIES := 
LOCAL_SHARED_LIBRARIES := \
	gvr
LOCAL_LDLIBS := -llog -landroid -lGLESv3 -lEGL
include $(BUILD_SHARED_LIBRARY)
