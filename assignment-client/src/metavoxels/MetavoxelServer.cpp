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

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QThread>

#include <PacketHeaders.h>

#include <MetavoxelMessages.h>
#include <MetavoxelUtil.h>

#include "MetavoxelServer.h"

MetavoxelServer::MetavoxelServer(const QByteArray& packet) :
    ThreadedAssignment(packet),
    _nextSender(0),
    _savedDataInitialized(false) {
}

void MetavoxelServer::applyEdit(const MetavoxelEditMessage& edit) {
    MetavoxelData data = _data;
    edit.apply(data, SharedObject::getWeakHash());
    setData(data);
}

void MetavoxelServer::setData(const MetavoxelData& data, bool loaded) {
    if (_data == data) {
        return;
    }    
    emit dataChanged(_data = data);
    
    if (loaded) {
        _savedData = data;
    
    } else if (!_savedDataInitialized) {        
        _savedDataInitialized = true;
        
        // start the save timer
        QTimer* saveTimer = new QTimer(this);
        connect(saveTimer, &QTimer::timeout, this, &MetavoxelServer::maybeSaveData);
        const int SAVE_INTERVAL = 1000 * 30;
        saveTimer->start(SAVE_INTERVAL);
    }
}

const QString METAVOXEL_SERVER_LOGGING_NAME = "metavoxel-server";

void MetavoxelServer::run() {
    commonInit(METAVOXEL_SERVER_LOGGING_NAME, NodeType::MetavoxelServer);
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);
    
    connect(nodeList, &NodeList::nodeAdded, this, &MetavoxelServer::maybeAttachSession);
    connect(nodeList, &NodeList::nodeKilled, this, &MetavoxelServer::maybeDeleteSession);
    
    // initialize Bitstream before using it in multiple threads
    Bitstream::preThreadingInit();
    
    // create the senders, each with its own thread
    int threadCount = QThread::idealThreadCount();
    if (threadCount == -1) {
        const int DEFAULT_THREAD_COUNT = 4;
        threadCount = DEFAULT_THREAD_COUNT;
    }
    qDebug() << "Creating" << threadCount << "sender threads";
    for (int i = 0; i < threadCount; i++) {
        QThread* thread = new QThread(this);
        MetavoxelSender* sender = new MetavoxelSender(this);
        sender->moveToThread(thread);
        connect(thread, &QThread::finished, sender, &QObject::deleteLater);
        thread->start();
        QMetaObject::invokeMethod(sender, "start");
        _senders.append(sender);
    }
    
    // create the persister and start it in its own thread
    _persister = new MetavoxelPersister(this);
    QThread* persistenceThread = new QThread(this);
    _persister->moveToThread(persistenceThread);
    connect(persistenceThread, &QThread::finished, _persister, &QObject::deleteLater);
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
    
    foreach (MetavoxelSender* sender, _senders) {
        sender->thread()->quit();
        sender->thread()->wait();
    }
    _persister->thread()->quit();
    _persister->thread()->wait();
}

void MetavoxelServer::maybeAttachSession(const SharedNodePointer& node) {
    if (node->getType() == NodeType::Agent) {
        QMutexLocker locker(&node->getMutex());
        MetavoxelSender* sender = _senders.at(_nextSender);
        _nextSender = (_nextSender + 1) % _senders.size();
        MetavoxelSession* session = new MetavoxelSession(node, sender);
        session->moveToThread(sender->thread());
        QMetaObject::invokeMethod(sender, "addSession", Q_ARG(QObject*, session));
        node->setLinkedData(session);
    }
}

void MetavoxelServer::maybeDeleteSession(const SharedNodePointer& node) {
    if (node->getType() == NodeType::Agent) {
        // we assume the node is already locked
        MetavoxelSession* session = static_cast<MetavoxelSession*>(node->getLinkedData());
        if (session) {    
            node->setLinkedData(NULL);
            session->deleteLater();
        }
    }
}

void MetavoxelServer::maybeSaveData() {
    if (_savedData != _data) {
        QMetaObject::invokeMethod(_persister, "save", Q_ARG(const MetavoxelData&, _savedData = _data));
    }
}

MetavoxelSender::MetavoxelSender(MetavoxelServer* server) :
    _server(server),
    _sendTimer(this) {
    
    _sendTimer.setSingleShot(true);
    connect(&_sendTimer, &QTimer::timeout, this, &MetavoxelSender::sendDeltas);
    
    connect(_server, &MetavoxelServer::dataChanged, this, &MetavoxelSender::setData);
}

const int SEND_INTERVAL = 50;

