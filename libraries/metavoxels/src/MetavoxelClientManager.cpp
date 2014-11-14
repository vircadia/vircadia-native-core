//
//  MetavoxelClientManager.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 6/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDateTime>
#include <QReadLocker>
#include <QThread>
#include <QWriteLocker>

#include "MetavoxelClientManager.h"
#include "MetavoxelMessages.h"

MetavoxelClientManager::MetavoxelClientManager() :
        _updater(new MetavoxelUpdater(this)) {
    QThread* thread = new QThread(this);
    _updater->moveToThread(thread);
    connect(thread, &QThread::finished, _updater, &QObject::deleteLater);
    thread->start();
    QMetaObject::invokeMethod(_updater, "start");
}

MetavoxelClientManager::~MetavoxelClientManager() {
    if (_updater) {
        _updater->thread()->quit();
        _updater->thread()->wait();
    }
}

void MetavoxelClientManager::init() {
    connect(NodeList::getInstance(), &NodeList::nodeAdded, this, &MetavoxelClientManager::maybeAttachClient);
    connect(NodeList::getInstance(), &NodeList::nodeKilled, this, &MetavoxelClientManager::maybeDeleteClient);
}

SharedObjectPointer MetavoxelClientManager::findFirstRaySpannerIntersection(const glm::vec3& origin,
        const glm::vec3& direction, const AttributePointer& attribute, float& distance) {
    SharedObjectPointer closestSpanner;
    float closestDistance = FLT_MAX;
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelClient* client = static_cast<MetavoxelClient*>(node->getLinkedData());
            if (client) {
                float clientDistance;
                SharedObjectPointer clientSpanner = client->getDataCopy().findFirstRaySpannerIntersection(
                    origin, direction, attribute, clientDistance);
                if (clientSpanner && clientDistance < closestDistance) {
                    closestSpanner = clientSpanner;
                    closestDistance = clientDistance;
                }
            }
        }
    }
    if (closestSpanner) {
        distance = closestDistance;
    }
    return closestSpanner;
}

class RayHeightfieldIntersectionVisitor : public RaySpannerIntersectionVisitor {
public:
    
    float intersectionDistance;
    
    RayHeightfieldIntersectionVisitor(const glm::vec3& origin, const glm::vec3& direction, const MetavoxelLOD& lod);
    
    virtual bool visitSpanner(Spanner* spanner, float distance);
};

RayHeightfieldIntersectionVisitor::RayHeightfieldIntersectionVisitor(const glm::vec3& origin,
        const glm::vec3& direction, const MetavoxelLOD& lod) :
    RaySpannerIntersectionVisitor(origin, direction, QVector<AttributePointer>() <<
        AttributeRegistry::getInstance()->getSpannersAttribute(),
            QVector<AttributePointer>(), QVector<AttributePointer>(), lod),
    intersectionDistance(FLT_MAX) {
}

bool RayHeightfieldIntersectionVisitor::visitSpanner(Spanner* spanner, float distance) {
    if (spanner->isHeightfield()) {
        intersectionDistance = distance;
        return false;
    }
    return true;
}

bool MetavoxelClientManager::findFirstRayHeightfieldIntersection(const glm::vec3& origin,
        const glm::vec3& direction, float& distance) {
    RayHeightfieldIntersectionVisitor visitor(origin, direction, getLOD());
    guide(visitor);
    if (visitor.intersectionDistance == FLT_MAX) {
        return false;
    }
    distance = visitor.intersectionDistance;
    return true;
}

class HeightfieldHeightVisitor : public SpannerVisitor {
public:
    
    float height;
    
    HeightfieldHeightVisitor(const MetavoxelLOD& lod, const glm::vec3& location);
    
    virtual bool visit(Spanner* spanner);
    virtual int visit(MetavoxelInfo& info);

private:
    
    glm::vec3 _location;
};

HeightfieldHeightVisitor::HeightfieldHeightVisitor(const MetavoxelLOD& lod, const glm::vec3& location) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute(),
        QVector<AttributePointer>(), QVector<AttributePointer>(), lod),
    height(-FLT_MAX),
    _location(location) {
}

bool HeightfieldHeightVisitor::visit(Spanner* spanner) {
    height = qMax(height, spanner->getHeight(_location));
    return true;
}

