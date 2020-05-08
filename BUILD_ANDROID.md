# Build Android

*Last Updated on December 21, 2019*

Please read the [general build guide](BUILD.md) for information on building other platforms. Only Android specific instructions are found in this file. **Note that these instructions apply to building for Oculus Quest.**

## Dependencies

Building is currently supported on Windows, OSX and Linux, but developers intending to do work on the library dependencies are strongly urged to use 64 bit Linux as a build platform.

### OS specific dependencies

Please install the dependencies for your OS using the [Windows](BUILD_WIN.md), [OSX](BUILD_OSX.md) or [Linux](BUILD_LINUX.md) build instructions before attempting to build for Android.

### Android Studio

Download the [Android Studio](https://developer.android.com/studio/index.html) installer and run it. Once installed, at the welcome screen, click _Configure_ in the lower right corner and select _SDK Manager_.

From the _SDK Platforms_ tab, select API levels 26 and 28.  

From the _SDK Tools_ tab, select the following

* Android SDK Build-Tools
* GPU Debugging Tools
* LLDB 
* Android SDK Platform-Tools
* Android SDK Tools
* NDK (even if you have the NDK installed separately)

Still in the _SDK Tools_ tab, click _Show Package Details_. Select CMake 3.6.4. Do this even if you have a separate CMake installation.

Also, make sure the NDK installed version is 18 (or higher).

## Environment

### Create a keystore in Android Studio
Follow the directions [here](https://developer.android.com/studio/publish/app-signing#generate-key) to create a keystore file. You can save it anywhere (preferably not in the `hifi` folder).

### Set up machine specific Gradle properties

Create a `gradle.properties` file in the `.gradle` folder (`$HOME/.gradle` on Unix, `Users/<yourname>/.gradle` on Windows). Edit the file to contain the following

    HIFI_ANDROID_PRECOMPILED=<your_home_directory>/Android/hifi_externals
    HIFI_ANDROID_KEYSTORE=<key_store_directory>/<keystore_name>.jks
    HIFI_ANDROID_KEYSTORE_PASSWORD=<password>
    HIFI_ANDROID_KEY_ALIAS=<key_alias>
    HIFI_ANDROID_KEY_PASSWORD=<key_passwords>

Note, do not use $HOME for the path. It must be a fully qualified path name. Also, be sure to use forward slashes in your path.

#### If you are building for an Android phone

Add these lines to `gradle.properties`

    SUPPRESS_QUEST_INTERFACE
    SUPPRESS_QUEST_FRAME_PLAYER

#### If you are building for an Oculus Quest

Add these lines to `gradle.properties`

    SUPPRESS_INTERFACE
    SUPPRESS_FRAME_PLAYER

The above code to suppress modules is not necessary, but will speed up the build process.

### Clone the repository

`git clone https://github.com/kasenvr/project-athena.git`

## Building & Running

### Building Modules

* Open Android Studio
* Choose _Open an existing Android Studio project_
* Navigate to the `hifi` repository and choose the `android` folder and select _OK_
* Wait for Gradle to sync (this should take around 20 minutes the first time)
* From the _Build_ menu select _Make Project_

### Running a Module

* In the toolbar at the top of Android Studio, next to the green hammer icon, you should see a dropdown menu.
* You may already see a configuration for the module you are trying to build. If so, select it. 
* Otherwise, select _Edit Configurations_.

Your configuration should be as follows

* Type: Android App
* Module: <your module> (you probably want `interface` or `questInterface`)

For the interface modules, you also need to select the activity to launch. 

#### For the Android phone interface

* From the _Launch_ drop down menu, select _Specified Activity_
* In the _Activity_ field directly below, put `io.highfidelity.hifiinterface.PermissionChecker`

#### For the Oculus Quest interface

* From the _Launch_ drop down menu, select _Specified Activity_
* In the _Activity_ field directly below, put `io.highfidelity.questInterface.PermissionsChecker`

Note the 's' in Permission**s**Checker for the Quest.

Now you are able to run your module! Click the green play button in the top toolbar of Android Studio.

## Troubleshooting

To view a more complete debug log,

* Click the icon with the two overlapping squares in the upper left corner of the tab where the sync is running (hover text says _Toggle view_)
* To change verbosity, click _File > Settings_. Under _Build, Execution, Deployment > Compiler_ you can add command-line flags, as per Gradle documentation

Some things you can try if you want to do a clean build
 
* Delete the `build` and `.externalNativeBuild` folders from the folder for each module you're building (for example, `hifi/android/apps/interface`)
* If you have set your `HIFI_VCPKG_ROOT` environment variable, delete the contents of that directory; otherwise, delete `AppData/Local/Temp/hifi`
* In Android Studio, click _File > Invalidate Caches / Restart_ and select _Invalidate and Restart_

If you see lots of "couldn't acquire lock" errors,
* Open Task Manager and close any running Clang / Gradle processes