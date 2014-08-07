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
    _updater->thread()->quit();
    _updater->thread()->wait();
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

void MetavoxelClientManager::setSphere(const glm::vec3& center, float radius, const QColor& color) {
    Sphere* sphere = new Sphere();
    sphere->setTranslation(center);
    sphere->setScale(radius);
    sphere->setColor(color);
    setSpanner(sphere);
}

void MetavoxelClientManager::setSpanner(const SharedObjectPointer& object, bool reliable) {
    MetavoxelEditMessage edit = { QVariant::fromValue(SetSpannerEdit(object)) };
    applyEdit(edit, reliable);
}

void MetavoxelClientManager::applyEdit(const MetavoxelEditMessage& edit, bool reliable) {
    QMetaObject::invokeMethod(_updater, "applyEdit", Q_ARG(const MetavoxelEditMessage&, edit), Q_ARG(bool, reliable));
}

class HeightVisitor : public MetavoxelVisitor {
public:
    
    float height;
    
    HeightVisitor(const MetavoxelLOD& lod, const glm::vec3& location);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    glm::vec3 _location;
};

HeightVisitor::HeightVisitor(const MetavoxelLOD& lod, const glm::vec3& location) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getHeightfieldAttribute(),
        QVector<AttributePointer>(), lod),
    height(-FLT_MAX),
    _location(location) {
}

static const int REVERSE_ORDER = MetavoxelVisitor::encodeOrder(7, 6, 5, 4, 3, 2, 1, 0);
static const float EIGHT_BIT_MAXIMUM_RECIPROCAL = 1.0f / 255.0f;

int HeightVisitor::visit(MetavoxelInfo& info) {
    glm::vec3 relative = _location - info.minimum;
    if (relative.x < 0.0f || relative.z < 0.0f || relative.x > info.size || relative.z > info.size ||
            height >= info.minimum.y + info.size) {
        return STOP_RECURSION;
    }
    if (!info.isLeaf) {
        return REVERSE_ORDER;
    }
    HeightfieldDataPointer pointer = info.inputValues.at(0).getInlineValue<HeightfieldDataPointer>();
    if (!pointer) {
        return STOP_RECURSION;
    }
    const QByteArray& contents = pointer->getContents();
    const uchar* src = (const uchar*)contents.constData();
    int size = glm::sqrt((float)contents.size());
    int highest = size - 1;
    relative *= highest / info.size;
    glm::vec3 floors = glm::floor(relative);
    glm::vec3 ceils = glm::ceil(relative);
    glm::vec3 fracts = glm::fract(relative);
    int floorX = qMin(qMax((int)floors.x, 0), highest);
    int floorZ = qMin(qMax((int)floors.z, 0), highest);
    int ceilX = qMin(qMax((int)ceils.x, 0), highest);
    int ceilZ = qMin(qMax((int)ceils.z, 0), highest);
    float upperLeft = src[floorZ * size + floorX];
    float lowerRight = src[ceilZ * size + ceilX];
    float interpolatedHeight;
    if (fracts.x > fracts.z) {
        float upperRight = src[floorZ * size + ceilX];
        interpolatedHeight = glm::mix(glm::mix(upperLeft, upperRight, fracts.x), lowerRight, fracts.z);
        
    } else {
        float lowerLeft = src[ceilZ * size + floorX];
        interpolatedHeight = glm::mix(upperLeft, glm::mix(lowerLeft, lowerRight, fracts.x), fracts.z);
    }
    if (interpolatedHeight == 0.0f) {
        return STOP_RECURSION;
    }
    height = qMax(height, info.minimum.y + interpolatedHeight * info.size * EIGHT_BIT_MAXIMUM_RECIPROCAL);
    return SHORT_CIRCUIT;
}

float MetavoxelClientManager::getHeight(const glm::vec3& location) {
    HeightVisitor visitor(getLOD(), location);
    guide(visitor);
    return visitor.height;
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
    _reliableDeltaID(0) {
    
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

void MetavoxelClient::dataChanged(const MetavoxelData& oldData) {
    // make thread-safe copy
    QWriteLocker locker(&_dataCopyLock);
    _dataCopy = _data;
}

void MetavoxelClient::writeUpdateMessage(Bitstream& out) {
    ClientStateMessage state = { _updater->getLOD() };
    out << QVariant::fromValue(state);
}

void MetavoxelClient::handleMessage(const QVariant& message, Bitstream& in) {
    int userType = message.userType(); 
    if (userType == MetavoxelDeltaMessage::Type) {
        PacketRecord* receiveRecord = getLastAcknowledgedReceiveRecord();
        if (_reliableDeltaChannel) {    
            _remoteData.readDelta(receiveRecord->getData(), receiveRecord->getLOD(), in, _remoteDataLOD = _reliableDeltaLOD);
            _sequencer.getInputStream().persistReadMappings(in.getAndResetReadMappings());
            in.clearPersistentMappings();
            _reliableDeltaChannel = NULL;
        
        } else {
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
        int id = message.value<MetavoxelDeltaPendingMessage>().id;
        if (id > _reliableDeltaID) {
            _reliableDeltaID = id;
            _reliableDeltaChannel = _sequencer.getReliableInputChannel(RELIABLE_DELTA_CHANNEL_INDEX);
            _reliableDeltaChannel->getBitstream().copyPersistentMappings(_sequencer.getInputStream());
            _reliableDeltaLOD = getLastAcknowledgedSendRecord()->getLOD();
            PacketRecord* receiveRecord = getLastAcknowledgedReceiveRecord();
            _remoteDataLOD = receiveRecord->getLOD();
            _remoteData = receiveRecord->getData();
        }
    } else {
        Endpoint::handleMessage(message, in);
    }
}

PacketRecord* MetavoxelClient::maybeCreateSendRecord() const {
    return new PacketRecord(_reliableDeltaChannel ? _reliableDeltaLOD : _updater->getLOD());
}

PacketRecord* MetavoxelClient::maybeCreateReceiveRecord() const {
    return new PacketRecord(_remoteDataLOD, _remoteData);
}

