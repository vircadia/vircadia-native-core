//
//  version.h
//  libraries/vircadia_sdk/src
//
//  Created by Nshan G. on 18 Feb 2022.
//  Copyright 2022 Vircadia contributors.
//

#ifndef VIRCADIA_LIBRARIES_CLIENT_SDK_SRC_VERSION_H
#define VIRCADIA_LIBRARIES_CLIENT_SDK_SRC_VERSION_H

#if defined(_MSC_VER)
#ifdef VIRCADIA_SDK_DLL_EXPORT
/* We are building this library */
#   define VIRCADIA_SDK_DYN_API extern "C" __declspec(dllexport)
#else
/* We are using this library */
#   define VIRCADIA_SDK_DYN_API  extern "C" __declspec(dllimport)
#endif
#else
    #define VIRCADIA_SDK_DYN_API extern "C"
#endif

struct vircadia_sdk_version_data {
    int major;
    int minor;
    int tweak;
    const char* commit;
    const char* number;
    const char* full;
};

VIRCADIA_SDK_DYN_API
const vircadia_sdk_version_data* vircadia_sdk_version();

#endif /* end of include guard */
