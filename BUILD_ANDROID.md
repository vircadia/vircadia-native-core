Please read the [general build guide](BUILD.md) for information on building other platform. Only Android specific instructions are found in this file.

# Dependencies

*Currently Android building is only supported on 64 bit Linux host environments*

You will need the following tools to build our Android targets.

* [Gradle](https://gradle.org/install/)
* [Android Studio](https://developer.android.com/studio/index.html)

### Gradle

Install gradle version 4.1 or higher.  Following the instructions to install via [SDKMAN!](http://sdkman.io/install.html) is recommended.  

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

Execute a gradle pre-build setup.  This step should only need to be done once

`gradle setupDepedencies`


# Building & Running

* Open Android Studio
* Choose _Open Existing Android Studio Project_
* Navigate to the `hifi` repository and choose the `android` folder and select _OK_
* If Android Studio asks you if you want to use the Gradle wrapper, select cancel and tell it where your local gradle installation is.  If you used SDKMAN to install gradle it will be located in `$HOME/.sdkman/candidates/gradle/current/`
* From the _Build_ menu select _Make Project_
* Once the build completes, from the _Run_ menu select _Run App_ 

