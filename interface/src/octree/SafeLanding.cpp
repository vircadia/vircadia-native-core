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
#include "model-networking/ModelCache.h"
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

SafeLanding::SafeLanding() {
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
    volatile EntityItemID id = entityID;
    if (_trackingEntities) {
        EntityItemPointer entity = _entityTree->findEntityByID(entityID);
        if (entity) {
            const auto& entityType = entity->getType();
            // Entity types of interest:
            static const std::set<EntityTypes::EntityType> solidTypes
                { EntityTypes::Box, EntityTypes::Sphere, EntityTypes::Shape, EntityTypes::Model };

            if (solidTypes.count(entity->getType()) && !entity->getCollisionless()) {
                _trackedEntities.emplace(entityID, entity);
                trackResources(entity);
                qCDebug(interfaceapp) << "Tracking entity " << entity->getItemName();
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

void SafeLanding::trackResources(EntityItemPointer entity) {
    if (entity->getType() == EntityTypes::Model) {
        ModelEntityItem * modelEntity = std::dynamic_pointer_cast<ModelEntityItem>(entity).get();
        QString resourceURL;
        QSharedPointer<Resource> resource;
        auto shapeType = modelEntity->getShapeType();

        if (shapeType == SHAPE_TYPE_COMPOUND) {
            resourceURL = modelEntity->getCompoundShapeURL();
            resource = DependencyManager::get<ModelCache>()->getCollisionGeometryResource(resourceURL);
        } else if (shapeType == SHAPE_TYPE_STATIC_MESH || shapeType == SHAPE_TYPE_SIMPLE_COMPOUND) {
            resourceURL = modelEntity->getModelURL();
            resource = DependencyManager::get<ModelCache>()->getGeometryResource(resourceURL);
        }

        if (resource) {
            connect(resource.data(), &Resource::loaded, this, &SafeLanding::resourceLoaded);
            // Remove it either way:
            connect(resource.data(), &Resource::failed, this, &SafeLanding::resourceLoaded);
            if (!resource->isLoaded()) {
                _trackedURLs.insert(resourceURL);
            }
        }
    }
}

void SafeLanding::resourceLoaded() {
    QObject * sender = QObject::sender();
    if (sender) {
        Resource * resource = dynamic_cast<Resource*>(sender);
        const QString resourceURL = resource->getURL().toString();
        _trackedURLs.erase(resourceURL);
        qCDebug(interfaceapp) << "Removed tracked URL" << resourceURL;
    }
}

bool SafeLanding::isLoadSequenceComplete() {
    if (sequenceNumbersComplete() && _trackedURLs.empty()) {
        _trackingEntities = false;
        _trackedEntities.clear();
    }

    return !_trackingEntities;
}

void SafeLanding::DebugDumpSequenceIDs() const {
    int p = -1;
    qCDebug(interfaceapp) << "Sequence set size:" << _sequenceNumbers.size();
    for (auto s: _sequenceNumbers) {
        if (p == -1) {
            p = s;
            qCDebug(interfaceapp) << "First: " << s;
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
