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
#include <SharedUtil.h>

#include "EntityTreeRenderer.h"
#include "RenderableModelEntityItem.h"
#include "InterfaceLogging.h"
#include "Application.h"

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

void SafeLanding::startTracking(QSharedPointer<EntityTreeRenderer> entityTreeRenderer) {
    if (!entityTreeRenderer.isNull()) {
        auto entityTree = entityTreeRenderer->getTree();
        if (entityTree && !_trackingEntities) {
            Locker lock(_lock);
            _entityTreeRenderer = entityTreeRenderer;
            _trackedEntities.clear();
            _maxTrackedEntityCount = 0;
            _initialStart = INVALID_SEQUENCE;
            _initialEnd = INVALID_SEQUENCE;
            _sequenceNumbers.clear();
            _trackingEntities = true;
            _startTime = usecTimestampNow();

            connect(std::const_pointer_cast<EntityTree>(entityTree).get(),
                &EntityTree::addingEntity, this, &SafeLanding::addTrackedEntity, Qt::DirectConnection);
            connect(std::const_pointer_cast<EntityTree>(entityTree).get(),
                &EntityTree::deletingEntity, this, &SafeLanding::deleteTrackedEntity);

            EntityTreeRenderer::setEntityLoadingPriorityFunction(&ElevatedPriority);
        }
    }
}

void SafeLanding::addTrackedEntity(const EntityItemID& entityID) {
    if (_trackingEntities && _entityTreeRenderer) {
        Locker lock(_lock);
        auto entityTree = _entityTreeRenderer->getTree();
        if (entityTree) {
            EntityItemPointer entity = entityTree->findEntityByID(entityID);
            if (entity && !entity->isLocalEntity() && entity->getCreated() < _startTime) {
                _trackedEntities.emplace(entityID, entity);

                int trackedEntityCount = (int)_trackedEntities.size();
                if (trackedEntityCount > _maxTrackedEntityCount) {
                    _maxTrackedEntityCount = trackedEntityCount;
                    _trackedEntityStabilityCount = 0;
                }
            }
        }
    }
}

void SafeLanding::deleteTrackedEntity(const EntityItemID& entityID) {
    Locker lock(_lock);
    _trackedEntities.erase(entityID);
}

void SafeLanding::finishSequence(int first, int last) {
    Locker lock(_lock);
    if (_trackingEntities) {
        _initialStart = first;
        _initialEnd = last;
    }
}

void SafeLanding::addToSequence(int sequenceNumber) {
    Locker lock(_lock);
    if (_trackingEntities) {
        _sequenceNumbers.insert(sequenceNumber);
    }
}

void SafeLanding::updateTracking() {
    if (!_trackingEntities || !_entityTreeRenderer) {
        return;
    }
    Locker lock(_lock);

    bool enableInterstitial = DependencyManager::get<NodeList>()->getDomainHandler().getInterstitialModeEnabled();

    auto entityMapIter = _trackedEntities.begin();
    while (entityMapIter != _trackedEntities.end()) {
        auto entity = entityMapIter->second;
        bool isVisuallyReady = true;
        if (enableInterstitial) {
            auto entityRenderable = _entityTreeRenderer->renderableForEntityId(entityMapIter->first);
            if (!entityRenderable) {
                _entityTreeRenderer->addingEntity(entityMapIter->first);
            }

            isVisuallyReady = entity->isVisuallyReady() || (!entityRenderable && !entity->isParentPathComplete());
        }
        if (isEntityPhysicsReady(entity) && isVisuallyReady) {
            entityMapIter = _trackedEntities.erase(entityMapIter);
        } else {
            if (!isVisuallyReady) {
                entity->requestRenderUpdate();
            }
            entityMapIter++;
        }
    }

    if (enableInterstitial) {
        _trackedEntityStabilityCount++;
    }

    if (_trackedEntities.empty()) {
        // no more tracked entities --> check sequenceNumbers
        if (_initialStart != INVALID_SEQUENCE) {
            Locker lock(_lock);
            int sequenceSize = _initialStart <= _initialEnd ? _initialEnd - _initialStart:
                _initialEnd + SEQUENCE_MODULO - _initialStart;
            auto startIter = _sequenceNumbers.find(_initialStart);
            auto endIter = _sequenceNumbers.find(_initialEnd - 1);

            bool missingSequenceNumbers = qApp->isMissingSequenceNumbers();
            if (sequenceSize == 0 ||
                    (startIter != _sequenceNumbers.end() &&
                     endIter != _sequenceNumbers.end() &&
                     ((distance(startIter, endIter) == sequenceSize - 1) || !missingSequenceNumbers))) {
                stopTracking();
            }
        }
    }
}

void SafeLanding::stopTracking() {
    Locker lock(_lock);
    _trackingEntities = false;
    if (_entityTreeRenderer) {
        auto entityTree = _entityTreeRenderer->getTree();
        disconnect(std::const_pointer_cast<EntityTree>(entityTree).get(),
            &EntityTree::addingEntity, this, &SafeLanding::addTrackedEntity);
        disconnect(std::const_pointer_cast<EntityTree>(entityTree).get(),
            &EntityTree::deletingEntity, this, &SafeLanding::deleteTrackedEntity);
        _entityTreeRenderer.reset();
    }
    EntityTreeRenderer::setEntityLoadingPriorityFunction(StandardPriority);
}

bool SafeLanding::trackingIsComplete() const {
    return !_trackingEntities && (_initialStart != INVALID_SEQUENCE);
}

float SafeLanding::loadingProgressPercentage() {
    Locker lock(_lock);
    static const int MINIMUM_TRACKED_ENTITY_STABILITY_COUNT = 15;

    float entityReadyPercentage = 0.0f;
    if (_maxTrackedEntityCount > 0) {
        entityReadyPercentage = ((_maxTrackedEntityCount - _trackedEntities.size()) / (float)_maxTrackedEntityCount);
    }

    if (_trackedEntityStabilityCount < MINIMUM_TRACKED_ENTITY_STABILITY_COUNT) {
        entityReadyPercentage *= 0.20f;
    }

    return entityReadyPercentage;
}

bool SafeLanding::isEntityPhysicsReady(const EntityItemPointer& entity) {
    if (entity && !entity->getCollisionless()) {
        const auto& entityType = entity->getType();
        if (entityType == EntityTypes::Model) {
            RenderableModelEntityItem * modelEntity = std::dynamic_pointer_cast<RenderableModelEntityItem>(entity).get();
            static const std::set<ShapeType> downloadedCollisionTypes
                { SHAPE_TYPE_COMPOUND, SHAPE_TYPE_SIMPLE_COMPOUND, SHAPE_TYPE_STATIC_MESH,  SHAPE_TYPE_SIMPLE_HULL };
            bool hasAABox;
            entity->getAABox(hasAABox);
            if (hasAABox && downloadedCollisionTypes.count(modelEntity->getShapeType()) != 0) {
                auto space = _entityTreeRenderer->getWorkloadSpace();
                uint8_t region = space ? space->getRegion(entity->getSpaceIndex()) : (uint8_t)workload::Region::INVALID;
                bool shouldBePhysical = region < workload::Region::R3 && entity->shouldBePhysical();
                return (!shouldBePhysical || entity->isInPhysicsSimulation() || modelEntity->computeShapeFailedToLoad());
            }
        }
    }
    return true;
}

float SafeLanding::ElevatedPriority(const EntityItem& entityItem) {
    return entityItem.getCollisionless() ? 0.0f : 10.0f;
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
