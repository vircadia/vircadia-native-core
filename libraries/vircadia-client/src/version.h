//
//  version.h
//  libraries/client/src
//
//  Created by Nshan G. on 18 Feb 2022.
//  Copyright 2022 Vircadia contributors.
//

/** \file */

#ifndef VIRCADIA_LIBRARIES_CLIENT_SRC_VERSION_H
#define VIRCADIA_LIBRARIES_CLIENT_SRC_VERSION_H

#include "common.h"

/**
 * \brief Defines the layout of version information returned by vircadia_client_version().
 */
struct vircadia_client_version_data {

    /** \brief The major version number.
     *
     * Increments on compatibility breaking changes.
     */
    int major;

    /** \brief The minor version number.
     *
     * Increments on backwards compatible API addition or major implementation changes.
     */
    int minor;

    /** \brief The tweak version number.
     *
     * Increments on bug fixes and minor implementation changes.
     */
    int tweak;

    /** \brief Short VCS identifier. */
    const char* commit;

    /** \brief Formatted version number string.
     *
     * Contains vircadia_client_version_data::major, vircadia_client_version_data::minor,
     * and vircadia_client_version_data::tweak.
     */
    const char* number;


    /** \brief Formatted version string.
     *
     * Contains vircadia_client_version_data::major, vircadia_client_version_data::minor,
     * vircadia_client_version_data::tweak, and vircadia_client_version_data::commit.
     */
    const char* full;
};

/**
 * \brief API version information.
 * \return A pointer to static version information struct.
 */
VIRCADIA_CLIENT_DYN_API
const vircadia_client_version_data* vircadia_client_version();

#endif /* end of include guard */
