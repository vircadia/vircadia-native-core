//
//  MetavoxelTests.cpp
//  metavoxel-tests
//
//  Created by Andrzej Kapolka on 2/7/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <stdlib.h>

#include <DatagramSequencer.h>
#include <SharedUtil.h>

#include "MetavoxelTests.h"

MetavoxelTests::MetavoxelTests(int& argc, char** argv) :
    QCoreApplication(argc, argv) {
}

static int highPriorityMessagesSent = 0;
static int unreliableMessagesSent = 0;
static int unreliableMessagesReceived = 0;

bool MetavoxelTests::run() {
    
    qDebug() << "Running metavoxel tests...";
    
    // seed the random number generator so that our tests are reproducible
    srand(0xBAAAAABE);

    // create two endpoints with the same header
    QByteArray datagramHeader("testheader");
    Endpoint alice(datagramHeader), bob(datagramHeader);
    
    alice.setOther(&bob);
    bob.setOther(&alice);
    
    // perform a large number of simulation iterations
    const int SIMULATION_ITERATIONS = 100000;
    for (int i = 0; i < SIMULATION_ITERATIONS; i++) {
        if (alice.simulate(i) || bob.simulate(i)) {
            return true;
        }
    }
    
    qDebug() << "Sent " << highPriorityMessagesSent << " high priority messages";
    
    qDebug() << "Sent " << unreliableMessagesSent << " unreliable messages, received " << unreliableMessagesReceived;
    
    qDebug() << "All tests passed!";
    
    return false;
}

Endpoint::Endpoint(const QByteArray& datagramHeader) :
    _sequencer(new DatagramSequencer(datagramHeader)),
    _highPriorityMessagesToSend(0.0f) {
    
    connect(_sequencer, SIGNAL(readyToWrite(const QByteArray&)), SLOT(sendDatagram(const QByteArray&)));
    connect(_sequencer, SIGNAL(readyToRead(Bitstream&)), SLOT(readMessage(Bitstream&)));
    connect(_sequencer, SIGNAL(receivedHighPriorityMessage(const QVariant&)),
        SLOT(handleHighPriorityMessage(const QVariant&)));
}

static QByteArray createRandomBytes() {
    const int MIN_BYTES = 4;
    const int MAX_BYTES = 16;
    QByteArray bytes(randIntInRange(MIN_BYTES, MAX_BYTES), 0);
    for (int i = 0; i < bytes.size(); i++) {
        bytes[i] = rand();
    }
    return bytes;
}

static QVariant createRandomMessage() {
    switch (randIntInRange(0, 2)) {
        case 0: {
            TestMessageA message = { randomBoolean(), rand(), randFloat() };
            return QVariant::fromValue(message);
        }
        case 1: {
            TestMessageB message = { createRandomBytes() };
            return QVariant::fromValue(message); 
        }
        case 2:
        default: {
            TestMessageC message;
            message.foo = randomBoolean();
            message.bar = rand();
            message.baz = randFloat();
            message.bong.foo = createRandomBytes();
            return QVariant::fromValue(message);
        }
    }
}

static bool messagesEqual(const QVariant& firstMessage, const QVariant& secondMessage) {
    int type = firstMessage.userType();
    if (secondMessage.userType() != type) {
        return false;
    }
    if (type == TestMessageA::Type) {
        return firstMessage.value<TestMessageA>() == secondMessage.value<TestMessageA>();
    } else if (type == TestMessageB::Type) {
        return firstMessage.value<TestMessageB>() == secondMessage.value<TestMessageB>();
    } else if (type == TestMessageC::Type) {
        return firstMessage.value<TestMessageC>() == secondMessage.value<TestMessageC>();
    } else {
        return firstMessage == secondMessage;
    }
}

bool Endpoint::simulate(int iterationNumber) {
    // enqueue some number of high priority messages
    const float MIN_HIGH_PRIORITY_MESSAGES = 0.0f;
    const float MAX_HIGH_PRIORITY_MESSAGES = 2.0f;
    _highPriorityMessagesToSend += randFloatInRange(MIN_HIGH_PRIORITY_MESSAGES, MAX_HIGH_PRIORITY_MESSAGES);   
    while (_highPriorityMessagesToSend >= 1.0f) {
        QVariant message = createRandomMessage();
        _highPriorityMessagesSent.append(message);
        _sequencer->sendHighPriorityMessage(message);
        highPriorityMessagesSent++;
        _highPriorityMessagesToSend -= 1.0f;
    }
    
    // send a packet
    try {
        Bitstream& out = _sequencer->startPacket();
        SequencedTestMessage message = { iterationNumber, createRandomMessage() };
        _unreliableMessagesSent.append(message);
        unreliableMessagesSent++;
        out << message;
        _sequencer->endPacket();
    
    } catch (const QString& message) {
        qDebug() << message;
        return true;
    }
    
    return false;
}

void Endpoint::sendDatagram(const QByteArray& datagram) {
    // some datagrams are dropped
    const float DROP_PROBABILITY = 0.1f;
    if (randFloat() < DROP_PROBABILITY) {
        return;
    }
    _other->_sequencer->receivedDatagram(datagram);
}

void Endpoint::handleHighPriorityMessage(const QVariant& message) {
    if (_other->_highPriorityMessagesSent.isEmpty()) {
        throw QString("Received unsent/already sent high priority message.");
    }
    QVariant sentMessage = _other->_highPriorityMessagesSent.takeFirst();
    if (!messagesEqual(message, sentMessage)) {
        throw QString("Sent/received high priority message mismatch.");
    }
}

void Endpoint::readMessage(Bitstream& in) {
    SequencedTestMessage message;
    in >> message;
    
    for (QList<SequencedTestMessage>::iterator it = _other->_unreliableMessagesSent.begin();
            it != _other->_unreliableMessagesSent.end(); it++) {
        if (it->sequenceNumber == message.sequenceNumber) {
            if (!messagesEqual(it->submessage, message.submessage)) {
                throw QString("Sent/received unreliable message mismatch.");
            }
            _other->_unreliableMessagesSent.erase(_other->_unreliableMessagesSent.begin(), it + 1);
            unreliableMessagesReceived++;
            return;
        }
    }
    throw QString("Received unsent/already sent unreliable message.");
}
