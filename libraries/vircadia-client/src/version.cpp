//
//  version.cpp
//  libraries/client/src
//
//  Created by Nshan G. on 18 Feb 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "version.h"

#define STRINGIFY2( x) #x
#define STRINGIFY(x) STRINGIFY2(x)

VIRCADIA_CLIENT_DYN_API
const vircadia_client_version_data* vircadia_client_version() {
    static const vircadia_client_version_data data {
        VIRCADIA_CLIENT_VERSION_YEAR,
        VIRCADIA_CLIENT_VERSION_MAJOR,
        VIRCADIA_CLIENT_VERSION_MINOR,
        STRINGIFY(VIRCADIA_CLIENT_GIT_COMMIT_SHA_SHORT),
        STRINGIFY(VIRCADIA_CLIENT_VERSION),
        STRINGIFY(VIRCADIA_CLIENT_VERSION_FULL)
    };
    return &data;
}
