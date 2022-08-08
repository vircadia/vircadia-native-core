### Summary

C API/ABI documentation for [Vircadia Client Library](https://github.com/vircadia/vircadia/blob/unity-sdk/libraries/vircadia-client). The main library id divided into several headers that each provide APIs to specific component:
* context.h API for initialization of the library context/state that is used by all other components.
* node_list.h API for connecting to a given server and enumerating nodes.
* messages.h API for communicating with the message mixer node.
* avatars.h API for communicating with the avatar mixer node.
* audio.h API for communicating with the audio mixer node.
* client.h The main library header that included all components.