static const int REVERSE_ORDER = MetavoxelVisitor::encodeOrder(7, 6, 5, 4, 3, 2, 1, 0);

int HeightfieldHeightVisitor::visit(MetavoxelInfo& info) {
    if (_location.x < info.minimum.x || _location.z < info.minimum.z || _location.x > info.minimum.x + info.size ||
            _location.z > info.minimum.z + info.size) {
        return STOP_RECURSION;
    } 
    SpannerVisitor::visit(info);
    return (height == -FLT_MAX) ? (info.isLeaf ? STOP_RECURSION : REVERSE_ORDER) : SHORT_CIRCUIT;
}

float MetavoxelClientManager::getHeightfieldHeight(const glm::vec3& location) {
    HeightfieldHeightVisitor visitor(getLOD(), location);
    guide(visitor);
    return visitor.height;
}

void MetavoxelClientManager::paintHeightfieldHeight(const glm::vec3& position, float radius, float height) {
    MetavoxelEditMessage edit = { QVariant::fromValue(PaintHeightfieldHeightEdit(position, radius, height)) };
    applyEdit(edit, true);
}

void MetavoxelClientManager::applyEdit(const MetavoxelEditMessage& edit, bool reliable) {
    QMetaObject::invokeMethod(_updater, "applyEdit", Q_ARG(const MetavoxelEditMessage&, edit), Q_ARG(bool, reliable));
}

MetavoxelLOD MetavoxelClientManager::getLOD() {
    return MetavoxelLOD();
}

void MetavoxelClientManager::maybeAttachClient(const SharedNodePointer& node) {
    if (node->getType() == NodeType::MetavoxelServer) {
        QMutexLocker locker(&node->getMutex());
        MetavoxelClient* client = createClient(node);
        client->moveToThread(_updater->thread());
        QMetaObject::invokeMethod(_updater, "addClient", Q_ARG(QObject*, client));
        node->setLinkedData(client);
    }
}

void MetavoxelClientManager::maybeDeleteClient(const SharedNodePointer& node) {
    if (node->getType() == NodeType::MetavoxelServer) {
        // we assume the node is already locked
        MetavoxelClient* client = static_cast<MetavoxelClient*>(node->getLinkedData());
        if (client) {
            node->setLinkedData(NULL);
            client->deleteLater();
        }
    }
}

MetavoxelClient* MetavoxelClientManager::createClient(const SharedNodePointer& node) {
    return new MetavoxelClient(node, _updater);
}

void MetavoxelClientManager::guide(MetavoxelVisitor& visitor) {
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelClient* client = static_cast<MetavoxelClient*>(node->getLinkedData());
            if (client) {
                client->getDataCopy().guide(visitor);
            }
        }
    }
}

MetavoxelUpdater::MetavoxelUpdater(MetavoxelClientManager* clientManager) :
    _clientManager(clientManager),
    _sendTimer(this) {
    
    _sendTimer.setSingleShot(true);
    connect(&_sendTimer, &QTimer::timeout, this, &MetavoxelUpdater::sendUpdates);
}

const int SEND_INTERVAL = 33;

void MetavoxelUpdater::start() {
    _lastSend = QDateTime::currentMSecsSinceEpoch();
    _sendTimer.start(SEND_INTERVAL);
}

void MetavoxelUpdater::addClient(QObject* client) {
    _clients.insert(static_cast<MetavoxelClient*>(client));
    connect(client, &QObject::destroyed, this, &MetavoxelUpdater::removeClient);
}

void MetavoxelUpdater::applyEdit(const MetavoxelEditMessage& edit, bool reliable) {
    // apply to all clients
    foreach (MetavoxelClient* client, _clients) {
        client->applyEdit(edit, reliable);
    }
}

void MetavoxelUpdater::getStats(QObject* receiver, const QByteArray& method) {
    int internal = 0, leaves = 0;
    int sendProgress = 0, sendTotal = 0;
    int receiveProgress = 0, receiveTotal = 0;    
    foreach (MetavoxelClient* client, _clients) {
        client->getData().countNodes(internal, leaves, _lod);
        client->getSequencer().addReliableChannelStats(sendProgress, sendTotal, receiveProgress, receiveTotal);
    }
    QMetaObject::invokeMethod(receiver, method.constData(), Q_ARG(int, internal), Q_ARG(int, leaves), Q_ARG(int, sendProgress),
        Q_ARG(int, sendTotal), Q_ARG(int, receiveProgress), Q_ARG(int, receiveTotal));
}

