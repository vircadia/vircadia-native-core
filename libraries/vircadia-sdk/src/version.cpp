//
//  version.cpp
//  libraries/vircadia_sdk/src
//
//  Created by Nshan G. on 18 Feb 2022.
//  Copyright 2022 Vircadia contributors.
//

#include "version.h"

#define STRINGIFY2( x) #x
#define STRINGIFY(x) STRINGIFY2(x)

VIRCADIA_SDK_DYN_API
const vircadia_sdk_version_data* vircadia_sdk_version() {
    static const vircadia_sdk_version_data data {
        VIRCADIA_SDK_VERSION_MAJOR,
        VIRCADIA_SDK_VERSION_MINOR,
        VIRCADIA_SDK_VERSION_TWEAK,
        STRINGIFY(VIRCADIA_SDK_GIT_COMMIT_SHA_SHORT),
        STRINGIFY(VIRCADIA_SDK_VERSION),
        STRINGIFY(VIRCADIA_SDK_VERSION_FULL)
    };
    return &data;
}
