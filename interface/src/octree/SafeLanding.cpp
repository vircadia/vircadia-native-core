//
//  SafeLanding.cpp
//  interface/src/octree
//
//  Created by Simon Walton.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SafeLanding.h"
#include "EntityTreeRenderer.h"
#include "ModelEntityItem.h"
#include "InterfaceLogging.h"

const int SafeLanding::SEQUENCE_MODULO = std::numeric_limits<OCTREE_PACKET_SEQUENCE>::max() + 1;

namespace {
    template<typename T> bool lessThanWraparound(int a, int b) {
        constexpr int MAX_T_VALUE = std::numeric_limits<T>::max();
        if (b <= a) {
            b += MAX_T_VALUE;
        }
        return (b - a) < (MAX_T_VALUE / 2);
    }
}

bool SafeLanding::SequenceLessThan::operator()(const int& a, const int& b) const {
    return lessThanWraparound<OCTREE_PACKET_SEQUENCE>(a, b);
}

void SafeLanding::startEntitySequence(QSharedPointer<EntityTreeRenderer> entityTreeRenderer) {
    if (!_trackingEntities) {
        auto entityTree = entityTreeRenderer->getTree();

        if (entityTree) {
            _entityTree = entityTree;
            connect(std::const_pointer_cast<EntityTree>(_entityTree).get(),
                &EntityTree::addingEntity, this, &SafeLanding::addTrackedEntity);
            connect(std::const_pointer_cast<EntityTree>(_entityTree).get(),
                &EntityTree::deletingEntity, this, &SafeLanding::deleteTrackedEntity);
            _trackingEntities = true;
            _sequenceNumbers.clear();
        }
    }
}

void SafeLanding::stopEntitySequence() {
    _trackingEntities = false;
    _initialStart = INVALID_SEQUENCE;
    _initialEnd = INVALID_SEQUENCE;
    _trackedEntities.clear();
    _sequenceNumbers.clear();
}

void SafeLanding::addTrackedEntity(const EntityItemID& entityID) {
    if (_trackingEntities) {
        EntityItemPointer entity = _entityTree->findEntityByID(entityID);

        if (entity && !entity->getCollisionless()) {
            const auto& entityType = entity->getType();
            if (entityType == EntityTypes::Model) {
                ModelEntityItem * modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity).get();
                auto shapeType = modelEntity->getShapeType();
                if (shapeType == SHAPE_TYPE_COMPOUND || shapeType == SHAPE_TYPE_STATIC_MESH ||
                    shapeType == SHAPE_TYPE_SIMPLE_COMPOUND) {
                    // Only track entities with downloaded collision bodies.
                    _trackedEntities.emplace(entityID, entity);
                    qCDebug(interfaceapp) << "Safe Landing: Tracking entity " << entity->getItemName();
                }
            }
        }
    }
}

void SafeLanding::deleteTrackedEntity(const EntityItemID& entityID) {
    _trackedEntities.erase(entityID);
}

void SafeLanding::setCompletionSequenceNumbers(int first, int last) {
    _initialStart = first;
    _initialEnd = (last + 1) % SEQUENCE_MODULO;
    auto firstIter = _sequenceNumbers.find(_initialStart);
    if (firstIter != _sequenceNumbers.end()) {
        _sequenceNumbers.erase(_sequenceNumbers.begin(), firstIter);
    }
    _sequenceNumbers.erase(_sequenceNumbers.find(_initialEnd), _sequenceNumbers.end());
}

void SafeLanding::sequenceNumberReceived(int sequenceNumber) {
    if (_trackingEntities) {
        _sequenceNumbers.insert(sequenceNumber);
    }
}

bool SafeLanding::sequenceNumbersComplete() {
    if (_initialStart != INVALID_SEQUENCE) {
        auto startIter = _sequenceNumbers.find(_initialStart);
        if (startIter != _sequenceNumbers.end()) {
            _sequenceNumbers.erase(_sequenceNumbers.begin(), startIter);
            _sequenceNumbers.erase(_sequenceNumbers.find(_initialEnd), _sequenceNumbers.end());
            int sequenceSize = _initialStart < _initialEnd ? _initialEnd - _initialStart :
                _initialEnd + SEQUENCE_MODULO - _initialStart;
            // First no. exists, nothing outside of first, last exists, so complete iff set size is sequence size.
            return (int) _sequenceNumbers.size() == sequenceSize;
        }
    }
    return false;
}

void SafeLanding::resourceLoaded() {
    QObject * sender = QObject::sender();
    if (sender) {
        Resource * resource = dynamic_cast<Resource*>(sender);
        const QString resourceURL = resource->getURL().toString();
        _trackedURLs.erase(resourceURL);
        qCDebug(interfaceapp) << "Safe Landing: Removed tracked URL" << resourceURL;
    }
}

bool SafeLanding::isLoadSequenceComplete() {
    if (sequenceNumbersComplete() && entityPhysicsComplete()) {
        _trackingEntities = false;
        _trackedEntities.clear();
        qCDebug(interfaceapp) << "Safe Landing: load sequence complete";
    }

    return !_trackingEntities;
}

bool SafeLanding::entityPhysicsComplete() {
    for (auto entityMapIter = _trackedEntities.begin(); entityMapIter != _trackedEntities.end(); ++entityMapIter) {
        auto entity = entityMapIter->second;
        if (!entity->shouldBePhysical() || entity->isReadyToComputeShape()) {
            entityMapIter = _trackedEntities.erase(entityMapIter);
            if (entityMapIter == _trackedEntities.end()) {
                break;
            }
        }
    }
    return _trackedEntities.empty();
}

void SafeLanding::debugDumpSequenceIDs() const {
    int p = -1;
    qCDebug(interfaceapp) << "Sequence set size:" << _sequenceNumbers.size();
    for (auto s: _sequenceNumbers) {
        if (p == -1) {
            p = s;
            qCDebug(interfaceapp) << "First:" << s;
        } else {
            if (s != p + 1) {
                qCDebug(interfaceapp) << "Gap from" << p << "to" << s << "(exclusive)";
                p = s;
            }
        }
    }
    if (p != -1) {
        qCDebug(interfaceapp) << "Last:" << p;
    }
}
