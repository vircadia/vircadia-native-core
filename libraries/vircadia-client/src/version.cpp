//
//  version.cpp
//  libraries/client/src
//
//  Created by Nshan G. on 18 Feb 2022.
//  Copyright 2022 Vircadia contributors.
//

#include "version.h"

#define STRINGIFY2( x) #x
#define STRINGIFY(x) STRINGIFY2(x)

VIRCADIA_CLIENT_DYN_API
const vircadia_client_version_data* vircadia_client_version() {
    static const vircadia_client_version_data data {
        VIRCADIA_CLIENT_VERSION_MAJOR,
        VIRCADIA_CLIENT_VERSION_MINOR,
        VIRCADIA_CLIENT_VERSION_TWEAK,
        STRINGIFY(VIRCADIA_CLIENT_GIT_COMMIT_SHA_SHORT),
        STRINGIFY(VIRCADIA_CLIENT_VERSION),
        STRINGIFY(VIRCADIA_CLIENT_VERSION_FULL)
    };
    return &data;
}
