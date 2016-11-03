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

    static const int UDP_IPV4_HEADER_SIZE = 28;
    static const int MAX_PACKET_SIZE_WITH_UDP_HEADER = 1492;
    static const int MAX_PACKET_SIZE = MAX_PACKET_SIZE_WITH_UDP_HEADER - UDP_IPV4_HEADER_SIZE;
    static const int MAX_PACKETS_IN_FLIGHT = 25600;
    static const int CONNECTION_RECEIVE_BUFFER_SIZE_PACKETS = 8192;
    static const int CONNECTION_SEND_BUFFER_SIZE_PACKETS = 8192;
    static const int UDP_SEND_BUFFER_SIZE_BYTES = 1048576;
    static const int UDP_RECEIVE_BUFFER_SIZE_BYTES = 1048576;
    static const int DEFAULT_SYN_INTERVAL_USECS = 10 * 1000;

    
    // Header constants

    // Bit sizes (in order)
    static const int CONTROL_BIT_SIZE = 1;
    static const int RELIABILITY_BIT_SIZE = 1;
    static const int MESSAGE_BIT_SIZE = 1;
    static const int OBFUSCATION_LEVEL_SIZE = 2;
    static const int SEQUENCE_NUMBER_SIZE= 27;

    static const int PACKET_POSITION_SIZE = 2;
    static const int MESSAGE_NUMBER_SIZE = 30;

    static const int MESSAGE_PART_NUMBER_SIZE = 32;

    // Offsets
    static const int SEQUENCE_NUMBER_OFFSET = 0;
    static const int OBFUSCATION_LEVEL_OFFSET = SEQUENCE_NUMBER_OFFSET + SEQUENCE_NUMBER_SIZE;
    static const int MESSAGE_BIT_OFFSET = OBFUSCATION_LEVEL_OFFSET + OBFUSCATION_LEVEL_SIZE;
    static const int RELIABILITY_BIT_OFFSET = MESSAGE_BIT_OFFSET + MESSAGE_BIT_SIZE;
    static const int CONTROL_BIT_OFFSET = RELIABILITY_BIT_OFFSET + RELIABILITY_BIT_SIZE;

    static const int MESSAGE_NUMBER_OFFSET = 0;
    static const int PACKET_POSITION_OFFSET = MESSAGE_NUMBER_OFFSET + MESSAGE_NUMBER_SIZE;

    static const int MESSAGE_PART_NUMBER_OFFSET = 0;

    // Masks
    static const uint32_t CONTROL_BIT_MASK = uint32_t(1) << CONTROL_BIT_OFFSET;
    static const uint32_t RELIABILITY_BIT_MASK = uint32_t(1) << RELIABILITY_BIT_OFFSET;
    static const uint32_t MESSAGE_BIT_MASK = uint32_t(1) << MESSAGE_BIT_OFFSET;
    static const uint32_t OBFUSCATION_LEVEL_MASK = uint32_t(3) << OBFUSCATION_LEVEL_OFFSET;
    static const uint32_t BIT_FIELD_MASK = CONTROL_BIT_MASK | RELIABILITY_BIT_MASK | MESSAGE_BIT_MASK | OBFUSCATION_LEVEL_MASK;
    static const uint32_t SEQUENCE_NUMBER_MASK = ~BIT_FIELD_MASK;

    static const uint32_t PACKET_POSITION_MASK = uint32_t(3) << PACKET_POSITION_OFFSET;
    static const uint32_t MESSAGE_NUMBER_MASK = ~PACKET_POSITION_MASK;

    static const uint32_t MESSAGE_PART_NUMBER_MASK = ~uint32_t(0);


    // Static checks
    static_assert(CONTROL_BIT_SIZE + RELIABILITY_BIT_SIZE + MESSAGE_BIT_SIZE +
                  OBFUSCATION_LEVEL_SIZE + SEQUENCE_NUMBER_SIZE == 32, "Sequence number line size incorrect");
    static_assert(PACKET_POSITION_SIZE + MESSAGE_NUMBER_SIZE == 32, "Message number line size incorrect");
    static_assert(MESSAGE_PART_NUMBER_SIZE  == 32, "Message part number line size incorrect");

    static_assert(CONTROL_BIT_MASK == 0x80000000, "CONTROL_BIT_MASK incorrect");
    static_assert(RELIABILITY_BIT_MASK == 0x40000000, "RELIABILITY_BIT_MASK incorrect");
    static_assert(MESSAGE_BIT_MASK == 0x20000000, "MESSAGE_BIT_MASK incorrect");
    static_assert(OBFUSCATION_LEVEL_MASK == 0x18000000, "OBFUSCATION_LEVEL_MASK incorrect");
    static_assert(BIT_FIELD_MASK == 0xF8000000, "BIT_FIELD_MASK incorrect");
    static_assert(SEQUENCE_NUMBER_MASK == 0x07FFFFFF, "SEQUENCE_NUMBER_MASK incorrect");

    static_assert(PACKET_POSITION_MASK == 0xC0000000, "PACKET_POSITION_MASK incorrect");
    static_assert(MESSAGE_NUMBER_MASK == 0x3FFFFFFF, "MESSAGE_NUMBER_MASK incorrect");

    static_assert(MESSAGE_PART_NUMBER_MASK == 0xFFFFFFFF, "MESSAGE_PART_NUMBER_MASK incorrect");
}

#endif // hifi_udt_Constants_h
