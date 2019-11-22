# Dependencies
- [cmake](https://cmake.org/download/):  3.9

# Windows
cmake -G "Visual Studio 16 2019" ..

# OSX
cmake -G Xcode ..


if you wish to not use the compiled qml files pass the `-DLAUNCHER_SOURCE_TREE_RESOURCES=On` argument to cmake.