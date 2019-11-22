# HQ Launcher
Behavior of the HQ Launcher is as follows:
* Launching the user in the current HQ domain
* Update the Interface client to the current version.
* Update the HQ Launcher to the current version

# directory structure

## src/ - contains the c++ and objective-c.
* LauncherState - hold majority of the logic of the launcher (signin, config file, updating, running launcher)
* LauncherInstaller_windows - logic of how to install/uninstall HQ Launcher on windows
* Helper - helper functions
* UserSettings - getting the users setting (home location) from metaverse
* BuildsRequest - getting / parsing the build info from thunder api
* LoginRequest - checks the login credentials the user typed in. 
* Unzipper - helper class for extracting zip files

## resources/
* image/ - Holds the images and icon that are used by the launcher
* qml/ - UI elements - `QML_FILE_FOR_UI_STATE` varible in LauchherState defines what qml files are used by the laucnher.