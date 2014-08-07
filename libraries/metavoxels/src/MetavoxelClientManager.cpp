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

class RayHeightfieldIntersectionVisitor : public RayIntersectionVisitor {
public:
    
    float intersectionDistance;
    
    RayHeightfieldIntersectionVisitor(const glm::vec3& origin, const glm::vec3& direction, const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info, float distance);
};

RayHeightfieldIntersectionVisitor::RayHeightfieldIntersectionVisitor(const glm::vec3& origin,
        const glm::vec3& direction, const MetavoxelLOD& lod) :
    RayIntersectionVisitor(origin, direction, QVector<AttributePointer>() <<
        AttributeRegistry::getInstance()->getHeightfieldAttribute(), QVector<AttributePointer>(), lod),
    intersectionDistance(FLT_MAX) {
}

static const float EIGHT_BIT_MAXIMUM_RECIPROCAL = 1.0f / 255.0f;

int RayHeightfieldIntersectionVisitor::visit(MetavoxelInfo& info, float distance) {
    if (!info.isLeaf) {
        return _order;
    }
    HeightfieldDataPointer pointer = info.inputValues.at(0).getInlineValue<HeightfieldDataPointer>();
    if (!pointer) {
        return STOP_RECURSION;
    }
    const QByteArray& contents = pointer->getContents();
    const uchar* src = (const uchar*)contents.constData();
    int size = glm::sqrt((float)contents.size());
    int highest = size - 1;
    float heightScale = highest / EIGHT_BIT_MAXIMUM_RECIPROCAL;
    
    // find the initial location in heightfield coordinates
    glm::vec3 entry = (_origin + distance * _direction - info.minimum) * (float)highest / info.size;
    glm::vec3 floors = glm::floor(entry);
    glm::vec3 ceils = glm::ceil(entry);
    if (floors.x == ceils.x) {
        if (_direction.x > 0.0f) {
            ceils.x += 1.0f;
        } else {
            floors.x -= 1.0f;
        } 
    }
    if (floors.z == ceils.z) {
        if (_direction.z > 0.0f) {
            ceils.z += 1.0f;
        } else {
            floors.z -= 1.0f;
        }
    }
    
    bool withinBounds = true;
    float accumulatedDistance = 0.0f;
    while (withinBounds) {
        // find the heights at the corners of the current cell
        int floorX = qMin(qMax((int)floors.x, 0), highest);
        int floorZ = qMin(qMax((int)floors.z, 0), highest);
        int ceilX = qMin(qMax((int)ceils.x, 0), highest);
        int ceilZ = qMin(qMax((int)ceils.z, 0), highest);
        float upperLeft = src[floorZ * size + floorX] * heightScale;
        float upperRight = src[floorZ * size + ceilX] * heightScale;
        float lowerLeft = src[ceilZ * size + floorX] * heightScale;
        float lowerRight = src[ceilZ * size + ceilX] * heightScale;
        
        // find the distance to the next x coordinate
        float xDistance = FLT_MAX;
        if (_direction.x > 0.0f) {
            xDistance = (ceils.x - entry.x) / _direction.x;
        } else if (_direction.x < 0.0f) {
            xDistance = (floors.x - entry.x) / _direction.x;
        }
        
        // and the distance to the next z coordinate
        float zDistance = FLT_MAX;
        if (_direction.z > 0.0f) {
            zDistance = (ceils.z - entry.z) / _direction.z;
        } else if (_direction.z < 0.0f) {
            zDistance = (floors.z - entry.z) / _direction.z;
        }
        
        // the exit distance is the lower of those two
        float exitDistance = qMin(xDistance, zDistance);
        glm::vec3 exit, nextFloors = floors, nextCeils = ceils;
        if (exitDistance == FLT_MAX) {
            if (_direction.y > 0.0f) {
                return SHORT_CIRCUIT; // line points upwards; no collisions possible
            }    
            withinBounds = false; // line points downwards; check this cell only
            
        } else {
            // find the exit point and the next cell, and determine whether it's still within the bounds
            exit = entry + exitDistance * _direction;
            withinBounds = (exit.y >= 0.0f && exit.y <= highest);
            if (exitDistance == xDistance) {
                if (_direction.x > 0.0f) {
                    nextFloors.x += 1.0f;
                    withinBounds &= (nextCeils.x += 1.0f) <= highest;
                } else {
                    withinBounds &= (nextFloors.x -= 1.0f) >= 0.0f;
                    nextCeils.x -= 1.0f;
                }
            }
            if (exitDistance == zDistance) {
                if (_direction.z > 0.0f) {
                    nextFloors.z += 1.0f;
                    withinBounds &= (nextCeils.z += 1.0f) <= highest;
                } else {
                    withinBounds &= (nextFloors.z -= 1.0f) >= 0.0f;
                    nextCeils.z -= 1.0f;
                }
            }
            // check the vertical range of the ray against the ranges of the cell heights
            if (qMin(entry.y, exit.y) > qMax(qMax(upperLeft, upperRight), qMax(lowerLeft, lowerRight)) ||
                    qMax(entry.y, exit.y) < qMin(qMin(upperLeft, upperRight), qMin(lowerLeft, lowerRight))) {
                entry = exit;
                floors = nextFloors;
                ceils = nextCeils;
                accumulatedDistance += exitDistance;
                continue;
            } 
        }
        // having passed the bounds check, we must check against the planes
        glm::vec3 relativeEntry = entry - glm::vec3(floors.x, upperLeft, floors.z);
        
        // first check the triangle including the Z+ segment
        glm::vec3 lowerNormal(lowerLeft - lowerRight, 1.0f, upperLeft - lowerLeft);
        float lowerProduct = glm::dot(lowerNormal, _direction);
        if (lowerProduct < 0.0f) {
            float planeDistance = -glm::dot(lowerNormal, relativeEntry) / lowerProduct;
            glm::vec3 intersection = relativeEntry + planeDistance * _direction;
            if (intersection.x >= 0.0f && intersection.x <= 1.0f && intersection.z >= 0.0f && intersection.z <= 1.0f &&
                    intersection.z >= intersection.x) {
                intersectionDistance = distance + (accumulatedDistance + planeDistance) * (info.size / highest);
                return SHORT_CIRCUIT;
            }
        }
        
        // then the one with the X+ segment
        glm::vec3 upperNormal(upperLeft - upperRight, 1.0f, upperRight - lowerRight);
        float upperProduct = glm::dot(upperNormal, _direction);
        if (upperProduct < 0.0f) {
            float planeDistance = -glm::dot(upperNormal, relativeEntry) / upperProduct;
            glm::vec3 intersection = relativeEntry + planeDistance * _direction;
            if (intersection.x >= 0.0f && intersection.x <= 1.0f && intersection.z >= 0.0f && intersection.z <= 1.0f &&
                    intersection.x >= intersection.z) {
                intersectionDistance = distance + (accumulatedDistance + planeDistance) * (info.size / highest);
                return SHORT_CIRCUIT;
            }
        }
        
        // no joy; continue on our way
        entry = exit;
        floors = nextFloors;
        ceils = nextCeils;
        accumulatedDistance += exitDistance;
    }
    
    return STOP_RECURSION;
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

class HeightfieldHeightVisitor : public MetavoxelVisitor {
public:
    
    float height;
    
    HeightfieldHeightVisitor(const MetavoxelLOD& lod, const glm::vec3& location);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    glm::vec3 _location;
};

HeightfieldHeightVisitor::HeightfieldHeightVisitor(const MetavoxelLOD& lod, const glm::vec3& location) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getHeightfieldAttribute(),
        QVector<AttributePointer>(), lod),
    height(-FLT_MAX),
    _location(location) {
}

