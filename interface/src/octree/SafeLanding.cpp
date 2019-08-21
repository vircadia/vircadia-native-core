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


CalculateEntityLoadingPriority SafeLanding::entityLoadingOperatorElevateCollidables = [](const EntityItem& entityItem) {
    const int COLLIDABLE_ENTITY_PRIORITY = 10.0f;
    return entityItem.getCollisionless() * COLLIDABLE_ENTITY_PRIORITY;
};

namespace {
    template<typename T> bool lessThanWraparound(int32_t a, int32_t b) {
        constexpr int32_t MAX_T_VALUE = std::numeric_limits<T>::max();
        if (b <= a) {
            b += MAX_T_VALUE;
        }
        return (b - a) < (MAX_T_VALUE / 2);
    }
}

bool SafeLanding::SequenceLessThan::operator()(const OCTREE_PACKET_SEQUENCE& a, const OCTREE_PACKET_SEQUENCE& b) const {
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
            _sequenceStart = SafeLanding::INVALID_SEQUENCE;
            _sequenceEnd = SafeLanding::INVALID_SEQUENCE;
            _sequenceNumbers.clear();
            _trackingEntities = true;
            _startTime = usecTimestampNow();

            connect(std::const_pointer_cast<EntityTree>(entityTree).get(),
                &EntityTree::addingEntity, this, &SafeLanding::addTrackedEntity, Qt::DirectConnection);
            connect(std::const_pointer_cast<EntityTree>(entityTree).get(),
                &EntityTree::deletingEntity, this, &SafeLanding::deleteTrackedEntity);

            _prevEntityLoadingPriorityOperator = EntityTreeRenderer::getEntityLoadingPriorityOperator();
            EntityTreeRenderer::setEntityLoadingPriorityFunction(entityLoadingOperatorElevateCollidables);
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

                int32_t trackedEntityCount = (int32_t)_trackedEntities.size();
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

void SafeLanding::finishSequence(OCTREE_PACKET_SEQUENCE first, OCTREE_PACKET_SEQUENCE last) {
    Locker lock(_lock);
    if (_trackingEntities) {
        _sequenceStart = first;
        _sequenceEnd = last;
    }
}

void SafeLanding::addToSequence(OCTREE_PACKET_SEQUENCE sequenceNumber) {
    Locker lock(_lock);
    _sequenceNumbers.insert(sequenceNumber);
}

void SafeLanding::updateTracking() {
    if (!_trackingEntities || !_entityTreeRenderer) {
        return;
    }

    {
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
                entityMapIter++;
            }
        }
        if (enableInterstitial) {
            _trackedEntityStabilityCount++;
        }
    }

    if (_trackedEntities.empty()) {
        // no more tracked entities --> check sequenceNumbers
        if (_sequenceStart != SafeLanding::INVALID_SEQUENCE) {
            bool shouldStop = false;
            {
                Locker lock(_lock);
                auto sequenceSize = _sequenceEnd - _sequenceStart; // this works even in rollover case
                auto startIter = _sequenceNumbers.find(_sequenceStart);
                auto endIter = _sequenceNumbers.find(_sequenceEnd - 1);

                bool missingSequenceNumbers = qApp->isMissingSequenceNumbers();
                shouldStop = (sequenceSize == 0 ||
                    (startIter != _sequenceNumbers.end() &&
                     endIter != _sequenceNumbers.end() &&
                     ((distance(startIter, endIter) == sequenceSize - 1) || !missingSequenceNumbers)));
            }
            if (shouldStop) {
                stopTracking();
            }
        }
    }
}

void SafeLanding::stopTracking() {
    Locker lock(_lock);
    if (_trackingEntities) {
        _trackingEntities = false;
        if (_entityTreeRenderer) {
            auto entityTree = _entityTreeRenderer->getTree();
            disconnect(std::const_pointer_cast<EntityTree>(entityTree).get(),
                &EntityTree::addingEntity, this, &SafeLanding::addTrackedEntity);
            disconnect(std::const_pointer_cast<EntityTree>(entityTree).get(),
                &EntityTree::deletingEntity, this, &SafeLanding::deleteTrackedEntity);
            _entityTreeRenderer.reset();
        }
        EntityTreeRenderer::setEntityLoadingPriorityFunction(_prevEntityLoadingPriorityOperator);
    }
}

void SafeLanding::reset() {
    _trackingEntities = false;
    _trackedEntities.clear();
    _maxTrackedEntityCount = 0;
    _sequenceStart = SafeLanding::INVALID_SEQUENCE;
    _sequenceEnd = SafeLanding::INVALID_SEQUENCE;
}

bool SafeLanding::trackingIsComplete() const {
    return !_trackingEntities && (_sequenceStart != SafeLanding::INVALID_SEQUENCE);
}

float SafeLanding::loadingProgressPercentage() {
    Locker lock(_lock);

    float entityReadyPercentage = 0.0f;
    if (_maxTrackedEntityCount > 0) {
        entityReadyPercentage = ((_maxTrackedEntityCount - _trackedEntities.size()) / (float)_maxTrackedEntityCount);
    }

    constexpr int32_t MINIMUM_TRACKED_ENTITY_STABILITY_COUNT = 15;
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

                // Note: the meanings of the workload regions are:
                //   R1 = in physics simulation and willing to own simulation
                //   R2 = in physics simulation but does NOT want to own simulation
                //   R3 = not in physics simulation but kinematically animated when velocities are non-zero
                //   R4 = sorted by workload and found to be outside R3
                //   UNKNOWN = known to workload but not yet sorted
                //   INVALID = not known to workload
                // So any entity sorted into R3 or R4 is definitelyNotPhysical

                bool definitelyNotPhysical = region == workload::Region::R3 ||
                    region == workload::Region::R4 ||
                    !entity->shouldBePhysical() ||
                    modelEntity->unableToLoadCollisionShape();
                bool definitelyPhysical = entity->isInPhysicsSimulation();
                return definitelyNotPhysical || definitelyPhysical;
            }
        }
    }
    return true;
}

void SafeLanding::debugDumpSequenceIDs() const {
    qCDebug(interfaceapp) << "Sequence set size:" << _sequenceNumbers.size();

    auto itr = _sequenceNumbers.begin();
    OCTREE_PACKET_SEQUENCE p = SafeLanding::INVALID_SEQUENCE;
    if (itr != _sequenceNumbers.end()) {
        p = (*itr);
        qCDebug(interfaceapp) << "First:" << (int32_t)p;
        ++itr;
        while (itr != _sequenceNumbers.end()) {
            OCTREE_PACKET_SEQUENCE s = *itr;
            if (s != p + 1) {
                qCDebug(interfaceapp) << "Gap from" << (int32_t)p << "to" << (int32_t)s << "(exclusive)";
                p = s;
            }
            ++itr;
        }
        if (p != SafeLanding::INVALID_SEQUENCE) {
            qCDebug(interfaceapp) << "Last:" << p;
        }
    }
}
