### Client API

This is C/C++ library for Vircadia client->server communication.

### Coding Style

This library is meant to use by other projects in particular other language bindings, and it exposes a C API for that purpose. The C API interfaces adhere to common C coding naming conventions:
- All words in names are separated by underscores (snake case).
- File, variable, function and type names are all lower case.
- Global constants (including enum values) and macros are all upper case.
In other aspects they adheres to the [coding standard](https://github.com/namark/vircadia/blob/master/CODING_STANDARD.md).


The `src/internal` folder contains the internal C++ implementation details and is not subject to these C style naming convention. The `.cpp` files that correspond to C API header files in the `src` folder make use of these internals and may contain a mixture of styles.


The `tests` folder contains unit test for the C API that also adhere to the C naming conventions.

### Building

To build the dynamic library.
```
cmake --build . --target vircadia-client
```


To build the unit tests:
```
cmake --build . --target vircadia-client-tests
```
and to run them:
```
ctest -j10
```
parallelization is important, since some tests have two parts, sender and receiver that communicate. Ideally all test should run in parallel, you can use a number of processes much greater than the processor core count, since most of the test idle most of the time.


To build the documentation (requires doxygen):
```
cmake --build . --target vircadia-client-docs
```

[Android build instructions](BUILD_ANDROID.md).

### Packaging

Several build options can be set to minimize dependencies of the library and simplify packaging.
- `ENABLE_WEBRTC_DATA_CHANNELS` can be set to `OFF` since that is only used by the server to support the Web SDK and client.
- `BUILD_SHARED_LIBS` can be set to `OFF` to link internal libraries statically.
- `STATIC_STDLIB` can be set to `ON` to not require standard runtime libraries that might not be present on a bare-bones system.

The following commands in the build directory will produce a package containing all public headers and the dynamic library.
```
cmake .. -DENABLE_WEBRTC_DATA_CHANNELS=OFF -DBUILD_SHARED_LIBS=OFF -DSTATIC_STDLIB=ON -DCMAKE_INSTALL_PREFIX:PATH=vircadia-client-package
cmake --build . --config Release --target vircadia-client -j4
cd libraries/vircadia-client
cmake --build . --target install/strip
```
With these options the only direct dependencies will be QtCore, QtScript, QtNetwork and OpenSSL libraries, but dependent on the system some other indirect dependencies might need to be included in the package as well.



