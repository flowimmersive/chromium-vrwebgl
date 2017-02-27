MY_LOCAL_PATH := $(call my-dir)

# ==========================================================
# gvr
# ==========================================================
LOCAL_PATH := ../../../../../third_party/gvr-android-sdk
include $(CLEAR_VARS)
LOCAL_MODULE := gvr
LOCAL_SRC_FILES := \
	libgvr_shim_static_arm.a
include $(PREBUILT_STATIC_LIBRARY)

# ==========================================================
# VRWebGL_GVRMobileSDK
# ==========================================================
LOCAL_PATH := .
include $(CLEAR_VARS)
LOCAL_MODULE := VRWebGL_GVRMobileSDK
LOCAL_C_INCLUDES := \
	. \
	../../../../../third_party/gvr-android-sdk/src/libraries/headers \
	../../../../../third_party/WebKit/Source
LOCAL_SRC_FILES := \
	./VRWebGL_GVRMobileSDK.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLCommandProcessor.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLCommand.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLMath.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGL_gl.cpp 	
LOCAL_CFLAGS := -std=gnu++11 -Werror 
LOCAL_STATIC_LIBRARIES := \
	gvr
LOCAL_LDLIBS := -llog -landroid -lGLESv3 -lEGL
include $(BUILD_SHARED_LIBRARY)
