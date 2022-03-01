//
//  version.h
//  libraries/client/src
//
//  Created by Nshan G. on 18 Feb 2022.
//  Copyright 2022 Vircadia contributors.
//

#ifndef VIRCADIA_LIBRARIES_CLIENT_SRC_VERSION_H
#define VIRCADIA_LIBRARIES_CLIENT_SRC_VERSION_H

#include "common.h"

struct vircadia_client_version_data {
    int major;
    int minor;
    int tweak;
    const char* commit;
    const char* number;
    const char* full;
};

VIRCADIA_CLIENT_DYN_API
const vircadia_client_version_data* vircadia_client_version();

#endif /* end of include guard */
