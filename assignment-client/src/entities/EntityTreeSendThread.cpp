//
//  EntityTreeSendThread.cpp
//  assignment-client/src/entities
//
//  Created by Stephen Birarda on 2/15/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityTreeSendThread.h"
#include <iostream> // adebug

#include <EntityNodeData.h>
#include <EntityTypes.h>

#include "EntityServer.h"

const float DO_NOT_SEND = -1.0e-6f;

void ConicalView::set(const ViewFrustum& viewFrustum) {
    // The ConicalView has two parts: a central sphere (same as ViewFrustm) and a circular cone that bounds the frustum part.
    // Why?  Because approximate intersection tests are much faster to compute for a cone than for a frustum.
    _position = viewFrustum.getPosition();
    _direction = viewFrustum.getDirection();

    // We cache the sin and cos of the half angle of the cone that bounds the frustum.
    // (the math here is left as an exercise for the reader)
    float A = viewFrustum.getAspectRatio();
    float t = tanf(0.5f * viewFrustum.getFieldOfView());
    _cosAngle = 1.0f / sqrtf(1.0f + (A * A + 1.0f) * (t * t));
    _sinAngle = sqrtf(1.0f - _cosAngle * _cosAngle);

    _radius = viewFrustum.getCenterRadius();
}

float ConicalView::computePriority(const AACube& cube) const {
    glm::vec3 p = cube.calcCenter() - _position; // position of bounding sphere in view-frame
    float d = glm::length(p); // distance to center of bounding sphere
    float r = 0.5f * cube.getScale(); // radius of bounding sphere
    if (d < _radius + r) {
        return r;
    }
    if (glm::dot(p, _direction) > sqrtf(d * d - r * r) * _cosAngle - r * _sinAngle) {
        const float AVOID_DIVIDE_BY_ZERO = 0.001f;
        return r / (d + AVOID_DIVIDE_BY_ZERO);
    }
    return DO_NOT_SEND;
}


// static
float ConicalView::computePriority(const EntityItemPointer& entity) const {
    assert(entity);
    bool success;
    AACube cube = entity->getQueryAACube(success);
    if (success) {
        return computePriority(cube);
    } else {
        // when in doubt give it something positive
        return 1.0f;
    }
}

float PrioritizedEntity::updatePriority(const ConicalView& conicalView) {
    EntityItemPointer entity = _weakEntity.lock();
    if (entity) {
        _priority = conicalView.computePriority(entity);
    } else {
        _priority = DO_NOT_SEND;
    }
    return _priority;
}

Fork::Fork(EntityTreeElementPointer& element) : _nextIndex(0) {
    assert(element);
    _weakElement = element;
}

EntityTreeElementPointer Fork::getNextElementFirstTime(const ViewFrustum& view) {
    if (_nextIndex == -1) {
        // only get here for the root Fork at the very beginning of traversal
        // safe to assume this element is in view
        ++_nextIndex;
        return _weakElement.lock();
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && view.cubeIntersectsKeyhole(nextElement->getAACube())) {
                    return nextElement;
                }
            }
        }
    }
    return EntityTreeElementPointer();
}

EntityTreeElementPointer Fork::getNextElementAgain(const ViewFrustum& view, uint64_t lastTime) {
    if (_nextIndex == -1) {
        // only get here for the root Fork at the very beginning of traversal
        // safe to assume this element is in view
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        assert(element); // should never lose root element
        return element;
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && nextElement->getLastChanged() > lastTime &&
                    nextElement && view.cubeIntersectsKeyhole(nextElement->getAACube())) {
                    return nextElement;
                }
            }
        }
    }
    return EntityTreeElementPointer();
}

EntityTreeElementPointer Fork::getNextElementDifferential(const ViewFrustum& view, const ViewFrustum& lastView, uint64_t lastTime) {
    if (_nextIndex == -1) {
        // only get here for the root Fork at the very beginning of traversal
        // safe to assume this element is in view
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        assert(element); // should never lose root element
        return element;
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement &&
                        (!(nextElement->getLastChanged() < lastTime &&
                            ViewFrustum::INSIDE == lastView.calculateCubeKeyholeIntersection(nextElement->getAACube()))) &&
                        ViewFrustum::OUTSIDE != view.calculateCubeKeyholeIntersection(nextElement->getAACube())) {
                    return nextElement;
                }
            }
        }
    }
    return EntityTreeElementPointer();
}

EntityTreeSendThread::EntityTreeSendThread(OctreeServer* myServer, const SharedNodePointer& node)
    : OctreeSendThread(myServer, node) {
    const int32_t MIN_PATH_DEPTH = 16;
    _forks.reserve(MIN_PATH_DEPTH);
}

