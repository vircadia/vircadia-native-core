# Dependencies
- [cmake](https://cmake.org/download/):  3.9

# Windows
* Download `Visual Studio 2019`
`cmake -G "Visual Studio 16 2019" ..`

# MacOS
* Install `Xcode`
`cmake -G Xcode ..`


If you wish to not use the compiled qml files, pass the `-DLAUNCHER_SOURCE_TREE_RESOURCES=On` argument to cmake.