static const int REVERSE_ORDER = MetavoxelVisitor::encodeOrder(7, 6, 5, 4, 3, 2, 1, 0);

int HeightfieldHeightVisitor::visit(MetavoxelInfo& info) {
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
    
    // find the bounds of the cell containing the point and the shared vertex heights
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
    
    // the final vertex (and thus which triangle we check) depends on which half we're on
    if (fracts.x > fracts.z) {
        float upperRight = src[floorZ * size + ceilX];
        interpolatedHeight = glm::mix(glm::mix(upperLeft, upperRight, fracts.x), lowerRight, fracts.z);
        
    } else {
        float lowerLeft = src[ceilZ * size + floorX];
        interpolatedHeight = glm::mix(upperLeft, glm::mix(lowerLeft, lowerRight, fracts.x), fracts.z);
    }
    if (interpolatedHeight == 0.0f) {
        return STOP_RECURSION; // ignore zero values
    }
    
    // convert the interpolated height into world space
    height = qMax(height, info.minimum.y + interpolatedHeight * info.size * EIGHT_BIT_MAXIMUM_RECIPROCAL);
    return SHORT_CIRCUIT;
}

float MetavoxelClientManager::getHeightfieldHeight(const glm::vec3& location) {
    HeightfieldHeightVisitor visitor(getLOD(), location);
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

