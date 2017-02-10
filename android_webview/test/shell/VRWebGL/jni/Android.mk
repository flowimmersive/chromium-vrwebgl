MY_LOCAL_PATH := $(call my-dir)

# ==========================================================
# vrapi
# ==========================================================
LOCAL_PATH := ./3rdparty/ovr_sdk_mobile_1.0.3/armeabi-v7a
include $(CLEAR_VARS)
LOCAL_MODULE := vrapi
LOCAL_SRC_FILES := \
	libvrapi.so
include $(PREBUILT_SHARED_LIBRARY)

# ==========================================================
# systemutils
# ==========================================================
LOCAL_PATH := ./3rdparty/ovr_sdk_mobile_1.0.3/armeabi-v7a
include $(CLEAR_VARS)
LOCAL_MODULE := systemutils
LOCAL_SRC_FILES := \
	libsystemutils.a
include $(PREBUILT_STATIC_LIBRARY)

# ==========================================================
# openglloader
# ==========================================================
LOCAL_PATH := ./3rdparty/ovr_sdk_mobile_1.0.3/armeabi-v7a
include $(CLEAR_VARS)
LOCAL_MODULE := openglloader
LOCAL_SRC_FILES := \
	libopenglloader.a
include $(PREBUILT_STATIC_LIBRARY)

# ==========================================================
# ovrkernel
# ==========================================================
LOCAL_PATH := ./3rdparty/ovr_sdk_mobile_1.0.3/armeabi-v7a
include $(CLEAR_VARS)
LOCAL_MODULE := ovrkernel
LOCAL_SRC_FILES := \
	libovrkernel.a
include $(PREBUILT_STATIC_LIBRARY)

# ==========================================================
# VRWebGL
# ==========================================================
# LOCAL_PATH := .
# include $(CLEAR_VARS)
# LOCAL_MODULE := VRWebGL
# LOCAL_C_INCLUDES := \
# 	../../../../../third_party/WebKit/Source
# LOCAL_SRC_FILES := \
# 	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLCommandProcessor.cpp \
# 	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLCommand.cpp 
# LOCAL_CFLAGS := -std=c++11 -Werror 
# include $(BUILD_STATIC_LIBRARY)

# ==========================================================
# VRWebGL_OculusMobileSDK
# ==========================================================
LOCAL_PATH := .
include $(CLEAR_VARS)
LOCAL_MODULE := VRWebGL_OculusMobileSDK
LOCAL_C_INCLUDES := \
	. \
	./3rdparty/ovr_sdk_mobile_1.0.3/include \
	../../../../../third_party/WebKit/Source
LOCAL_SRC_FILES := \
	./VRWebGL_OculusMobileSDK.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLCommandProcessor.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLCommand.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGLMath.cpp \
	../../../../../third_party/WebKit/Source/modules/vr/VRWebGL_gl.cpp 	
LOCAL_CFLAGS := -std=gnu++11 -Werror 
LOCAL_SHARED_LIBRARIES := \
	vrapi
LOCAL_WHOLE_STATIC_LIBRARIES := \
	openglloader 
	# VRWebGL
LOCAL_STATIC_LIBRARIES := \
	systemutils \
	ovrkernel
LOCAL_LDLIBS := -llog -landroid -lGLESv3 -lEGL
include $(BUILD_SHARED_LIBRARY)
