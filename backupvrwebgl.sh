# Get the branch name to know the output folder
BRANCH_NAME=$(git symbolic-ref -q HEAD)
BRANCH_NAME=${BRANCH_NAME##refs/heads/}
BRANCH_NAME=${BRANCH_NAME:-HEAD}
# Remove the backup folder for the branch
rm -rf ../Backup_VRWebGL/$BRANCH_NAME/*
# WebKit vr
mkdir -p ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/third_party/WebKit/Source/modules/vr
if [ $? -ne 0 ]; then exit 1; fi
cp -r third_party/WebKit/Source/modules/vr/*.* ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/third_party/WebKit/Source/modules/vr/
if [ $? -ne 0 ]; then exit 1; fi
cp -r third_party/WebKit/Source/modules/BUILD.gn ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/third_party/WebKit/Source/modules/
if [ $? -ne 0 ]; then exit 1; fi
cp -r third_party/WebKit/Source/modules/modules_idl_files.gni ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/third_party/WebKit/Source/modules/
if [ $? -ne 0 ]; then exit 1; fi
# Android Chromium Webview
# NOTE: Could copy only the elements that have been changed.
mkdir -p ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/android_webview/test
if [ $? -ne 0 ]; then exit 1; fi
cp -r android_webview/test/shell ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/android_webview/test
if [ $? -ne 0 ]; then exit 1; fi
cp android_webview/test/BUILD.gn ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/android_webview/test
if [ $? -ne 0 ]; then exit 1; fi
# Remove the temporary files
rm -rf ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/android_webview/test/shell/VRWebGL/libs
if [ $? -ne 0 ]; then exit 1; fi
rm -rf ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/android_webview/test/shell/VRWebGL/obj
if [ $? -ne 0 ]; then exit 1; fi
rm -rf ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/android_webview/test/shell/VRWebGL/jni/objs
if [ $? -ne 0 ]; then exit 1; fi
rm -rf ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/android_webview/test/shell/tango
if [ $? -ne 0 ]; then exit 1; fi
# Some additional interesting files related to how the webview should operate
mkdir -p ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/build/android/lint
mkdir -p ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/android_webview/java/src/org/chromium/android_webview
if [ $? -ne 0 ]; then exit 1; fi
cp build/android/adb_run_android_webview_shell ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/build/android
cp build/android/lint/suppressions.xml ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/build/android/lint
cp android_webview/java/src/org/chromium/android_webview/AwBrowserProcess.java ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/android_webview/java/src/org/chromium/android_webview
# APK
mkdir ../Backup_VRWebGL/$BRANCH_NAME/bin
if [ $? -ne 0 ]; then exit 1; fi
cp out/$BRANCH_NAME/apks/AndroidWebView.apk ../Backup_VRWebGL/$BRANCH_NAME/bin/VRWebGL.apk
if [ $? -ne 0 ]; then exit 1; fi
# Build script, notes, backup script, examples, ... 
cp build_install_run.sh ../Backup_VRWebGL/$BRANCH_NAME/chromium/src/
if [ $? -ne 0 ]; then exit 1; fi
cp ./backupvrwebgl.sh ../Backup_VRWebGL/$BRANCH_NAME/chromium/src
if [ $? -ne 0 ]; then exit 1; fi
cp ./advancedlogcat.sh ../Backup_VRWebGL/$BRANCH_NAME/chromium/src
if [ $? -ne 0 ]; then exit 1; fi
