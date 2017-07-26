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

const float INVALID_ENTITY_SEND_PRIORITY = -1.0e-6f;

void PrioritizedEntity::updatePriority(const ViewFrustum& view) {
    EntityItemPointer entity = _weakEntity.lock();
    if (entity) {
        bool success;
        AACube cube = entity->getQueryAACube(success);
        if (success) {
            glm::vec3 center = cube.calcCenter() - view.getPosition();
            const float MIN_DISTANCE = 0.001f;
            float distanceToCenter = glm::length(center) + MIN_DISTANCE;
            float distance = distanceToCenter; //- 0.5f * cube.getScale();
            if (distance < MIN_DISTANCE) {
                // this object's bounding box overlaps the camera --> give it a big priority
                _priority = cube.getScale();
            } else {
                // NOTE: we assume view.aspectRatio < 1.0 (view width greater than height)
                // so we only check against the larger (horizontal) view angle
                float front = glm::dot(center, view.getDirection()) / distanceToCenter;
                if (front > cosf(view.getFieldOfView()) || distance < view.getCenterRadius()) {
                    _priority = cube.getScale() / distance; // + front;
                } else {
                    _priority = INVALID_ENTITY_SEND_PRIORITY;
                }
            }
        } else {
            // when in doubt give it something positive
            _priority = 1.0f;
        }
    } else {
        _priority = INVALID_ENTITY_SEND_PRIORITY;
    }
}

TreeTraversalPath::Fork::Fork(EntityTreeElementPointer& element) : _nextIndex(0) {
    assert(element);
    _weakElement = element;
}

EntityTreeElementPointer TreeTraversalPath::Fork::getNextElement(const ViewFrustum& view) {
    if (_nextIndex == -1) {
        // only get here for the TreeTraversalPath's root Fork at the very beginning of traversal
        // safe to assume this element is in view
        ++_nextIndex;
        return _weakElement.lock();
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && ViewFrustum::OUTSIDE != nextElement->computeViewIntersection(view)) {
                    return nextElement;
                }
            }
        }
    }
    return EntityTreeElementPointer();
}

EntityTreeElementPointer TreeTraversalPath::Fork::getNextElementAgain(const ViewFrustum& view, uint64_t oldTime) {
    if (_nextIndex == -1) {
        // only get here for the TreeTraversalPath's root Fork at the very beginning of traversal
        // safe to assume this element is in view
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        assert(element); // should never lose root element
        if (element->getLastChanged() < oldTime) {
            _nextIndex = NUMBER_OF_CHILDREN;
            return EntityTreeElementPointer();
        }
        if (element->getLastChanged() > oldTime) {
            return element;
        }
    }
    if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && nextElement->getLastChanged() > oldTime &&
                    ViewFrustum::OUTSIDE != nextElement->computeViewIntersection(view)) {
                    return nextElement;
                }
            }
        }
    }
    return EntityTreeElementPointer();
}

EntityTreeElementPointer TreeTraversalPath::Fork::getNextElementDelta(const ViewFrustum& newView, const ViewFrustum& oldView, uint64_t oldTime) {
    if (_nextIndex == -1) {
        // only get here for the TreeTraversalPath's root Fork at the very beginning of traversal
        // safe to assume this element is in newView
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        assert(element); // should never lose root element
        if (element->getLastChanged() < oldTime) {
            _nextIndex = NUMBER_OF_CHILDREN;
            return EntityTreeElementPointer();
        }
        return element;
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement &&
                        !(nextElement->getLastChanged() < oldTime &&
                            ViewFrustum::INSIDE == nextElement->computeViewIntersection(oldView)) &&
                        ViewFrustum::OUTSIDE != nextElement->computeViewIntersection(newView)) {
                    return nextElement;
                }
            }
        }
    }
    return EntityTreeElementPointer();
}

TreeTraversalPath::TreeTraversalPath() {
    const int32_t MIN_PATH_DEPTH = 16;
    _forks.reserve(MIN_PATH_DEPTH);
    _traversalCallback = std::bind(&TreeTraversalPath::traverseFirstTime, this);
}

