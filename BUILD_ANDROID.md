Please read the [general build guide](BUILD.md) for information on building other platform. Only Android specific instructions are found in this file.

# Dependencies

Building is currently supported on OSX, Windows and Linux platforms, but developers intending to do work on the library dependencies are strongly urged to use 64 bit Linux as a build platform

You will need the following tools to build Android targets.

* [Android Studio](https://developer.android.com/studio/index.html)

### Android Studio

Download the Android Studio installer and run it.   Once installed, at the welcome screen, click configure in the lower right corner and select SDK manager

From the SDK Platforms tab, select API levels 24 and 26.  

From the SDK Tools tab select the following

* Android SDK Build-Tools
* GPU Debugging Tools
* CMake (even if you have a separate CMake installation)
* LLDB 
* Android SDK Platform-Tools
* Android SDK Tools
* NDK (even if you have the NDK installed separately)

Make sure the NDK installed version is 18 (or higher)

# Environment 

Setting up the environment for android builds requires some additional steps

#### Set up machine specific Gradle properties

Create a `gradle.properties` file in $HOME/.gradle.   Edit the file to contain the following

    HIFI_ANDROID_PRECOMPILED=<your_home_directory>/Android/hifi_externals

Note, do not use `$HOME` for the path.  It must be a fully qualified path name.

### Setup the repository

Clone the repository

`git clone https://github.com/highfidelity/hifi.git`

Enter the repository `android` directory

`cd hifi/android`

Execute two gradle pre-build steps.  This steps should only need to be done once, unless you're working on the Android dependencies

`./gradlew extractDependencies`

`./gradlew setupDependencies`

# Building & Running

* Open Android Studio
* Choose _Open Existing Android Studio Project_
* Navigate to the `hifi` repository and choose the `android` folder and select _OK_
* From the _Build_ menu select _Make Project_
* Once the build completes, from the _Run_ menu select _Run App_ 

