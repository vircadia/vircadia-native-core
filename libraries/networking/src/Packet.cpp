//
//  Packet.cpp
//
//
//  Created by Clement on 7/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Packet.h"

#include <QByteArray>

std::unique_ptr<Packet> Packet::makePacket() {
    return std::unique_ptr<Packet>(new Packet());
}

QByteArray Packet::payload() {
    return QByteArray();
}