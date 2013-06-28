//
//  PacketHeaders.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/28/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "PacketHeaders.h"

PACKET_VERSION packetVersion(PACKET_HEADER header) {
    switch (header) {
        default:
            return 0;
            break;
    }
}