//
//  MetavoxelServer.cpp
//  assignment-client/src/metavoxels
//
//  Created by Andrzej Kapolka on 12/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDateTime>
#include <QFile>
#include <QSaveFile>
#include <QThread>

#include <PacketHeaders.h>

#include <MetavoxelMessages.h>
#include <MetavoxelUtil.h>

#include "MetavoxelServer.h"

const int SEND_INTERVAL = 50;

MetavoxelServer::MetavoxelServer(const QByteArray& packet) :
    ThreadedAssignment(packet),
    _sendTimer(this) {
    
    _sendTimer.setSingleShot(true);
    connect(&_sendTimer, SIGNAL(timeout()), SLOT(sendDeltas()));
}

void MetavoxelServer::applyEdit(const MetavoxelEditMessage& edit) {
    edit.apply(_data, SharedObject::getWeakHash());
}

const QString METAVOXEL_SERVER_LOGGING_NAME = "metavoxel-server";

void MetavoxelServer::run() {
    commonInit(METAVOXEL_SERVER_LOGGING_NAME, NodeType::MetavoxelServer);
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);
    
    connect(nodeList, SIGNAL(nodeAdded(SharedNodePointer)), SLOT(maybeAttachSession(const SharedNodePointer&)));
    connect(nodeList, SIGNAL(nodeKilled(SharedNodePointer)), SLOT(maybeDeleteSession(const SharedNodePointer&)));
    
    _lastSend = QDateTime::currentMSecsSinceEpoch();
    _sendTimer.start(SEND_INTERVAL);
    
    // initialize Bitstream before using it in multiple threads
    Bitstream::preThreadingInit();
    
    // create the persister and start it in its own thread
    _persister = new MetavoxelPersister(this);
    QThread* persistenceThread = new QThread(this);
    _persister->moveToThread(persistenceThread);
    _persister->connect(persistenceThread, SIGNAL(finished()), SLOT(deleteLater()));
    persistenceThread->start();
    
    // queue up the load
    QMetaObject::invokeMethod(_persister, "load");
}

void MetavoxelServer::readPendingDatagrams() {
    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;
    
    NodeList* nodeList = NodeList::getInstance();
    
    while (readAvailableDatagram(receivedPacket, senderSockAddr)) {
        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            switch (packetTypeForPacket(receivedPacket)) {
                case PacketTypeMetavoxelData:
                    nodeList->findNodeAndUpdateWithDataFromPacket(receivedPacket);
                    break;
                
                default:
                    nodeList->processNodeData(senderSockAddr, receivedPacket);
                    break;
            }
        }
    }
}

void MetavoxelServer::aboutToFinish() {
    QMetaObject::invokeMethod(_persister, "save", Q_ARG(const MetavoxelData&, _data));
    _persister->thread()->quit();
    _persister->thread()->wait();
}

void MetavoxelServer::maybeAttachSession(const SharedNodePointer& node) {
    if (node->getType() == NodeType::Agent) {
        QMutexLocker locker(&node->getMutex());
        node->setLinkedData(new MetavoxelSession(node, this));
    }
}

void MetavoxelServer::maybeDeleteSession(const SharedNodePointer& node) {
    if (node->getType() == NodeType::Agent) {
        QMutexLocker locker(&node->getMutex());
        MetavoxelSession* session = static_cast<MetavoxelSession*>(node->getLinkedData());
        if (session) {
            node->setLinkedData(NULL);
            session->deleteLater();
        }
    }
}

void MetavoxelServer::sendDeltas() {
    // send deltas for all sessions
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::Agent) {
            static_cast<MetavoxelSession*>(node->getLinkedData())->update();
        }
    }
    
    // restart the send timer
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int elapsed = now - _lastSend;
    _lastSend = now;
    
    _sendTimer.start(qMax(0, 2 * SEND_INTERVAL - qMax(elapsed, SEND_INTERVAL)));
}

MetavoxelSession::MetavoxelSession(const SharedNodePointer& node, MetavoxelServer* server) :
    Endpoint(node, new PacketRecord(), NULL),
    _server(server),
    _reliableDeltaChannel(NULL),
    _reliableDeltaID(0) {
    
    connect(&_sequencer, SIGNAL(receivedHighPriorityMessage(const QVariant&)), SLOT(handleMessage(const QVariant&)));
    connect(&_sequencer, SIGNAL(sendAcknowledged(int)), SLOT(checkReliableDeltaReceived()));
    connect(_sequencer.getReliableInputChannel(), SIGNAL(receivedMessage(const QVariant&, Bitstream&)),
        SLOT(handleMessage(const QVariant&, Bitstream&)));
}

