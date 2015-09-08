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

#include "SequenceNumber.h"

namespace udt {
    static const int MAX_PACKET_SIZE_WITH_UDP_HEADER = 1500;
    static const int MAX_PACKET_SIZE = MAX_PACKET_SIZE_WITH_UDP_HEADER - 28;
    static const int MAX_PACKETS_IN_FLIGHT = 25600;
    static const int CONNECTION_RECEIVE_BUFFER_SIZE_PACKETS = 8192;
    static const int CONNECTION_SEND_BUFFER_SIZE_PACKETS = 8192;
    static const int UDP_SEND_BUFFER_SIZE_BYTES = 1048576;
    static const int UDP_RECEIVE_BUFFER_SIZE_BYTES = 1048576;
    static const int DEFAULT_SYN_INTERVAL_USECS = 10 * 1000;
    static const int SEQUENCE_NUMBER_BITS = sizeof(SequenceNumber) * 8;
    static const int MESSAGE_LINE_NUMBER_BITS = 32;
    static const int MESSAGE_NUMBER_BITS = 30;
    static const uint32_t CONTROL_BIT_MASK = uint32_t(1) << (SEQUENCE_NUMBER_BITS - 1);
}

#endif // hifi_udt_Constants_h
