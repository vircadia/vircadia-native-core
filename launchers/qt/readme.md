# HQ Launcher
Behavior of the HQ Launcher is as follows:
* Update the HQ Launcher to the latest version
* Sign up and sign in is the user is not
* Download the latest Interface client
* Launching the user in the current HQ domain

# directory structure

## src/ - contains the c++ and objective-c.
* `BuildsRequest` - getting / parsing the build info from thunder api
* `CommandlineOptions` - parses and stores commandline arguments
* `Helper` - helper functions
* `Helper_darwin` - objective-c implemention of helper funcions
* `Helper_windows` - helper function that depend on windows api
* `Launcher` - initialized the Launcher Application and resources
* `LauncherInstaller_windows` - logic of how to install/uninstall HQ Launcher on windows
* `LauncherState` - hold majority of the logic of the launcher (signin, config file, updating, running launcher)
  * config files hold the following saved data
    * logged in
    * home location
* `LauncherWindows` - wrapper for `QQuickWindow` that implements drag feature
* `LoginRequest` - checks the login credentials the user typed in.
* `NSTask+NSTaskExecveAddtions` - Extension of NSTask for replacing Launcher process with interface client process
* `PathUtils` - Helper class for getting relative paths for HQ Launcher
* `SignupRequest` - Determines if the users request to signup for a new account succeeded based on the entered credentials
* `Unzipper` - helper class for extracting zip files
* `UserSettingsRequest` - getting the users setting (home location) from metaverse

## resources/
* `images/`-  Holds the images and icon that are used by the launcher
* `qml/`
  * UI elements
  * `QML_FILE_FOR_UI_STATE` varible in LauchherState defines what qml files are used by the laucnher.