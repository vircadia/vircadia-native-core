//
//  SaltShaker.h
//  libraries/networking/src/udt
//
//  Created by Clement on 2/18/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SaltShaker_h
#define hifi_SaltShaker_h

#include "Packet.h"

namespace udt {

class SaltShaker {
public:
    std::unique_ptr<Packet> salt(const Packet& packet, unsigned int saltiness);
    void unsalt(Packet& packet, unsigned int saltiness);
};

}

#endif // hifi_SaltShaker_h