void TreeTraversalPath::startNewTraversal(const ViewFrustum& view, EntityTreeElementPointer root) {
    if (_startOfLastCompletedTraversal == 0) {
        _traversalCallback = std::bind(&TreeTraversalPath::traverseFirstTime, this);
        _currentView = view;
    } else if (_currentView.isVerySimilar(view)) {
        _traversalCallback = std::bind(&TreeTraversalPath::traverseAgain, this);
    } else {
        _currentView = view;
        _traversalCallback = std::bind(&TreeTraversalPath::traverseDelta, this);
    }
    _forks.clear();
    if (root) {
        _forks.push_back(Fork(root));
        // set root fork's index such that root element returned at getNextElement()
        _forks.back().initRootNextIndex();
    }
    _startOfCurrentTraversal = usecTimestampNow();
}

EntityTreeElementPointer TreeTraversalPath::traverseFirstTime() {
    return _forks.back().getNextElement(_currentView);
}

EntityTreeElementPointer TreeTraversalPath::traverseAgain() {
    return _forks.back().getNextElementAgain(_currentView, _startOfLastCompletedTraversal);
}

EntityTreeElementPointer TreeTraversalPath::traverseDelta() {
    return _forks.back().getNextElementDelta(_currentView, _lastCompletedView, _startOfLastCompletedTraversal);
}

EntityTreeElementPointer TreeTraversalPath::getNextElement() {
    if (_forks.empty() || !_traversalCallback) {
        return EntityTreeElementPointer();
    }
    EntityTreeElementPointer nextElement = _traversalCallback();
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
                _lastCompletedView = _currentView;
                _startOfLastCompletedTraversal = _startOfCurrentTraversal;
                return nextElement;
            }
            // keep looking for nextElement
            nextElement = _traversalCallback();
            if (nextElement) {
                // we've descended one level so add it to the path
                _forks.push_back(Fork(nextElement));
            }
        }
    }
    return nextElement;
}

void TreeTraversalPath::dump() const {
    for (size_t i = 0; i < _forks.size(); ++i) {
        std::cout << (int)(_forks[i].getNextIndex()) << "-->";
    }
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

static size_t adebug = 0;

void EntityTreeSendThread::traverseTreeAndSendContents(SharedNodePointer node, OctreeQueryNode* nodeData,
            bool viewFrustumChanged, bool isFullScene) {
    if (viewFrustumChanged) {
        ViewFrustum view;
        nodeData->copyCurrentViewFrustum(view);
        EntityTreeElementPointer root = std::dynamic_pointer_cast<EntityTreeElement>(_myServer->getOctree()->getRoot());
        _path.startNewTraversal(view, root);

        std::cout << "adebug reset view" << std::endl;  // adebug
        adebug = 0;
    }
    if (!_path.empty()) {
        int32_t numElements = 0;
        uint64_t t0 = usecTimestampNow();
        uint64_t now = t0;

        QVector<EntityItemPointer> entities;
        EntityTreeElementPointer nextElement = _path.getNextElement();
        while (nextElement) {
            nextElement->getEntities(_path.getView(), entities);
            ++numElements;

            now = usecTimestampNow();
            const uint64_t PARTIAL_TRAVERSAL_TIME_BUDGET = 80; // usec
            if (now - t0 > PARTIAL_TRAVERSAL_TIME_BUDGET) {
                break;
            }
            nextElement = _path.getNextElement();
        }
        uint64_t dt1 = now - t0;
        for (EntityItemPointer& entity : entities) {
            PrioritizedEntity entry(entity);
            entry.updatePriority(_path.getView());
            if (entry.getPriority() > INVALID_ENTITY_SEND_PRIORITY) {
                _sendQueue.push(entry);
            }
        }
        adebug += entities.size();
        std::cout << "adebug  traverseTreeAndSendContents totalEntities = " << adebug
            << "  numElements = " << numElements
            << "  numEntities = " << entities.size()
            << "  dt = " << dt1 << std::endl;  // adebug
    } else if (!_sendQueue.empty()) {
        while (!_sendQueue.empty()) {
            PrioritizedEntity entry = _sendQueue.top();
            EntityItemPointer entity = entry.getEntity();
            if (entity) {
                std::cout << "adebug  traverseTreeAndSendContents()  " << entry.getPriority()
                    << "  '" << entity->getName().toStdString() << "'"
                    << std::endl;  // adebug
            }
            _sendQueue.pop();
        }
        // std::priority_queue doesn't have a clear method,
        // so we "clear" _sendQueue by setting it equal to an empty queue
        _sendQueue = EntityPriorityQueue();
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


