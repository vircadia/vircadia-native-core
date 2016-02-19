//
//  SaltShaker.cpp
//  libraries/networking/src/udt
//
//  Created by Clement on 2/18/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SaltShaker.h"

#include <array>

using namespace udt;

using Key = uint64_t;
static const std::array<Key, 4> KEYS {{
    0x0,
    0xd6ea42f07644016a,
    0x700f7e3414dc4d8c,
    0x54c92e8d2c871642
}};

void saltingHelper(char* start, int size, Key key) {;
    const auto end = start + size;

    auto p = start;
    for (; p < end; p += sizeof(Key)) {
        *reinterpret_cast<Key*>(p) ^= key;
    }

    p -= sizeof(Key);

    for (int i = 0; p < end; ++p || ++i) {
        *p ^= *(reinterpret_cast<const char*>(&key) + i);
    }
}

std::unique_ptr<Packet> SaltShaker::salt(const Packet& packet, unsigned int saltiness) {
    Q_ASSERT_X(saltiness < KEYS.size(), Q_FUNC_INFO, "");

    auto copy = Packet::createCopy(packet);
    saltingHelper(copy->getPayload(), copy->getPayloadSize(), KEYS[saltiness]);
    copy->writeSequenceNumber(copy->getSequenceNumber(), (Packet::ObfuscationLevel)saltiness);
    return copy;
}

void unsalt(Packet& packet, unsigned int saltiness) {
    Q_ASSERT_X(saltiness < KEYS.size(), Q_FUNC_INFO, "");

    saltingHelper(packet.getPayload(), packet.getPayloadSize(), KEYS[saltiness]);
}