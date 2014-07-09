//
//  MetavoxelTests.h
//  tests/metavoxels/src
//
//  Created by Andrzej Kapolka on 2/7/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MetavoxelTests_h
#define hifi_MetavoxelTests_h

#include <QCoreApplication>
#include <QVariantList>

#include <Endpoint.h>
#include <ScriptCache.h>

class SequencedTestMessage;

/// Tests various aspects of the metavoxel library.
class MetavoxelTests : public QCoreApplication {
    Q_OBJECT
    
public:
    
    MetavoxelTests(int& argc, char** argv);
    
    /// Performs our various tests.
    /// \return true if any of the tests failed.
    bool run();
};

/// Represents a simulated endpoint.
class TestEndpoint : public Endpoint {
    Q_OBJECT

public:
    
    enum Mode { BASIC_PEER_MODE, CONGESTION_MODE, METAVOXEL_SERVER_MODE, METAVOXEL_CLIENT_MODE };
    
    TestEndpoint(Mode mode = BASIC_PEER_MODE);

    void setOther(TestEndpoint* other) { _other = other; }

    /// Perform a simulation step.
    /// \return true if failure was detected
    bool simulate(int iterationNumber);

    virtual int parseData(const QByteArray& packet);
    
protected:

    virtual void sendDatagram(const QByteArray& data);
    virtual void readMessage(Bitstream& in);
    
    virtual void handleMessage(const QVariant& message, Bitstream& in); 
    
    virtual PacketRecord* maybeCreateSendRecord() const;
    virtual PacketRecord* maybeCreateReceiveRecord() const;
    
private slots:
   
    void handleHighPriorityMessage(const QVariant& message);
    void handleReliableMessage(const QVariant& message, Bitstream& in);
    void readReliableChannel();
    void checkReliableDeltaReceived();

private:
    
    void compareMetavoxelData();
    
    Mode _mode;
    
    SharedObjectPointer _localState;
    SharedObjectPointer _remoteState;
    
    MetavoxelData _data;
    MetavoxelLOD _dataLOD;
    MetavoxelData _remoteData;
    MetavoxelLOD _remoteDataLOD;
    MetavoxelLOD _lod;
    
    SharedObjectPointer _sphere;
    
    TestEndpoint* _other;
    
    typedef QPair<QByteArray, int> ByteArrayIntPair;
    QList<ByteArrayIntPair> _delayedDatagrams;

    typedef QVector<QByteArray> ByteArrayVector;
    QList<ByteArrayVector> _pipeline;
    int _remainingPipelineCapacity;

    float _highPriorityMessagesToSend;
    QVariantList _highPriorityMessagesSent;
    QList<SequencedTestMessage> _unreliableMessagesSent;
    float _reliableMessagesToSend;
    QVariantList _reliableMessagesSent;
    CircularBuffer _dataStreamed;
    
    ReliableChannel* _reliableDeltaChannel;
    int _reliableDeltaReceivedOffset;
    MetavoxelData _reliableDeltaData;
    MetavoxelLOD _reliableDeltaLOD;
    Bitstream::WriteMappings _reliableDeltaWriteMappings;
    int _reliableDeltaID;
};

/// A simple shared object.
class TestSharedObjectA : public SharedObject {
    Q_OBJECT
    Q_ENUMS(TestEnum)
    Q_FLAGS(TestFlag TestFlags)
    Q_PROPERTY(float foo READ getFoo WRITE setFoo NOTIFY fooChanged)
    Q_PROPERTY(TestEnum baz READ getBaz WRITE setBaz)
    Q_PROPERTY(TestFlags bong READ getBong WRITE setBong)
    Q_PROPERTY(QScriptValue bizzle READ getBizzle WRITE setBizzle)
    
public:
    
    enum TestEnum { FIRST_TEST_ENUM, SECOND_TEST_ENUM, THIRD_TEST_ENUM };
    
    enum TestFlag { NO_TEST_FLAGS = 0x0, FIRST_TEST_FLAG = 0x01, SECOND_TEST_FLAG = 0x02, THIRD_TEST_FLAG = 0x04 };
    Q_DECLARE_FLAGS(TestFlags, TestFlag)
    