void MetavoxelUpdater::sendUpdates() {
    // get the latest LOD from the client manager
    _lod = _clientManager->getLOD();

    // send updates for all clients
    foreach (MetavoxelClient* client, _clients) {
        client->update();
    }
    
    // restart the send timer
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int elapsed = now - _lastSend;
    _lastSend = now;
    
    _sendTimer.start(qMax(0, 2 * SEND_INTERVAL - qMax(elapsed, SEND_INTERVAL)));
}

void MetavoxelUpdater::removeClient(QObject* client) {
    _clients.remove(static_cast<MetavoxelClient*>(client));
}

MetavoxelClient::MetavoxelClient(const SharedNodePointer& node, MetavoxelUpdater* updater) :
    Endpoint(node, new PacketRecord(), new PacketRecord()),
    _updater(updater),
    _reliableDeltaChannel(NULL),
    _reliableDeltaID(0),
    _dummyInputStream(_dummyDataStream),
    _dummyPacketNumber(0) {
    
    connect(_sequencer.getReliableInputChannel(RELIABLE_DELTA_CHANNEL_INDEX),
        SIGNAL(receivedMessage(const QVariant&, Bitstream&)), SLOT(handleMessage(const QVariant&, Bitstream&)));
}

MetavoxelData MetavoxelClient::getDataCopy() {
    QReadLocker locker(&_dataCopyLock);
    return _dataCopy;
}

void MetavoxelClient::applyEdit(const MetavoxelEditMessage& edit, bool reliable) {
    if (reliable) {
        _sequencer.getReliableOutputChannel()->sendMessage(QVariant::fromValue(edit));
    
    } else {
        // apply immediately to local tree
        MetavoxelData oldData = _data;
        edit.apply(_data, _sequencer.getWeakSharedObjectHash());
        if (_data != oldData) {
            dataChanged(oldData);
        }
        
        // start sending it out
        _sequencer.sendHighPriorityMessage(QVariant::fromValue(edit));
    }
}

PacketRecord* MetavoxelClient::getAcknowledgedSendRecord(int packetNumber) const {
    PacketRecord* lastAcknowledged = getLastAcknowledgedSendRecord();
    if (lastAcknowledged->getPacketNumber() == packetNumber) {
        return lastAcknowledged;
    }
    foreach (PacketRecord* record, _clearedSendRecords) {
        if (record->getPacketNumber() == packetNumber) {
            return record;
        }
    }
    return NULL;
}

PacketRecord* MetavoxelClient::getAcknowledgedReceiveRecord(int packetNumber) const {
    PacketRecord* lastAcknowledged = getLastAcknowledgedReceiveRecord();
    if (lastAcknowledged->getPacketNumber() == packetNumber) {
        return lastAcknowledged;
    }
    foreach (const ClearedReceiveRecord& record, _clearedReceiveRecords) {
        if (record.first->getPacketNumber() == packetNumber) {
            return record.first;
        }
    }
    return NULL;
}

void MetavoxelClient::dataChanged(const MetavoxelData& oldData) {
    // make thread-safe copy
    QWriteLocker locker(&_dataCopyLock);
    _dataCopy = _data;
}

void MetavoxelClient::recordReceive() {
    Endpoint::recordReceive();
    
    // clear the cleared lists
    foreach (PacketRecord* record, _clearedSendRecords) {
        delete record;
    }
    _clearedSendRecords.clear();
    
    foreach (const ClearedReceiveRecord& record, _clearedReceiveRecords) {
        delete record.first;
    }
    _clearedReceiveRecords.clear();
}

void MetavoxelClient::clearSendRecordsBefore(int index) {
    // move to cleared list
    QList<PacketRecord*>::iterator end = _sendRecords.begin() + index + 1;
    for (QList<PacketRecord*>::const_iterator it = _sendRecords.begin(); it != end; it++) {
        _clearedSendRecords.append(*it);
    }
    _sendRecords.erase(_sendRecords.begin(), end);
}

