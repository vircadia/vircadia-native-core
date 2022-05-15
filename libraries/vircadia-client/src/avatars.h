//
//  avatars.h
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 16 May 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_AVATARS_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_AVATARS_H

#include <stdint.h>

#include "common.h"

VIRCADIA_CLIENT_DYN_API
int vircadia_enable_avatars(int context_id);

VIRCADIA_CLIENT_DYN_API
int vircadia_update_avatars(int context_id);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_display_name(int context_id, const char* display_name);

#endif /* end of include guard */
