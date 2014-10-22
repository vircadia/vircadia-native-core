//
//  Endpoint.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 6/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PacketHeaders.h>

#include "Endpoint.h"

Endpoint::Endpoint(const SharedNodePointer& node, PacketRecord* baselineSendRecord, PacketRecord* baselineReceiveRecord) :
    _node(node),
    _sequencer(byteArrayWithPopulatedHeader(PacketTypeMetavoxelData), this) {
    
    connect(&_sequencer, SIGNAL(readyToWrite(const QByteArray&)), SLOT(sendDatagram(const QByteArray&)));
    connect(&_sequencer, SIGNAL(readyToRead(Bitstream&)), SLOT(readMessage(Bitstream&)));
    connect(&_sequencer, SIGNAL(sendRecorded()), SLOT(recordSend()));
    connect(&_sequencer, SIGNAL(receiveRecorded()), SLOT(recordReceive()));
    connect(&_sequencer, SIGNAL(sendAcknowledged(int)), SLOT(clearSendRecordsBefore(int)));
    connect(&_sequencer, SIGNAL(receiveAcknowledged(int)), SLOT(clearReceiveRecordsBefore(int)));
    
    // insert the baseline send and receive records
    _sendRecords.append(baselineSendRecord);
    _receiveRecords.append(baselineReceiveRecord);
}

Endpoint::~Endpoint() {
    foreach (PacketRecord* record, _sendRecords) {
        delete record;
    }
    foreach (PacketRecord* record, _receiveRecords) {
        delete record;
    }
}

void Endpoint::update() {
    int packetsToSend = _sequencer.notePacketGroup();
    for (int i = 0; i < packetsToSend; i++) {
        Bitstream& out = _sequencer.startPacket();
        writeUpdateMessage(out);
        _sequencer.endPacket();
    }
}

int Endpoint::parseData(const QByteArray& packet) {
    // process through sequencer
    QMetaObject::invokeMethod(&_sequencer, "receivedDatagram", Q_ARG(const QByteArray&, packet));
    return packet.size();
}

void Endpoint::sendDatagram(const QByteArray& data) {
    NodeList::getInstance()->writeDatagram(data, _node);
}

void Endpoint::readMessage(Bitstream& in) {
    QVariant message;
    in >> message;
    handleMessage(message, in);
}

void Endpoint::handleMessage(const QVariant& message, Bitstream& in) {
    if (message.userType() == QMetaType::QVariantList) {
        foreach (const QVariant& element, message.toList()) {
            handleMessage(element, in);
        }
    }
}

void Endpoint::recordSend() {
    _sendRecords.append(maybeCreateSendRecord());
}

void Endpoint::recordReceive() {
    _receiveRecords.append(maybeCreateReceiveRecord());
}

void Endpoint::clearSendRecordsBefore(int index) {
    QList<PacketRecord*>::iterator end = _sendRecords.begin() + index + 1;
    for (QList<PacketRecord*>::const_iterator it = _sendRecords.begin(); it != end; it++) {
        delete *it;
    }
    _sendRecords.erase(_sendRecords.begin(), end);
}

void Endpoint::clearReceiveRecordsBefore(int index) {
    QList<PacketRecord*>::iterator end = _receiveRecords.begin() + index + 1;
    for (QList<PacketRecord*>::const_iterator it = _receiveRecords.begin(); it != end; it++) {
        delete *it;
    }
    _receiveRecords.erase(_receiveRecords.begin(), end);
}

void Endpoint::writeUpdateMessage(Bitstream& out) {
    out << QVariant();
}

PacketRecord* Endpoint::maybeCreateSendRecord() const {
    return NULL;
}

PacketRecord* Endpoint::maybeCreateReceiveRecord() const {
    return NULL;
}

PacketRecord::PacketRecord(int packetNumber, const MetavoxelLOD& lod, const MetavoxelData& data) :
    _packetNumber(packetNumber),
    _lod(lod),
    _data(data) {
}

PacketRecord::~PacketRecord() {
}
