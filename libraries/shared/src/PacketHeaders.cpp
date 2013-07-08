//
//  PacketHeaders.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/28/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "PacketHeaders.h"

PACKET_VERSION versionForPacketType(PACKET_TYPE type) {
    switch (type) {
        default:
            return 0;
            break;
    }
}

int populateTypeAndVersion(unsigned char* destination, PACKET_TYPE type) {
    destination[0] = type;
    destination[1] = versionForPacketType(type);
    
    // return the number of bytes written for pointer pushing
    return 2;
}