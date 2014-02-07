//
//  PacketHeaders.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/28/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <math.h>

#include <QtCore/QDebug>

#include "NodeList.h"

#include "PacketHeaders.h"

int arithmeticCodingValueFromBuffer(const char* checkValue) {
    if (((uchar) *checkValue) < 255) {
        return *checkValue;
    } else {
        return 255 + arithmeticCodingValueFromBuffer(checkValue + 1);
    }
}

int numBytesArithmeticCodingFromBuffer(const char* checkValue) {
    if (((uchar) *checkValue) < 255) {
        return 1;
    } else {
        return 1 + numBytesArithmeticCodingFromBuffer(checkValue + 1);
    }
}

int packArithmeticallyCodedValue(int value, char* destination) {
    if (value < 255) {
        // less than 255, just pack our value
        destination[0] = (uchar) value;
        return 1;
    } else {
        // pack 255 and then recursively pack on
        ((unsigned char*)destination)[0] = 255;
        return 1 + packArithmeticallyCodedValue(value - 255, destination + 1);
    }
}

PacketVersion versionForPacketType(PacketType type) {
    switch (type) {
        case PacketTypeParticleData:
            return 1;
        default:
            return 0;
    }
}

QByteArray byteArrayWithPopluatedHeader(PacketType type, const QUuid& connectionUUID) {
    QByteArray freshByteArray(MAX_PACKET_HEADER_BYTES, 0);
    freshByteArray.resize(populatePacketHeader(freshByteArray, type, connectionUUID));
    return freshByteArray;
}

int populatePacketHeader(QByteArray& packet, PacketType type, const QUuid& connectionUUID) {
    if (packet.size() < numBytesForPacketHeaderGivenPacketType(type)) {
        packet.resize(numBytesForPacketHeaderGivenPacketType(type));
    }
    
    return populatePacketHeader(packet.data(), type, connectionUUID);
}

int populatePacketHeader(char* packet, PacketType type, const QUuid& connectionUUID) {
    int numTypeBytes = packArithmeticallyCodedValue(type, packet);
    packet[numTypeBytes] = versionForPacketType(type);
    
    char* position = packet + numTypeBytes + sizeof(PacketVersion);
    
    QUuid packUUID = connectionUUID.isNull() ? NodeList::getInstance()->getSessionUUID() : connectionUUID;
    
    QByteArray rfcUUID = packUUID.toRfc4122();
    memcpy(position, rfcUUID.constData(), NUM_BYTES_RFC4122_UUID);
    position += NUM_BYTES_RFC4122_UUID;
    
    // pack 16 bytes of zeros where the md5 hash will be placed one data is packed
    memset(position, 0, NUM_BYTES_MD5_HASH);
    position += NUM_BYTES_MD5_HASH;
    
    // return the number of bytes written for pointer pushing
    return position - packet;
}

bool packetVersionMatch(const QByteArray& packet) {
    // currently this just checks if the version in the packet matches our return from versionForPacketType
    // may need to be expanded in the future for types and versions that take > than 1 byte
    
    if (packet[1] == versionForPacketType(packetTypeForPacket(packet)) || packetTypeForPacket(packet) == PacketTypeStunResponse) {
        return true;
    } else {
        PacketType mismatchType = packetTypeForPacket(packet);
        int numPacketTypeBytes = arithmeticCodingValueFromBuffer(packet.data());
       
        QUuid nodeUUID = uuidFromPacketHeader(packet);
        
        qDebug() << "Packet mismatch on" << packetTypeForPacket(packet) << "- Sender"
            << nodeUUID << "sent" << qPrintable(QString::number(packet[numPacketTypeBytes])) << "but"
            << qPrintable(QString::number(versionForPacketType(mismatchType))) << "expected.";

        return false;
    }
}

int numBytesForPacketHeader(const QByteArray& packet) {
    // returns the number of bytes used for the type, version, and UUID
    return numBytesArithmeticCodingFromBuffer(packet.data()) + NUM_STATIC_HEADER_BYTES;
}

int numBytesForPacketHeader(const char* packet) {
    // returns the number of bytes used for the type, version, and UUID
    return numBytesArithmeticCodingFromBuffer(packet) + NUM_STATIC_HEADER_BYTES;
}

int numBytesForPacketHeaderGivenPacketType(PacketType type) {
    return (int) ceilf((float)type / 255) + NUM_STATIC_HEADER_BYTES;
}

QUuid uuidFromPacketHeader(const QByteArray& packet) {
    return QUuid::fromRfc4122(packet.mid(numBytesArithmeticCodingFromBuffer(packet.data()) + sizeof(PacketVersion),
                                         NUM_BYTES_RFC4122_UUID));
}

PacketType packetTypeForPacket(const QByteArray& packet) {
    return (PacketType) arithmeticCodingValueFromBuffer(packet.data());
}

PacketType packetTypeForPacket(const char* packet) {
    return (PacketType) arithmeticCodingValueFromBuffer(packet);
}