void EntityTreeSendThread::preDistributionProcessing() {
    auto node = _node.toStrongRef();
    auto nodeData = static_cast<EntityNodeData*>(node->getLinkedData());

    if (nodeData) {
        auto jsonQuery = nodeData->getJSONParameters();

        // check if we have a JSON query with flags
        auto flags = jsonQuery[EntityJSONQueryProperties::FLAGS_PROPERTY].toObject();
        if (!flags.isEmpty()) {
            // check the flags object for specific flags that require special pre-processing

            bool includeAncestors = flags[EntityJSONQueryProperties::INCLUDE_ANCESTORS_PROPERTY].toBool();
            bool includeDescendants = flags[EntityJSONQueryProperties::INCLUDE_DESCENDANTS_PROPERTY].toBool();

            if (includeAncestors || includeDescendants) {
                // we need to either include the ancestors, descendants, or both for entities matching the filter
                // included in the JSON query

                // first reset our flagged extra entities so we start with an empty set
                nodeData->resetFlaggedExtraEntities();

                auto entityTree = std::static_pointer_cast<EntityTree>(_myServer->getOctree());

                bool requiresFullScene = false;

                // enumerate the set of entity IDs we know currently match the filter
                foreach(const QUuid& entityID, nodeData->getSentFilteredEntities()) {
                    if (includeAncestors) {
                        // we need to include ancestors - recurse up to reach them all and add their IDs
                        // to the set of extra entities to include for this node
                        entityTree->withReadLock([&]{
                            auto filteredEntity = entityTree->findEntityByID(entityID);
                            if (filteredEntity) {
                                requiresFullScene |= addAncestorsToExtraFlaggedEntities(entityID, *filteredEntity, *nodeData);
                            }
                        });
                    }

                    if (includeDescendants) {
                        // we need to include descendants - recurse down to reach them all and add their IDs
                        // to the set of extra entities to include for this node
                        entityTree->withReadLock([&]{
                            auto filteredEntity = entityTree->findEntityByID(entityID);
                            if (filteredEntity) {
                                requiresFullScene |= addDescendantsToExtraFlaggedEntities(entityID, *filteredEntity, *nodeData);
                            }
                        });
                    }
                }

                if (requiresFullScene) {
                    // for one or more of the entities matching our filter we found new extra entities to include

                    // because it is possible that one of these entities hasn't changed since our last send
                    // and therefore would not be recursed to, we need to force a full traversal for this pass
                    // of the tree to allow it to grab all of the extra entities we're asking it to include
                    nodeData->setShouldForceFullScene(requiresFullScene);
                }
            }
        }
    }
}

void EntityTreeSendThread::traverseTreeAndSendContents(SharedNodePointer node, OctreeQueryNode* nodeData,
            bool viewFrustumChanged, bool isFullScene) {
    if (nodeData->getUsesFrustum()) {
        if (viewFrustumChanged) {
            ViewFrustum viewFrustum;
            nodeData->copyCurrentViewFrustum(viewFrustum);
            EntityTreeElementPointer root = std::dynamic_pointer_cast<EntityTreeElement>(_myServer->getOctree()->getRoot());
            startNewTraversal(viewFrustum, root);
        }
    }
    if (!_forks.empty()) {
        uint64_t t0 = usecTimestampNow();
        uint64_t now = t0;

        ConicalView conicalView(_currentView);
        EntityTreeElementPointer nextElement = getNextElement();
        while (nextElement) {
            nextElement->forEachEntity([&](EntityItemPointer entity) {
                bool success = false;
                AACube cube = entity->getQueryAACube(success);
                if (success) {
                    if (_currentView.cubeIntersectsKeyhole(cube)) {
                        float priority = conicalView.computePriority(cube);
                        _sendQueue.push(PrioritizedEntity(entity, priority));
                        std::cout << "adebug      '" << entity->getName().toStdString() << "'  send = " << (priority != DO_NOT_SEND) << std::endl;     // adebug
                    } else {
                        std::cout << "adebug      '" << entity->getName().toStdString() << "'  out of view" << std::endl;     // adebug
                    }
                } else {
                    const float WHEN_IN_DOUBT_PRIORITY = 1.0f;
                    _sendQueue.push(PrioritizedEntity(entity, WHEN_IN_DOUBT_PRIORITY));
                }
            });

            now = usecTimestampNow();
            const uint64_t PARTIAL_TRAVERSAL_TIME_BUDGET = 100000; // usec
            if (now - t0 > PARTIAL_TRAVERSAL_TIME_BUDGET) {
                break;
            }
            nextElement = getNextElement();
        }
        uint64_t dt1 = now - t0;

    //} else if (!_sendQueue.empty()) {
        size_t sendQueueSize = _sendQueue.size();
        while (!_sendQueue.empty()) {
            PrioritizedEntity entry = _sendQueue.top();
            EntityItemPointer entity = entry.getEntity();
            if (entity) {
                std::cout << "adebug    '" << entity->getName().toStdString() << "'"
                    << "  :  " << entry.getPriority() << std::endl;  // adebug
            }
            _sendQueue.pop();
        }
        // std::priority_queue doesn't have a clear method,
        // so we "clear" _sendQueue by setting it equal to an empty queue
        _sendQueue = EntityPriorityQueue();
        std::cout << "adebug -end"
            << "  Q.size = " << sendQueueSize
            << "  dt = " << dt1 << std::endl;  // adebug
        std::cout << "adebug" << std::endl;     // adebug
    }

    OctreeSendThread::traverseTreeAndSendContents(node, nodeData, viewFrustumChanged, isFullScene);
}