void MetavoxelClient::clearReceiveRecordsBefore(int index) {
    // copy the mappings on first call per packet
    if (_sequencer.getIncomingPacketNumber() > _dummyPacketNumber) {
        _dummyPacketNumber = _sequencer.getIncomingPacketNumber();
        _dummyInputStream.copyPersistentMappings(_sequencer.getInputStream());
    }
    
    // move to cleared list
    QList<PacketRecord*>::iterator end = _receiveRecords.begin() + index + 1;
    for (QList<PacketRecord*>::const_iterator it = _receiveRecords.begin(); it != end; it++) {
        _clearedReceiveRecords.append(ClearedReceiveRecord(*it, _sequencer.getReadMappings(index)));
    }
    _receiveRecords.erase(_receiveRecords.begin(), end);
}

void MetavoxelClient::writeUpdateMessage(Bitstream& out) {
    ClientStateMessage state = { _updater->getLOD() };
    out << QVariant::fromValue(state);
}

void MetavoxelClient::handleMessage(const QVariant& message, Bitstream& in) {
    int userType = message.userType(); 
    if (userType == MetavoxelDeltaMessage::Type) {
        if (_reliableDeltaChannel) {    
            MetavoxelData reference = _remoteData;
            MetavoxelLOD referenceLOD = _remoteDataLOD;
            _remoteData.readDelta(reference, referenceLOD, in, _remoteDataLOD = _reliableDeltaLOD);
            _sequencer.getInputStream().persistReadMappings(in.getAndResetReadMappings());
            in.clearPersistentMappings();
            _reliableDeltaChannel = NULL;
        
        } else {
            PacketRecord* receiveRecord = getLastAcknowledgedReceiveRecord();
            _remoteData.readDelta(receiveRecord->getData(), receiveRecord->getLOD(), in,
                _remoteDataLOD = getLastAcknowledgedSendRecord()->getLOD());
            in.reset();
        }
        // copy to local and reapply local edits
        MetavoxelData oldData = _data;
        _data = _remoteData;
        foreach (const DatagramSequencer::HighPriorityMessage& message, _sequencer.getHighPriorityMessages()) {
            if (message.data.userType() == MetavoxelEditMessage::Type) {
                message.data.value<MetavoxelEditMessage>().apply(_data, _sequencer.getWeakSharedObjectHash());
            }
        }
        if (_data != oldData) {
            dataChanged(oldData);
        }
    } else if (userType == MetavoxelDeltaPendingMessage::Type) {
        // check the id to make sure this is not a delta we've already processed
        MetavoxelDeltaPendingMessage pending = message.value<MetavoxelDeltaPendingMessage>();
        if (pending.id > _reliableDeltaID) {
            _reliableDeltaID = pending.id;
            PacketRecord* sendRecord = getAcknowledgedSendRecord(pending.receivedPacketNumber);
            if (!sendRecord) {
                qWarning() << "Missing send record for delta" << pending.receivedPacketNumber;
                return;
            }
            _reliableDeltaLOD = sendRecord->getLOD();
            PacketRecord* receiveRecord = getAcknowledgedReceiveRecord(pending.sentPacketNumber);
            if (!receiveRecord) {
                qWarning() << "Missing receive record for delta" << pending.sentPacketNumber;
                return;
            }
            _remoteDataLOD = receiveRecord->getLOD();
            _remoteData = receiveRecord->getData();
            
            _reliableDeltaChannel = _sequencer.getReliableInputChannel(RELIABLE_DELTA_CHANNEL_INDEX);
            if (receiveRecord == getLastAcknowledgedReceiveRecord()) {
                _reliableDeltaChannel->getBitstream().copyPersistentMappings(_sequencer.getInputStream());
                
            } else {
                _reliableDeltaChannel->getBitstream().copyPersistentMappings(_dummyInputStream);
                foreach (const ClearedReceiveRecord& record, _clearedReceiveRecords) {
                    _reliableDeltaChannel->getBitstream().persistReadMappings(record.second);
                    if (record.first == receiveRecord) {
                        break;
                    }
                }
            }
        }
    } else {
        Endpoint::handleMessage(message, in);
    }
}

PacketRecord* MetavoxelClient::maybeCreateSendRecord() const {
    return new PacketRecord(_sequencer.getOutgoingPacketNumber(),
        _reliableDeltaChannel ? _reliableDeltaLOD : _updater->getLOD());
}

PacketRecord* MetavoxelClient::maybeCreateReceiveRecord() const {
    return new PacketRecord(_sequencer.getIncomingPacketNumber(), _remoteDataLOD, _remoteData);
}

