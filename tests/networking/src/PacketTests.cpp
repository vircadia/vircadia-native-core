//
//  PacketTests.cpp
//  tests/networking/src
//
//  Created by Stephen Birarda on 07/14/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PacketTests.h"
#include "../QTestExtensions.h"

#include <NLPacket.h>

QTEST_MAIN(PacketTests)

std::unique_ptr<NLPacket> copyToReadPacket(std::unique_ptr<NLPacket>& packet) {
    auto size = packet->getDataSize();
    auto data = std::unique_ptr<char[]>(new char[size]);
    memcpy(data.get(), packet->getData(), size);
    return NLPacket::fromReceivedPacket(std::move(data), size, HifiSockAddr());
}

void PacketTests::emptyPacketTest() {
    auto packet = NLPacket::create(PacketType::Unknown);

    QCOMPARE(packet->getType(), PacketType::Unknown);
    QCOMPARE(packet->getPayloadSize(), 0);
    QCOMPARE(packet->getDataSize(), NLPacket::totalHeaderSize(packet->getType()));
    QCOMPARE(packet->bytesLeftToRead(), 0);
    QCOMPARE(packet->bytesAvailableForWrite(), packet->getPayloadCapacity());
}

void PacketTests::packetTypeTest() {
    auto packet = NLPacket::create(PacketType::EntityAdd);

    QCOMPARE(packet->getType(), PacketType::EntityAdd);

    packet->setType(PacketType::MixedAudio);
    QCOMPARE(packet->getType(), PacketType::MixedAudio);

    packet->setType(PacketType::MuteEnvironment);
    QCOMPARE(packet->getType(), PacketType::MuteEnvironment);
}

void PacketTests::writeTest() {
    auto packet = NLPacket::create(PacketType::Unknown);

    QCOMPARE(packet->getPayloadSize(), 0);

    QCOMPARE(packet->write("somedata"), 8);
    QCOMPARE(packet->getPayloadSize(), 8);

    QCOMPARE(packet->write("moredata"), 8);
    QCOMPARE(packet->getPayloadSize(), 16);

    COMPARE_DATA(packet->getPayload(), "somedatamoredata", 16);
}

void PacketTests::readTest() {
    // Test reads for several different size packets
    for (int i = 1; i < 4; i++) {
        auto packet = NLPacket::create(PacketType::Unknown);

        auto size = packet->getPayloadCapacity();
        size /= i;

        char* data = new char[size];

        // Fill with "random" data
        for (qint64 i = 0; i < size; i++) {
            data[i] = i;
        }

        QCOMPARE(packet->write(data, size), size);

        auto readPacket = copyToReadPacket(packet);

        QCOMPARE(readPacket->getDataSize(), packet->getDataSize());
        QCOMPARE(readPacket->getPayloadSize(), packet->getPayloadSize());
        COMPARE_DATA(readPacket->getData(), packet->getData(), packet->getDataSize());
        COMPARE_DATA(readPacket->getPayload(), packet->getPayload(), packet->getPayloadSize());

        auto payloadSize = readPacket->getPayloadSize();
        char* readData = new char[payloadSize];
        QCOMPARE(readPacket->read(readData, payloadSize), payloadSize);
        COMPARE_DATA(readData, data, payloadSize);
    }
}

void PacketTests::writePastCapacityTest() {
    auto packet = NLPacket::create(PacketType::Unknown);

    auto size = packet->getPayloadCapacity();
    char* data = new char[size];

    // Fill with "random" data
    for (qint64 i = 0; i < size; i++) {
        data[i] = i;
    }

    QCOMPARE(packet->getPayloadSize(), 0);

    QCOMPARE(packet->write(data, size), size);

    QCOMPARE(packet->getPayloadSize(), size);
    COMPARE_DATA(packet->getPayload(), data, size);

    QCOMPARE(packet->bytesAvailableForWrite(), 0);
    QCOMPARE(packet->getPayloadSize(), size);

    QCOMPARE(NLPacket::PACKET_WRITE_ERROR, packet->write("data")); // asserts in DEBUG

    // NLPacket::write() shouldn't allow the caller to write if no space is left
    QCOMPARE(packet->getPayloadSize(), size);
}

void PacketTests::primitiveTest() {
    auto packet = NLPacket::create(PacketType::Unknown);

    int value1 = 5;
    char value2 = 10;
    bool value3 = true;
    qint64 value4 = -93404;

    packet->writePrimitive(value1);
    packet->writePrimitive(value2);
    packet->writePrimitive(value3);
    packet->writePrimitive(value4);

    auto recvPacket = copyToReadPacket(packet);

    // Peek & read first value
    {
        int peekValue = 0;
        QCOMPARE(recvPacket->peekPrimitive(&peekValue), (int)sizeof(peekValue));
        QCOMPARE(peekValue, value1);
        int readValue = 0;
        QCOMPARE(recvPacket->readPrimitive(&readValue), (int)sizeof(readValue));
        QCOMPARE(readValue, value1);
    }

    // Peek & read second value
    {
        char peekValue = 0;
        QCOMPARE(recvPacket->peekPrimitive(&peekValue), (int)sizeof(peekValue));
        QCOMPARE(peekValue, value2);

        char readValue = 0;
        QCOMPARE(recvPacket->readPrimitive(&readValue), (int)sizeof(readValue));
        QCOMPARE(readValue, value2);
    }

    // Peek & read third value
    {
        bool peekValue = 0;
        QCOMPARE(recvPacket->peekPrimitive(&peekValue), (int)sizeof(peekValue));
        QCOMPARE(peekValue, value3);

        bool readValue = 0;
        QCOMPARE(recvPacket->readPrimitive(&readValue), (int)sizeof(readValue));
        QCOMPARE(readValue, value3);
    }

    // Peek & read fourth value
    {
        qint64 peekValue = 0;
        QCOMPARE(recvPacket->peekPrimitive(&peekValue), (int)sizeof(peekValue));
        QCOMPARE(peekValue, value4);

        qint64 readValue = 0;
        QCOMPARE(recvPacket->readPrimitive(&readValue), (int)sizeof(readValue));
        QCOMPARE(readValue, value4);
    }

    // We should be at the end of the packet, with no more to peek or read
    int noValue;
    QCOMPARE(recvPacket->peekPrimitive(&noValue), 0);
    QCOMPARE(recvPacket->readPrimitive(&noValue), 0);
}
