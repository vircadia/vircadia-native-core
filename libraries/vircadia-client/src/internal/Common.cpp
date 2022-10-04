//
//  Common.cpp
//  libraries/vircadia-client/src/internal
//
//  Created by Nshan G. on 16 Apr 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Common.h"

#include <QUuid>

namespace vircadia::client
{

    UUID toUUIDArray(const QUuid& from) {
        UUID to{};
        auto rfc4122 = from.toRfc4122();
        assert(rfc4122.size() == to.size());
        std::copy(rfc4122.begin(), rfc4122.end(), to.begin());
        return to;
    }

    QUuid qUuidfromBytes(const uint8_t* bytes) {
        return QUuid::fromRfc4122(QByteArray(
            reinterpret_cast<const char*>(bytes), NUM_BYTES_RFC4122_UUID));
    }

} // namespace vircadia::client
