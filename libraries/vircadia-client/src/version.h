//
//  version.h
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 18 Feb 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef VIRCADIA_LIBRARIES_CLIENT_SRC_VERSION_H
#define VIRCADIA_LIBRARIES_CLIENT_SRC_VERSION_H

#include "common.h"

/// @brief Defines the layout of version information returned by vircadia_client_version().
struct vircadia_client_version_data {

    /// @brief The year of the release.
    ///
    /// Updated with the first release of the year, either minor or major.
    int year;

    /// @brief The major version number.
    ///
    /// Increments with API breaking changes.
    int major;

    /// @brief The minor version number.
    ///
    /// Increments with backward compatible API changes/additions, bug fixes and implementation changes.
    int minor;

    /// @brief Short VCS identifier.
    const char* commit;

    /// @brief Formatted version number string.
    ///
    /// Contains vircadia_client_version_data::major and vircadia_client_version_data::minor
    const char* number;

    /// @brief Formatted version string.
    ///
    /// Contains vircadia_client_version_data::year, vircadia_client_version_data::major,
    /// vircadia_client_version_data::minor, and vircadia_client_version_data::commit.
    const char* full;
};

/// @brief API version information.
//
/// @return A pointer to static version information struct.
VIRCADIA_CLIENT_DYN_API
const vircadia_client_version_data* vircadia_client_version();

#endif /* end of include guard */
