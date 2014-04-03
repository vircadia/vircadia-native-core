//
//  MetavoxelTests.cpp
//  metavoxel-tests
//
//  Created by Andrzej Kapolka on 2/7/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <stdlib.h>

#include <SharedUtil.h>

#include <MetavoxelMessages.h>

#include "MetavoxelTests.h"

REGISTER_META_OBJECT(TestSharedObjectA)
REGISTER_META_OBJECT(TestSharedObjectB)

MetavoxelTests::MetavoxelTests(int& argc, char** argv) :
    QCoreApplication(argc, argv) {
}

static int datagramsSent = 0;
static int datagramsReceived = 0;
static int highPriorityMessagesSent = 0;
static int highPriorityMessagesReceived = 0;
static int unreliableMessagesSent = 0;
static int unreliableMessagesReceived = 0;
static int reliableMessagesSent = 0;
static int reliableMessagesReceived = 0;
static int streamedBytesSent = 0;
static int streamedBytesReceived = 0;
static int sharedObjectsCreated = 0;
static int sharedObjectsDestroyed = 0;

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

static TestMessageC createRandomMessageC() {
    TestMessageC message;
    message.foo = randomBoolean();
    message.bar = rand();
    message.baz = randFloat();
    message.bong.foo = createRandomBytes();
    return message;
}

static bool testSerialization(Bitstream::MetadataType metadataType) {
    QByteArray array;
    QDataStream outStream(&array, QIODevice::WriteOnly);
    Bitstream out(outStream, metadataType);
    SharedObjectPointer testObjectWrittenA = new TestSharedObjectA(randFloat());
    out << testObjectWrittenA;
    SharedObjectPointer testObjectWrittenB = new TestSharedObjectB(randFloat(), createRandomBytes());
    out << testObjectWrittenB;
    TestMessageC messageWritten = createRandomMessageC();
    out << QVariant::fromValue(messageWritten);
    QByteArray endWritten = "end";
    out << endWritten;
    out.flush();
    
    QDataStream inStream(array);
    Bitstream in(inStream, metadataType);
    in.addMetaObjectSubstitution("TestSharedObjectA", &TestSharedObjectB::staticMetaObject);
    in.addMetaObjectSubstitution("TestSharedObjectB", &TestSharedObjectA::staticMetaObject);
    in.addTypeSubstitution("TestMessageC", TestMessageA::Type);
    SharedObjectPointer testObjectReadA;
    in >> testObjectReadA;
    
    if (!testObjectReadA || testObjectReadA->metaObject() != &TestSharedObjectB::staticMetaObject) {
        qDebug() << "Wrong class for A" << testObjectReadA << metadataType;
        return true;
    }
    if (metadataType == Bitstream::FULL_METADATA && static_cast<TestSharedObjectA*>(testObjectWrittenA.data())->getFoo() !=
            static_cast<TestSharedObjectB*>(testObjectReadA.data())->getFoo()) {
        QDebug debug = qDebug() << "Failed to transfer shared field from A to B";
        testObjectWrittenA->dump(debug);
        testObjectReadA->dump(debug); 
        return true;
    }
    
    SharedObjectPointer testObjectReadB;
    in >> testObjectReadB;
    if (!testObjectReadB || testObjectReadB->metaObject() != &TestSharedObjectA::staticMetaObject) {
        qDebug() << "Wrong class for B" << testObjectReadB << metadataType;
        return true;
    }
    if (metadataType == Bitstream::FULL_METADATA && static_cast<TestSharedObjectB*>(testObjectWrittenB.data())->getFoo() !=
            static_cast<TestSharedObjectA*>(testObjectReadB.data())->getFoo()) {
        QDebug debug = qDebug() << "Failed to transfer shared field from B to A";
        testObjectWrittenB->dump(debug);
        testObjectReadB->dump(debug); 
        return true;
    }
    
    QVariant messageRead;
    in >> messageRead;
    if (!messageRead.isValid() || messageRead.userType() != TestMessageA::Type) {
        qDebug() << "Wrong type for message" << messageRead;
        return true;
    }
    if (metadataType == Bitstream::FULL_METADATA && messageWritten.foo != messageRead.value<TestMessageA>().foo) {
        QDebug debug = qDebug() << "Failed to transfer shared field between messages" <<
            messageWritten.foo << messageRead.value<TestMessageA>().foo;
        return true;
    }
    
    QByteArray endRead;
    in >> endRead;
    if (endWritten != endRead) {
        qDebug() << "End tag mismatch." << endRead;
        return true;
    }
    
    return false;
}

bool MetavoxelTests::run() {
    
    qDebug() << "Running transmission tests...";
    qDebug();
    
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
    qDebug() << "Sent" << reliableMessagesSent << "reliable messages, received" << reliableMessagesReceived;
    qDebug() << "Sent" << streamedBytesSent << "streamed bytes, received" << streamedBytesReceived;
    qDebug() << "Sent" << datagramsSent << "datagrams, received" << datagramsReceived;
    qDebug() << "Created" << sharedObjectsCreated << "shared objects, destroyed" << sharedObjectsDestroyed;
    qDebug();
    
    qDebug() << "Running serialization tests...";
    qDebug();
    
    if (testSerialization(Bitstream::HASH_METADATA) || testSerialization(Bitstream::FULL_METADATA)) {
        return true;
    }
    
    qDebug() << "All tests passed!";
    
    return false;
}