bool EntityTreeSendThread::addAncestorsToExtraFlaggedEntities(const QUuid& filteredEntityID,
                                                              EntityItem& entityItem, EntityNodeData& nodeData) {
    // check if this entity has a parent that is also an entity
    bool success = false;
    auto entityParent = entityItem.getParentPointer(success);

    if (success && entityParent && entityParent->getNestableType() == NestableType::Entity) {
        // we found a parent that is an entity item

        // first add it to the extra list of things we need to send
        bool parentWasNew = nodeData.insertFlaggedExtraEntity(filteredEntityID, entityParent->getID());

        // now recursively call ourselves to get its ancestors added too
        auto parentEntityItem = std::static_pointer_cast<EntityItem>(entityParent);
        bool ancestorsWereNew = addAncestorsToExtraFlaggedEntities(filteredEntityID, *parentEntityItem, nodeData);

        // return boolean if our parent or any of our ancestors were new additions (via insertFlaggedExtraEntity)
        return parentWasNew || ancestorsWereNew;
    }

    // since we didn't have a parent niether of our parents or ancestors could be new additions
    return false;
}

bool EntityTreeSendThread::addDescendantsToExtraFlaggedEntities(const QUuid& filteredEntityID,
                                                                EntityItem& entityItem, EntityNodeData& nodeData) {
    bool hasNewChild = false;
    bool hasNewDescendants = false;

    // enumerate the immediate children of this entity
    foreach (SpatiallyNestablePointer child, entityItem.getChildren()) {

        if (child && child->getNestableType() == NestableType::Entity) {
            // this is a child that is an entity

            // first add it to the extra list of things we need to send
            hasNewChild |= nodeData.insertFlaggedExtraEntity(filteredEntityID, child->getID());

            // now recursively call ourselves to get its descendants added too
            auto childEntityItem = std::static_pointer_cast<EntityItem>(child);
            hasNewDescendants |= addDescendantsToExtraFlaggedEntities(filteredEntityID, *childEntityItem, nodeData);
        }
    }

    // return our boolean indicating if we added new children or descendants as extra entities to send
    // (via insertFlaggedExtraEntity)
    return hasNewChild || hasNewDescendants;
}

void EntityTreeSendThread::startNewTraversal(const ViewFrustum& viewFrustum, EntityTreeElementPointer root) {
    if (_startOfCompletedTraversal == 0) {
        _currentView = viewFrustum;
        _getNextElementCallback = [&]() {
            return _forks.back().getNextElementFirstTime(_currentView);
        };

    } else if (_currentView.isVerySimilar(viewFrustum)) {
        _getNextElementCallback = [&]() {
            return _forks.back().getNextElementAgain(_currentView, _startOfCompletedTraversal);
        };
    } else {
        _currentView = viewFrustum;
        _getNextElementCallback = [&]() {
            return _forks.back().getNextElementDifferential(_currentView, _completedView, _startOfCompletedTraversal);
        };
    }

    _forks.clear();
    assert(root);
    _forks.push_back(Fork(root));
    // set root fork's index such that root element returned at getNextElement()
    _forks.back().initRootNextIndex();

    _startOfCurrentTraversal = usecTimestampNow();
}

EntityTreeElementPointer EntityTreeSendThread::getNextElement() {
    if (_forks.empty()) {
        return EntityTreeElementPointer();
    }
    EntityTreeElementPointer nextElement = _getNextElementCallback();
    if (nextElement) {
        int8_t nextIndex = _forks.back().getNextIndex();
        if (nextIndex > 0) {
            // nextElement needs to be added to the path
            _forks.push_back(Fork(nextElement));
        }
    } else {
        // we're done at this level
        while (!nextElement) {
            // pop one level
            _forks.pop_back();
            if (_forks.empty()) {
                // we've traversed the entire tree
                _completedView = _currentView;
                _startOfCompletedTraversal = _startOfCurrentTraversal;
                return nextElement;
            }
            // keep looking for nextElement
            nextElement = _getNextElementCallback();
            if (nextElement) {
                // we've descended one level so add it to the path
                _forks.push_back(Fork(nextElement));
            }
        }
    }
    return nextElement;
}

void EntityTreeSendThread::dump() const {
    for (size_t i = 0; i < _forks.size(); ++i) {
        std::cout << (int32_t)(_forks[i].getNextIndex()) << "-->";
    }
}
