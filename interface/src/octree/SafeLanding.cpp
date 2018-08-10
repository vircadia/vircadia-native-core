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
            Locker lock(_lock);
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
    Locker lock(_lock);
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
                static const std::set<ShapeType> downloadedCollisionTypes
                    { SHAPE_TYPE_COMPOUND, SHAPE_TYPE_SIMPLE_COMPOUND, SHAPE_TYPE_STATIC_MESH,  SHAPE_TYPE_SIMPLE_HULL };
                if (downloadedCollisionTypes.count(modelEntity->getShapeType()) != 0) {
                    // Only track entities with downloaded collision bodies.
                    Locker lock(_lock);
                    _trackedEntities.emplace(entityID, entity);
                    qCDebug(interfaceapp) << "Safe Landing: Tracking entity " << entity->getItemName();
                }
            }
        }
    }
}

void SafeLanding::deleteTrackedEntity(const EntityItemID& entityID) {
    Locker lock(_lock);
    _trackedEntities.erase(entityID);
}

void SafeLanding::setCompletionSequenceNumbers(int first, int last) {
    Locker lock(_lock);
    _initialStart = first;
    _initialEnd = (last + 1) % SEQUENCE_MODULO;
}

void SafeLanding::sequenceNumberReceived(int sequenceNumber) {
    if (_trackingEntities) {
        Locker lock(_lock);
        _sequenceNumbers.insert(sequenceNumber);
    }
}

bool SafeLanding::isLoadSequenceComplete() {
    if (sequenceNumbersComplete() && entityPhysicsComplete()) {
        Locker lock(_lock);
        _trackingEntities = false;
        _trackedEntities.clear();
        qCDebug(interfaceapp) << "Safe Landing: load sequence complete";
    }

    return !_trackingEntities;
}

bool SafeLanding::sequenceNumbersComplete() {
    if (_initialStart != INVALID_SEQUENCE) {
        Locker lock(_lock);
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

bool SafeLanding::entityPhysicsComplete() {
    Locker lock(_lock);
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