void MetavoxelSession::update() {
    // wait until we have a valid lod before sending
    if (!_lod.isValid()) {
        return;
    }
    // if we're sending a reliable delta, wait until it's acknowledged
    if (_reliableDeltaChannel) {
        sendPacketGroup();
        return;
    }
    Bitstream& out = _sequencer.startPacket();
    int start = _sequencer.getOutputStream().getUnderlying().device()->pos(); 
    out << QVariant::fromValue(MetavoxelDeltaMessage());
    PacketRecord* sendRecord = getLastAcknowledgedSendRecord();
    _server->getData().writeDelta(sendRecord->getData(), sendRecord->getLOD(), out, _lod);
    out.flush();
    int end = _sequencer.getOutputStream().getUnderlying().device()->pos();
    if (end > _sequencer.getMaxPacketSize()) {
        // we need to send the delta on the reliable channel
        _reliableDeltaChannel = _sequencer.getReliableOutputChannel(RELIABLE_DELTA_CHANNEL_INDEX);
        _reliableDeltaChannel->startMessage();
        _reliableDeltaChannel->getBuffer().write(_sequencer.getOutgoingPacketData().constData() + start, end - start);
        _reliableDeltaChannel->endMessage();
        
        _reliableDeltaWriteMappings = out.getAndResetWriteMappings();
        _reliableDeltaReceivedOffset = _reliableDeltaChannel->getBytesWritten();
        _reliableDeltaData = _server->getData();
        _reliableDeltaLOD = _lod;
        
        // go back to the beginning with the current packet and note that there's a delta pending
        _sequencer.getOutputStream().getUnderlying().device()->seek(start);
        MetavoxelDeltaPendingMessage msg = { ++_reliableDeltaID };
        out << QVariant::fromValue(msg);
        _sequencer.endPacket();
        
    } else {
        _sequencer.endPacket();
    }
    
    // perhaps send additional packets to fill out the group
    sendPacketGroup(1);   
}

void MetavoxelSession::handleMessage(const QVariant& message, Bitstream& in) {
    handleMessage(message);
}

PacketRecord* MetavoxelSession::maybeCreateSendRecord() const {
    return _reliableDeltaChannel ? new PacketRecord(_reliableDeltaLOD, _reliableDeltaData) :
        new PacketRecord(_lod, _server->getData());
}

void MetavoxelSession::handleMessage(const QVariant& message) {
    int userType = message.userType();
    if (userType == ClientStateMessage::Type) {
        ClientStateMessage state = message.value<ClientStateMessage>();
        _lod = state.lod;
    
    } else if (userType == MetavoxelEditMessage::Type) {
        _server->applyEdit(message.value<MetavoxelEditMessage>());
        
    } else if (userType == QMetaType::QVariantList) {
        foreach (const QVariant& element, message.toList()) {
            handleMessage(element);
        }
    }
}

void MetavoxelSession::checkReliableDeltaReceived() {
    if (!_reliableDeltaChannel || _reliableDeltaChannel->getOffset() < _reliableDeltaReceivedOffset) {
        return;
    }
    _sequencer.getOutputStream().persistWriteMappings(_reliableDeltaWriteMappings);
    _reliableDeltaWriteMappings = Bitstream::WriteMappings();
    _reliableDeltaData = MetavoxelData();
    _reliableDeltaChannel = NULL;
}

void MetavoxelSession::sendPacketGroup(int alreadySent) {
    int additionalPackets = _sequencer.notePacketGroup() - alreadySent;
    for (int i = 0; i < additionalPackets; i++) {
        Bitstream& out = _sequencer.startPacket();
        if (_reliableDeltaChannel) {
            MetavoxelDeltaPendingMessage msg = { _reliableDeltaID };
            out << QVariant::fromValue(msg);
        } else {
            out << QVariant();
        }
        _sequencer.endPacket();
    }
}

MetavoxelPersister::MetavoxelPersister(MetavoxelServer* server) :
    _server(server) {
}

const char* SAVE_FILE = "metavoxels.dat";

void MetavoxelPersister::load() {
    QFile file(SAVE_FILE);
    if (!file.exists()) {
        return;
    }
    MetavoxelData data;
    {
        QDebug debug = qDebug() << "Reading from" << SAVE_FILE << "...";
        file.open(QIODevice::ReadOnly);
        QDataStream inStream(&file);
        Bitstream in(inStream);
        try {
            in >> data;
        } catch (const BitstreamException& e) {
            debug << "failed, " << e.getDescription();
            return;
        }
        QMetaObject::invokeMethod(_server, "setData", Q_ARG(const MetavoxelData&, data));
        debug << "done.";
    }
    data.dumpStats();
}

void MetavoxelPersister::save(const MetavoxelData& data) {
    QDebug debug = qDebug() << "Writing to" << SAVE_FILE << "...";
    QSaveFile file(SAVE_FILE);
    file.open(QIODevice::WriteOnly);
    QDataStream outStream(&file);
    Bitstream out(outStream);
    out << data;
    out.flush();
    file.commit();
    debug << "done.";
}
