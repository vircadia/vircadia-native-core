//
//  PacketListener.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 07/14/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PacketListener.h"

#include "NodeList.h"

PacketListener::~PacketListener() {
    auto limitedNodelist = DependencyManager::get<LimitedNodeList>();
    if (limitedNodelist) {
        limitedNodelist->getPacketReceiver().unregisterListener(this);
    }
}