    Q_INVOKABLE TestSharedObjectA(float foo = 0.0f, TestEnum baz = FIRST_TEST_ENUM, TestFlags bong = 0);
    virtual ~TestSharedObjectA();

    void setFoo(float foo);
    float getFoo() const { return _foo; }

    void setBaz(TestEnum baz) { _baz = baz; }
    TestEnum getBaz() const { return _baz; }

    void setBong(TestFlags bong) { _bong = bong; }
    TestFlags getBong() const { return _bong; }
    
    void setBizzle(const QScriptValue& bizzle) { _bizzle = bizzle; }
    const QScriptValue& getBizzle() const { return _bizzle; }
    
signals:
    
    void fooChanged(float foo);    
    
private:
    
    float _foo;
    TestEnum _baz;
    TestFlags _bong;
    QScriptValue _bizzle;
};

DECLARE_ENUM_METATYPE(TestSharedObjectA, TestEnum)

/// Another simple shared object.
class TestSharedObjectB : public SharedObject {
    Q_OBJECT
    Q_ENUMS(TestEnum)
    Q_FLAGS(TestFlag TestFlags)
    Q_PROPERTY(float foo READ getFoo WRITE setFoo)
    Q_PROPERTY(QByteArray bar READ getBar WRITE setBar)
    Q_PROPERTY(TestEnum baz READ getBaz WRITE setBaz)
    Q_PROPERTY(TestFlags bong READ getBong WRITE setBong)
    
public:
    
    enum TestEnum { ZEROTH_TEST_ENUM, FIRST_TEST_ENUM, SECOND_TEST_ENUM, THIRD_TEST_ENUM, FOURTH_TEST_ENUM };
    
    enum TestFlag { NO_TEST_FLAGS = 0x0, ZEROTH_TEST_FLAG = 0x01, FIRST_TEST_FLAG = 0x02,
        SECOND_TEST_FLAG = 0x04, THIRD_TEST_FLAG = 0x08, FOURTH_TEST_FLAG = 0x10 };
    Q_DECLARE_FLAGS(TestFlags, TestFlag)
    
    Q_INVOKABLE TestSharedObjectB(float foo = 0.0f, const QByteArray& bar = QByteArray(),
        TestEnum baz = FIRST_TEST_ENUM, TestFlags bong = 0);
    virtual ~TestSharedObjectB();

    void setFoo(float foo) { _foo = foo; }
    float getFoo() const { return _foo; }
    
    void setBar(const QByteArray& bar) { _bar = bar; }
    const QByteArray& getBar() const { return _bar; }

    void setBaz(TestEnum baz) { _baz = baz; }
    TestEnum getBaz() const { return _baz; }

    void setBong(TestFlags bong) { _bong = bong; }
    TestFlags getBong() const { return _bong; }
    
private:
    
    float _foo;
    QByteArray _bar;
    TestEnum _baz;
    TestFlags _bong;
};

/// A simple test message.
class TestMessageA {
    STREAMABLE

public:
    
    STREAM bool foo;
    STREAM int bar;
    STREAM float baz;
};

DECLARE_STREAMABLE_METATYPE(TestMessageA)

// Another simple test message.
class TestMessageB {
    STREAMABLE

public:
    
    STREAM QByteArray foo;
    STREAM SharedObjectPointer bar;
    STREAM TestSharedObjectA::TestEnum baz;
};

DECLARE_STREAMABLE_METATYPE(TestMessageB)

// A test message that demonstrates inheritance and composition.
class TestMessageC : STREAM public TestMessageA {
    STREAMABLE

public:
    
    STREAM TestMessageB bong;
    STREAM QScriptValue bizzle;
};

DECLARE_STREAMABLE_METATYPE(TestMessageC)

/// Combines a sequence number with a submessage; used for testing unreliable transport.
class SequencedTestMessage {
    STREAMABLE

public:
    
    STREAM int sequenceNumber;
    STREAM QVariant submessage;
    STREAM SharedObjectPointer state;
};

DECLARE_STREAMABLE_METATYPE(SequencedTestMessage)

#endif // hifi_MetavoxelTests_h
