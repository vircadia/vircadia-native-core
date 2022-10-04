//
//  Common.h
//  libraries/vircadia-client/src/internal
//
//  Created by Nshan G. on 16 Apr 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_COMMON_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_COMMON_H

#include <array>
#include <cstdint>

#include <UUID.h>

class QUuid;

namespace vircadia::client {

    using UUID = std::array<uint8_t, NUM_BYTES_RFC4122_UUID>;

    UUID toUUIDArray(const QUuid& from);
    QUuid qUuidfromBytes(const uint8_t* bytes);

} // namespace vircadia::client

#endif /* end of include guard */
