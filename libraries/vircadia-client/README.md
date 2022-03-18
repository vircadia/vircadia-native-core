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
