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

Endpoint::Endpoint(const SharedNodePointer& node) :
    _node(node),
    _sequencer(byteArrayWithPopulatedHeader(PacketTypeMetavoxelData)) {
    
    connect(&_sequencer, SIGNAL(readyToWrite(const QByteArray&)), SLOT(sendDatagram(const QByteArray&)));
    connect(&_sequencer, SIGNAL(readyToRead(Bitstream&)), SLOT(readMessage(Bitstream&)));
    connect(&_sequencer, SIGNAL(sendAcknowledged(int)), SLOT(clearSendRecordsBefore(int)));
    connect(&_sequencer, SIGNAL(receiveAcknowledged(int)), SLOT(clearReceiveRecordsBefore(int)));
    
    // insert the baseline send and receive records
    _sendRecords.append(maybeCreateSendRecord(true));
    _receiveRecords.append(maybeCreateReceiveRecord(true));
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
    Bitstream& out = _sequencer.startPacket();
    out << QVariant();
    _sequencer.endPacket();

    // record the send
    _sendRecords.append(maybeCreateSendRecord());
}

int Endpoint::parseData(const QByteArray& packet) {
    // process through sequencer
    _sequencer.receivedDatagram(packet);
    return packet.size();
}

void Endpoint::sendDatagram(const QByteArray& data) {
    NodeList::getInstance()->writeDatagram(data, _node);
}

void Endpoint::readMessage(Bitstream& in) {
    QVariant message;
    in >> message;
    handleMessage(message, in);
    
    // record the receipt
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

void Endpoint::handleMessage(const QVariant& message, Bitstream& in) {
    if (message.userType() == QMetaType::QVariantList) {
        foreach (const QVariant& element, message.toList()) {
            handleMessage(element, in);
        }
    }
}

PacketRecord* Endpoint::maybeCreateSendRecord(bool baseline) const {
    return NULL;
}

PacketRecord* Endpoint::maybeCreateReceiveRecord(bool baseline) const {
    return NULL;
}

PacketRecord::PacketRecord(const MetavoxelLOD& lod) :
    _lod(lod) {
}

PacketRecord::~PacketRecord() {
}
