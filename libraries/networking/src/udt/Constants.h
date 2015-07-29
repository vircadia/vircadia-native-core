//
//  Constants.h
//  libraries/networking/src/udt
//
//  Created by Clement on 7/13/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_udt_Constants_h
#define hifi_udt_Constants_h

namespace udt {
    static const int MAX_PACKET_SIZE = 1450;
    static const int MAX_PACKETS_IN_FLIGHT = 25600;
    static const int CONNECTION_RECEIVE_BUFFER_SIZE_PACKETS = 8192;
    static const int CONNECTION_SEND_BUFFER_SIZE_PACKETS = 8192;
    static const int UDP_SEND_BUFFER_SIZE_BYTES = 1048576;
    static const int UDP_RECEIVE_BUFFER_SIZE_BYTES = 1048576;
}

#endif // hifi_udt_Constants_h
