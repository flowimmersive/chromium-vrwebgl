# Rebuilt (-B) specifying the makefile to use and the output folder for the final libraries and intermediate files
echo "Rebuilding..."
# Add V=1 for a verbose output that shows the commands that are being executed.
# $ANDROID_NDK_PATH/ndk-build NDK_APPLICATION_MK=./Application.mk NDK_LIBS_OUT=../libs NDK_OUT=./objs -B 
../../../../../third_party/android_tools/ndk/ndk-build NDK_APPLICATION_MK=./Application.mk NDK_LIBS_OUT=../libs NDK_OUT=./objs -B 
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ]; then 
	exit $EXIT_CODE
fi
mkdir -p ../../../../../third_party/VRWebGL/lib/armeabi-v7a
cp ../libs/armeabi-v7a/libVRWebGL_OculusMobileSDK.so ../../../../../third_party/VRWebGL/lib/armeabi-v7a
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ]; then 
	exit $EXIT_CODE
fi
echo "Rebuilt!"

