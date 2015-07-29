//
//  PacketTests.h
//  tests/networking/src
//
//  Created by Stephen Birarda on 07/14/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketTests_h
#define hifi_PacketTests_h

#pragma once

#include <QtTest/QtTest>

class PacketTests : public QObject {
    Q_OBJECT
private slots:
    // Test the base state of Packet
    void emptyPacketTest();

    // Test Packet writes
    void writeTest();

    // Test Packet reads
    void readTest();

    // Test Packet writing past capacity
    void writePastCapacityTest();

    // Test primitive reads and writes
    void primitiveTest();

    // Test set/get packet type
    void packetTypeTest();
};

#endif // hifi_PacketTests_h
