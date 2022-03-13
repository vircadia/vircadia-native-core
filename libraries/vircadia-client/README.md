### Client API

This is C++ library for Vircadia client->server communication used internally and wrapper C API for various language bindings.

### Building

The library is build as a dependency of the interface, but if you are just working on the API you can build it by itself with the following command in the build directory:
```
cmake --build . --target vircadia-client
```


To build the unit tests:
```
cmake --build . --target vircadia-client-tests
```
The tests can be run with `ctest`, or directly by executing `build/libraries/vircadia-client/tests/vircadia-client-tests` for more detailed output.


To build the documentation (requires doxygen):
```
cmake --build . --target vircadia-client-tests
```
