//
//  ReceivedMessage.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/09/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_ReceivedMessage_h
#define hifi_ReceivedMessage_h

#include <QByteArray>
#include <QObject>

#include <atomic>
#include <mutex>

#include "NLPacketList.h"

class ReceivedMessage : public QObject {
    Q_OBJECT
public:
    ReceivedMessage(const NLPacketList& packetList);
    ReceivedMessage(NLPacket& packet);

    const char* getPayload() const { return _data.constData(); }
    QByteArray getMessage() const { return _data; }
    PacketType getType() const { return _packetType; }
    PacketVersion getVersion() const { return _packetVersion; }

    void appendPacket(std::unique_ptr<NLPacket> packet);

    bool isComplete() const { return _isComplete; }

    const QUuid& getSourceID() const { return _sourceID; }
    const HifiSockAddr& getSenderSockAddr() { return _senderSockAddr; }

    void seek(qint64 position) { _position = position; }
    qint64 pos() const { return _position; }
    qint64 size() const { return _data.size(); }

    // Get the number of packets that were used to send this message
    qint64 getNumPackets() const { return _numPackets; }

    qint64 getDataSize() const { return _data.size(); }
    qint64 getPayloadSize() const { return _data.size(); }

    qint64 getBytesLeftToRead() const { return _data.size() -  _position; }

    qint64 peek(char* data, qint64 size);
    qint64 read(char* data, qint64 size);

    QByteArray peek(qint64 size);
    QByteArray read(qint64 size);
    QByteArray readAll();

    // This will return a QByteArray referencing the underlying data _without_ refcounting that data.
    // Be careful when using this method, only use it when the lifetime of the returned QByteArray will not
    // exceed that of the ReceivedMessage.
    QByteArray readWithoutCopy(qint64 size);

    template<typename T> qint64 peekPrimitive(T* data);
    template<typename T> qint64 readPrimitive(T* data);

signals:
    void progress(ReceivedMessage*);
    void completed(ReceivedMessage*);

private slots:
    void onComplete();

private:
    QByteArray _data;
    QUuid _sourceID;
    qint64 _numPackets;
    PacketType _packetType;
    PacketVersion _packetVersion;
    qint64 _position { 0 };
    HifiSockAddr _senderSockAddr;

    // Total size of message, including UDT headers. Does not include UDP headers.
    qint64 _totalDataSize;

    std::atomic<bool> _isComplete { true };  
};

Q_DECLARE_METATYPE(ReceivedMessage*)
Q_DECLARE_METATYPE(QSharedPointer<ReceivedMessage>)

template<typename T> qint64 ReceivedMessage::peekPrimitive(T* data) {
    return peek(reinterpret_cast<char*>(data), sizeof(T));
}

template<typename T> qint64 ReceivedMessage::readPrimitive(T* data) {
    return read(reinterpret_cast<char*>(data), sizeof(T));
}

#endif
