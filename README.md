# Index

* [Overview](#overview)
* [How to build your own version of Chromium with VRWebGL](#how_to_build_your_own_version_of_chromium_with_vrwebgl)
* [License](#license)

# <a name="overview">Overview</a>

**WORK IN PROGRESS**

# <a name="how_to_build_your_own_version_of_chromium_with_vrwebgl"></a>How to build your own version of Chromium with VRWebGL

1. Clone the Chromium project and prepare it to build it.
2. Build, install and run.

## 1. Clone the Chromium project and prepare it to be built

Chromium cloning/building instruction are available online: [https://www.chromium.org/developers/how-tos/android-build-instructions](https://www.chromium.org/developers/how-tos/android-build-instructions)

Anyway, in order to help with the process, we recommend you follow the following steps. 

VRWebGL is only available on the Android platform for the moment so in order to be able to use the modifications present in this project, you need to compile Chromium for Android that can only be done on Linux. Unfortunately, this document does not include instructions on how to setup a linux machine.

This tutorial assumes you are ussing SSH to work with github. You can learn how to setup SSH in github [here](https://help.github.com/articles/connecting-to-github-with-ssh/).

Let's assume that the machine is installed along with:

* GIT
* Python

Open a terminal window to be able

1. Install depot_tools. You can follow this [tutorial](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up) or simply follow these 2 steps:
  * `git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git`
  * `export PATH=$PATH:/path/to/depot_tools`
2. Create a folder to contain `chromium`: `$ mkdir ~/chromium && cd ~/chromium`
3. Checkout the Chromium repo: `~/chromium$ fetch --nohooks android`. **NOTE**: This process may take a long time (an hour?)
4. Enter the `src` folder: `$ cd src`.
5. Add a remote pointing to this repo:`git remote add vrwebgl git@github.com:flowimmersive/chromium-vrwebgl.git`. You can use a different name for your remote than `vrwebgl`, but remember the name as you will need to use it from now on.
6. Fetch the new remote: `git fetch vrwebgl`
7. Checkout the remote branch from the new remote of the corresponding VRWebGL version you want to work on, for example: `git checkout --track vrwebgl/vrwebgl_57.0.2987.5_oculus`. The branches that represent VRWebGL implementations start with the `vrwebgl_` prefix. The name of the branch is important as it will be used in the build step down below.
8. Create a folder where to make the final product compilation using the same name as the branch checked out in a step above: `~/chromium/src$ mkdir -p out/vrwebgl_57.0.2987.5_oculus` in our example.
9. Create and edit a new file `out/vrwebgl_57.0.2987.5_oculus/args.gn` with the command `~/chromium/src$ gedit out/vrwebgl_57.0.2987.5_oculus/args.gn` (or any other editor). Copy and paste the following content in the `args.gn` file:
  ```
  target_os = "android"
  target_cpu = "arm" 

  is_debug = false
  is_component_build = true

  proprietary_codecs = false
  ffmpeg_branding = "Chromium"

  enable_nacl = false
  remove_webcore_debug_symbols = true
  ```
10. Prepare to build: `~/chromium/src$ gn args out/vrwebgl_57.0.2987.5_oculus`. **NOTE**: once the command is executed, the vi editor will show you the content of the `args.gn` file just edited a few steps before. Just exit by pressing ESC and typing colon `':'` and `'q'` with an exclamation mark `'!'` = `:q!`.
11. Install the build dependencies: `~/chromium/src$ build/install-build-deps-android.sh` 
12. Synchronize the dependencies: `~/chromium/src$ gclient sync --disable-syntax-validation`
13. Setup the environment: `~/chromium/src$ . build/android/envsetup.sh`

I know, many steps to be followed, but once you have completed all of them (remember that some will take a loooong time to finish), you won't need to execute them again (except from `gclient sync --disable-syntax-validation` that you may need to execute it occassionally if you rebase a different tag or a branch based on a different tag).

## 2. Build, install and run

This tutorial specified that the name of the out folder created during the setup process above is the same as the branch (`vrwebgl_57.0.2987.5_oculus`). This is no coincidence, as the `build_install_run.sh` shell script provided along with this documentation allows to build the Chromium project depending on the current checked out git branch. This script not only compiles Chromium but also the native library to handle VRWebGL calls. Moreover, this script also installs the final APK on to a connected device and runs it, so it is convenient that you to connect the Android device via USB before executing it. The project that will be built by default is the Chromium WebView project, the only one that has been modified to provide VRWebGL capabilities.
```
~/chromium/src/build_install_run.sh
```
You can review the content of the script to see what it does (it is a fairly simple script) but if you would like to compile the final APK on your own you could do it by executing the following command:
```
~/chromium/src$ ninja -C out/vrwebgl_57.0.2987.5_oculus
```
The final APK will be built in the folder `~/chromium/src/out/vrwebgl_57.0.2987.5_oculus/out/apks`.

**NOTE:** It is important to note that this tutorial assumes the Oculus version of VRWebGL but there is also a Google VR version available in a different branch. Maybe some day these 2 branches will be merged into one.

