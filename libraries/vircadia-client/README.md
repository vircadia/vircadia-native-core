### Client API

This is C/C++ library for Vircadia client->server communication.

### Building

To build the dynamic library.
```
cmake --build . --target vircadia-client
```


To build the unit tests:
```
cmake --build . --target vircadia-client-tests
```
The tests can be run with `ctest`.


To build the documentation (requires doxygen):
```
cmake --build . --target vircadia-client-docs
```

### Packaging

Several build options can be set to minimize dependencies of the library and simplify packaging.
- `ENABLE_WEBRTC_DATA_CHANNELS` can be set to `OFF` since that is only used by the server to support the Web SDK and client.
- `STATIC_STDLIB` can be set to `ON` to not require standard runtime libraries that might not be present on a bare-bones system.

The following commands in the build directory will produce a package containing all public headers and the dynamic library.
```
cmake .. -DENABLE_WEBRTC_DATA_CHANNELS=OFF -DSTATIC_STDLIB=ON -DCMAKE_INSTALL_PREFIX:PATH=vircadia-client-package
cd libraries/vircadia-client
cmake --build . --target install/strip
```
With these options the only direct dependencies will be QtCore, QtScript, QtNetwork and OpenSSL libraries, but dependent on the system some other indirect dependencies might need to be included in the package as well.