static SharedObjectPointer createRandomSharedObject() {
    switch (randIntInRange(0, 2)) {
        case 0: return new TestSharedObjectA(randFloat());
        case 1: return new TestSharedObjectB();
        case 2:
        default: return SharedObjectPointer();
    }
}

Endpoint::Endpoint(const QByteArray& datagramHeader) :
    _sequencer(new DatagramSequencer(datagramHeader, this)),
    _highPriorityMessagesToSend(0.0f),
    _reliableMessagesToSend(0.0f) {
    
    connect(_sequencer, SIGNAL(readyToWrite(const QByteArray&)), SLOT(sendDatagram(const QByteArray&)));
    connect(_sequencer, SIGNAL(readyToRead(Bitstream&)), SLOT(readMessage(Bitstream&)));
    connect(_sequencer, SIGNAL(receivedHighPriorityMessage(const QVariant&)),
        SLOT(handleHighPriorityMessage(const QVariant&)));
    
    connect(_sequencer->getReliableInputChannel(), SIGNAL(receivedMessage(const QVariant&)),
        SLOT(handleReliableMessage(const QVariant&)));
    
    ReliableChannel* secondInput = _sequencer->getReliableInputChannel(1);
    secondInput->setMessagesEnabled(false);
    connect(&secondInput->getBuffer(), SIGNAL(readyRead()), SLOT(readReliableChannel()));
    
    // enqueue a large amount of data in a low-priority channel
    ReliableChannel* output = _sequencer->getReliableOutputChannel(1);
    output->setPriority(0.25f);
    output->setMessagesEnabled(false);
    const int MIN_STREAM_BYTES = 100000;
    const int MAX_STREAM_BYTES = 200000;
    QByteArray bytes = createRandomBytes(MIN_STREAM_BYTES, MAX_STREAM_BYTES);
    _dataStreamed.append(bytes);
    output->getBuffer().write(bytes);
    streamedBytesSent += bytes.size();
}

static QVariant createRandomMessage() {
    switch (randIntInRange(0, 2)) {
        case 0: {
            TestMessageA message = { randomBoolean(), rand(), randFloat() };
            return QVariant::fromValue(message);
        }
        case 1: {
            TestMessageB message = { createRandomBytes(), createRandomSharedObject() };
            return QVariant::fromValue(message); 
        }
        case 2:
        default: {
            return QVariant::fromValue(createRandomMessageC());
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
        TestMessageB first = firstMessage.value<TestMessageB>();
        TestMessageB second = secondMessage.value<TestMessageB>();
        return first.foo == second.foo && equals(first.bar, second.bar);
        
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
    
    // and some number of reliable messages
    const float MIN_RELIABLE_MESSAGES = 0.0f;
    const float MAX_RELIABLE_MESSAGES = 4.0f;
    _reliableMessagesToSend += randFloatInRange(MIN_RELIABLE_MESSAGES, MAX_RELIABLE_MESSAGES);   
    while (_reliableMessagesToSend >= 1.0f) {
        QVariant message = createRandomMessage();
        _reliableMessagesSent.append(message);
        _sequencer->getReliableOutputChannel()->sendMessage(message);
        reliableMessagesSent++;
        _reliableMessagesToSend -= 1.0f;
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
    if (message.userType() == ClearSharedObjectMessage::Type) {
        return;
    }
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

void Endpoint::handleReliableMessage(const QVariant& message) {
    if (message.userType() == ClearSharedObjectMessage::Type ||
            message.userType() == ClearMainChannelSharedObjectMessage::Type) {
        return;
    }
    if (_other->_reliableMessagesSent.isEmpty()) {
        throw QString("Received unsent/already sent reliable message.");
    }
    QVariant sentMessage = _other->_reliableMessagesSent.takeFirst();
    if (!messagesEqual(message, sentMessage)) {
        throw QString("Sent/received reliable message mismatch.");
    }
    reliableMessagesReceived++;
}

void Endpoint::readReliableChannel() {
    CircularBuffer& buffer = _sequencer->getReliableInputChannel(1)->getBuffer();
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

TestSharedObjectA::TestSharedObjectA(float foo) :
        _foo(foo) {
    sharedObjectsCreated++;    
}

TestSharedObjectA::~TestSharedObjectA() {
    sharedObjectsDestroyed++;
}

void TestSharedObjectA::setFoo(float foo) {
    if (_foo != foo) {
        emit fooChanged(_foo = foo);
    }
}

TestSharedObjectB::TestSharedObjectB(float foo, const QByteArray& bar) :
        _foo(foo),
        _bar(bar) {
    sharedObjectsCreated++;
}

TestSharedObjectB::~TestSharedObjectB() {
    sharedObjectsDestroyed++;
}
