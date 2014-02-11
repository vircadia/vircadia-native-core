//
//  MetavoxelTests.cpp
//  metavoxel-tests
//
//  Created by Andrzej Kapolka on 2/7/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <stdlib.h>

#include <SharedUtil.h>

#include "MetavoxelTests.h"

MetavoxelTests::MetavoxelTests(int& argc, char** argv) :
    QCoreApplication(argc, argv) {
}

static int datagramsSent = 0;
static int datagramsReceived = 0;
static int highPriorityMessagesSent = 0;
static int highPriorityMessagesReceived = 0;
static int unreliableMessagesSent = 0;
static int unreliableMessagesReceived = 0;
static int streamedBytesSent = 0;
static int streamedBytesReceived = 0;
static int lowPriorityStreamedBytesSent = 0;
static int lowPriorityStreamedBytesReceived = 0;

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
    
    qDebug() << "Sent" << highPriorityMessagesSent << "high priority messages, received" << highPriorityMessagesReceived;
    qDebug() << "Sent" << unreliableMessagesSent << "unreliable messages, received" << unreliableMessagesReceived;
    qDebug() << "Sent" << streamedBytesSent << "streamed bytes, received" << streamedBytesReceived;
    qDebug() << "Sent" << lowPriorityStreamedBytesSent << "low-priority streamed bytes, received" <<
        lowPriorityStreamedBytesReceived;
    qDebug() << "Sent" << datagramsSent << "datagrams, received" << datagramsReceived;
    
    qDebug() << "All tests passed!";
    
    return false;
}

static QByteArray createRandomBytes(int minimumSize, int maximumSize) {
    QByteArray bytes(randIntInRange(minimumSize, maximumSize), 0);
    for (int i = 0; i < bytes.size(); i++) {
        bytes[i] = rand();
    }
    return bytes;
}

static QByteArray createRandomBytes() {
    const int MIN_BYTES = 4;
    const int MAX_BYTES = 16;
    return createRandomBytes(MIN_BYTES, MAX_BYTES);
}

Endpoint::Endpoint(const QByteArray& datagramHeader) :
    _sequencer(new DatagramSequencer(datagramHeader, this)),
    _highPriorityMessagesToSend(0.0f) {
    
    connect(_sequencer, SIGNAL(readyToWrite(const QByteArray&)), SLOT(sendDatagram(const QByteArray&)));
    connect(_sequencer, SIGNAL(readyToRead(Bitstream&)), SLOT(readMessage(Bitstream&)));
    connect(_sequencer, SIGNAL(receivedHighPriorityMessage(const QVariant&)),
        SLOT(handleHighPriorityMessage(const QVariant&)));
    connect(&_sequencer->getReliableInputChannel()->getBuffer(), SIGNAL(readyRead()), SLOT(readReliableChannel()));
    connect(&_sequencer->getReliableInputChannel(1)->getBuffer(), SIGNAL(readyRead()), SLOT(readLowPriorityReliableChannel()));
    
    // enqueue a large amount of data in a low-priority channel
    ReliableChannel* output = _sequencer->getReliableOutputChannel(1);
    output->setPriority(0.25f);
    const int MIN_LOW_PRIORITY_DATA = 100000;
    const int MAX_LOW_PRIORITY_DATA = 200000;
    QByteArray bytes = createRandomBytes(MIN_LOW_PRIORITY_DATA, MAX_LOW_PRIORITY_DATA);
    _lowPriorityDataStreamed.append(bytes);
    output->getBuffer().write(bytes);
    lowPriorityStreamedBytesSent += bytes.size();
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
    // update/send our delayed datagrams
    for (QList<QPair<QByteArray, int> >::iterator it = _delayedDatagrams.begin(); it != _delayedDatagrams.end(); ) {
        if (it->second-- == 1) {
            _other->_sequencer->receivedDatagram(it->first);
            datagramsReceived++;    
            it = _delayedDatagrams.erase(it);
        
        } else {
            it++;
        }
    }

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
    
    // stream some random data
    const int MIN_BYTES_TO_STREAM = 10;
    const int MAX_BYTES_TO_STREAM = 100;
    QByteArray bytes = createRandomBytes(MIN_BYTES_TO_STREAM, MAX_BYTES_TO_STREAM);
    _dataStreamed.append(bytes);
    streamedBytesSent += bytes.size();
    _sequencer->getReliableOutputChannel()->getDataStream().writeRawData(bytes.constData(), bytes.size());
    
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
    datagramsSent++;
    
    // some datagrams are dropped
    const float DROP_PROBABILITY = 0.1f;
    if (randFloat() < DROP_PROBABILITY) {
        return;
    }
    
    // some are received out of order
    const float REORDER_PROBABILITY = 0.1f;
    if (randFloat() < REORDER_PROBABILITY) {
        const int MIN_DELAY = 1;
        const int MAX_DELAY = 5;
        // have to copy the datagram; the one we're passed is a reference to a shared buffer
        _delayedDatagrams.append(QPair<QByteArray, int>(QByteArray(datagram.constData(), datagram.size()),
            randIntInRange(MIN_DELAY, MAX_DELAY)));
        
        // and some are duplicated
        const float DUPLICATE_PROBABILITY = 0.01f;
        if (randFloat() > DUPLICATE_PROBABILITY) {
            return;
        }
    }
    
    _other->_sequencer->receivedDatagram(datagram);
    datagramsReceived++;
}

void Endpoint::handleHighPriorityMessage(const QVariant& message) {
    if (_other->_highPriorityMessagesSent.isEmpty()) {
        throw QString("Received unsent/already sent high priority message.");
    }
    QVariant sentMessage = _other->_highPriorityMessagesSent.takeFirst();
    if (!messagesEqual(message, sentMessage)) {
        throw QString("Sent/received high priority message mismatch.");
    }
    highPriorityMessagesReceived++;
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

void Endpoint::readReliableChannel() {
    CircularBuffer& buffer = _sequencer->getReliableInputChannel()->getBuffer();
    QByteArray bytes = buffer.read(buffer.bytesAvailable());
    if (_other->_dataStreamed.size() < bytes.size()) {
        throw QString("Received unsent/already sent streamed data.");
    }
    QByteArray compare = _other->_dataStreamed.readBytes(0, bytes.size());
    _other->_dataStreamed.remove(bytes.size());
    if (compare != bytes) {
        throw QString("Sent/received streamed data mismatch.");
    }
    streamedBytesReceived += bytes.size();
}

void Endpoint::readLowPriorityReliableChannel() {
    CircularBuffer& buffer = _sequencer->getReliableInputChannel(1)->getBuffer();
    QByteArray bytes = buffer.read(buffer.bytesAvailable());
    if (_other->_lowPriorityDataStreamed.size() < bytes.size()) {
        throw QString("Received unsent/already sent low-priority streamed data.");
    }
    QByteArray compare = _other->_lowPriorityDataStreamed.readBytes(0, bytes.size());
    _other->_lowPriorityDataStreamed.remove(bytes.size());
    if (compare != bytes) {
        throw QString("Sent/received low-priority streamed data mismatch.");
    }
    lowPriorityStreamedBytesReceived += bytes.size();
}