void MetavoxelSender::start() {
    _lastSend = QDateTime::currentMSecsSinceEpoch();
    _sendTimer.start(SEND_INTERVAL);
}

void MetavoxelSender::addSession(QObject* session) {
    _sessions.insert(static_cast<MetavoxelSession*>(session));
    connect(session, &QObject::destroyed, this, &MetavoxelSender::removeSession);
}

void MetavoxelSender::sendDeltas() {
    // send deltas for all sessions associated with our thread
    foreach (MetavoxelSession* session, _sessions) {
        session->update();
    }
    
    // restart the send timer
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int elapsed = now - _lastSend;
    _lastSend = now;
    
    _sendTimer.start(qMax(0, 2 * SEND_INTERVAL - qMax(elapsed, SEND_INTERVAL)));
}

void MetavoxelSender::removeSession(QObject* session) {
    _sessions.remove(static_cast<MetavoxelSession*>(session));
}

MetavoxelSession::MetavoxelSession(const SharedNodePointer& node, MetavoxelSender* sender) :
    Endpoint(node, new PacketRecord(), NULL),
    _sender(sender),
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
    _sender->getData().writeDelta(sendRecord->getData(), sendRecord->getLOD(), out, _lod);
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
        _reliableDeltaData = _sender->getData();
        _reliableDeltaLOD = _lod;
        
        // go back to the beginning with the current packet and note that there's a delta pending
        _sequencer.getOutputStream().getUnderlying().device()->seek(start);
        MetavoxelDeltaPendingMessage msg = { ++_reliableDeltaID, sendRecord->getPacketNumber(), _lodPacketNumber };
        out << (_reliableDeltaMessage = QVariant::fromValue(msg));
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
    return _reliableDeltaChannel ? new PacketRecord(_sequencer.getOutgoingPacketNumber(),
        _reliableDeltaLOD, _reliableDeltaData) : new PacketRecord(_sequencer.getOutgoingPacketNumber(),
            _lod, _sender->getData());
}

void MetavoxelSession::handleMessage(const QVariant& message) {
    int userType = message.userType();
    if (userType == ClientStateMessage::Type) {
        ClientStateMessage state = message.value<ClientStateMessage>();
        _lod = state.lod;
        _lodPacketNumber = _sequencer.getIncomingPacketNumber();
        
    } else if (userType == MetavoxelEditMessage::Type) {
        QMetaObject::invokeMethod(_sender->getServer(), "applyEdit", Q_ARG(const MetavoxelEditMessage&,
            message.value<MetavoxelEditMessage>()));
        
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
            out << _reliableDeltaMessage;
        } else {
            out << QVariant();
        }
        _sequencer.endPacket();
    }
}

MetavoxelPersister::MetavoxelPersister(MetavoxelServer* server) :
    _server(server) {
}

const char* SAVE_FILE = "/resources/metavoxels.dat";

const int FILE_MAGIC = 0xDADAFACE;
const int FILE_VERSION = 3;

void MetavoxelPersister::load() {
    QString path = QCoreApplication::applicationDirPath() + SAVE_FILE;
    QFile file(path);
    if (!file.exists()) {
        return;
    }
    MetavoxelData data;
    {
        QDebug debug = qDebug() << "Reading from" << path << "...";
        file.open(QIODevice::ReadOnly);
        QDataStream inStream(&file);
        Bitstream in(inStream);
        int magic, version;
        in >> magic;
        if (magic != FILE_MAGIC) {
            debug << "wrong file magic: " << magic;
            return;
        }
        in >> version;
        if (version != FILE_VERSION) {
            debug << "wrong file version: " << version;
            return;
        }
        try {
            in >> data;
        } catch (const BitstreamException& e) {
            debug << "failed, " << e.getDescription();
            return;
        }
        QMetaObject::invokeMethod(_server, "setData", Q_ARG(const MetavoxelData&, data), Q_ARG(bool, true));
        debug << "done.";
    }
    data.dumpStats();
}

void MetavoxelPersister::save(const MetavoxelData& data) {
    QString path = QCoreApplication::applicationDirPath() + SAVE_FILE;
    QDir directory = QFileInfo(path).dir();
    if (!directory.exists()) {
        directory.mkpath(".");
    }
    QDebug debug = qDebug() << "Writing to" << path << "...";
    QSaveFile file(path);
    file.open(QIODevice::WriteOnly);
    QDataStream outStream(&file);
    Bitstream out(outStream);
    out << FILE_MAGIC << FILE_VERSION << data;
    out.flush();
    file.commit();
    debug << "done.";
}
