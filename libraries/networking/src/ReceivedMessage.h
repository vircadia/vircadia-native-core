//
//  ReceivedMessage.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/09/15
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ReceivedMessage_h
#define hifi_ReceivedMessage_h

#include <QByteArray>
#include <QObject>
#include <QtCore/QSharedPointer>

#include <atomic>

#include "NLPacketList.h"

class ReceivedMessage : public QObject {
    Q_OBJECT
public:
    ReceivedMessage(const NLPacketList& packetList);
    ReceivedMessage(NLPacket& packet);
    ReceivedMessage(QByteArray byteArray, PacketType packetType, PacketVersion packetVersion,
                    const SockAddr& senderSockAddr, NLPacket::LocalID sourceID = NLPacket::NULL_LOCAL_ID);

    QByteArray getMessage() const { return _data; }
    const char* getRawMessage() const { return _data.constData(); }

    PacketType getType() const { return _packetType; }
    PacketVersion getVersion() const { return _packetVersion; }

    void setFailed();

    void appendPacket(NLPacket& packet);

    bool failed() const { return _failed; }
    bool isComplete() const { return _isComplete; }

    NLPacket::LocalID getSourceID() const { return _sourceID; }
    const SockAddr& getSenderSockAddr() { return _senderSockAddr; }

    qint64 getPosition() const { return _position; }

    // Get the number of packets that were used to send this message
    qint64 getNumPackets() const { return _numPackets; }

    qint64 getFirstPacketReceiveTime() const { return _firstPacketReceiveTime; }

    qint64 getSize() const { return _data.size(); }

    qint64 getBytesLeftToRead() const { return _data.size() -  _position; }

    void seek(qint64 position) { _position = position; }

    qint64 peek(char* data, qint64 size);
    qint64 read(char* data, qint64 size);

    // Temporary functionality for reading in the first HEAD_DATA_SIZE bytes of the message
    // safely across threads.
    qint64 readHead(char* data, qint64 size);

    QByteArray peek(qint64 size);
    QByteArray read(qint64 size);
    QByteArray readAll();

    QString readString();

    QByteArray readHead(qint64 size);

    // This will return a QByteArray referencing the underlying data _without_ refcounting that data.
    // Be careful when using this method, only use it when the lifetime of the returned QByteArray will not
    // exceed that of the ReceivedMessage.
    QByteArray readWithoutCopy(qint64 size);

    template<typename T> qint64 peekPrimitive(T* data);
    template<typename T> qint64 readPrimitive(T* data);

    template<typename T> qint64 readHeadPrimitive(T* data);

signals:
    void progress(qint64 size);
    void completed();

private slots:
    void onComplete();

private:
    QByteArray _data;
    QByteArray _headData;

    std::atomic<qint64> _position { 0 };
    std::atomic<qint64> _numPackets { 0 };
    std::atomic<quint64> _firstPacketReceiveTime { 0 };

    NLPacket::LocalID _sourceID { NLPacket::NULL_LOCAL_ID };
    PacketType _packetType;
    PacketVersion _packetVersion;
    SockAddr _senderSockAddr;

    std::atomic<bool> _isComplete { true };  
    std::atomic<bool> _failed { false };
};

Q_DECLARE_METATYPE(ReceivedMessage*)
Q_DECLARE_METATYPE(QSharedPointer<ReceivedMessage>)

template<typename T> qint64 ReceivedMessage::peekPrimitive(T* data) {
    return peek(reinterpret_cast<char*>(data), sizeof(T));
}

template<typename T> qint64 ReceivedMessage::readPrimitive(T* data) {
    return read(reinterpret_cast<char*>(data), sizeof(T));
}

template<typename T> qint64 ReceivedMessage::readHeadPrimitive(T* data) {
    return readHead(reinterpret_cast<char*>(data), sizeof(T));
}

#endif
