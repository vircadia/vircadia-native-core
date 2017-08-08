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

void Fork::getNextVisibleElementFirstTime(VisibleElement& next, const ViewFrustum& view) {
    // NOTE: no need to set next.intersection in the "FirstTime" context
    if (_nextIndex == -1) {
        // only get here for the root Fork at the very beginning of traversal
        // safe to assume this element intersects view
        ++_nextIndex;
        next.element = _weakElement.lock();
        return;
    } else if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && view.cubeIntersectsKeyhole(nextElement->getAACube())) {
                    next.element = nextElement;
                    return;
                }
            }
        }
    }
    next.element.reset();
}

void Fork::getNextVisibleElementAgain(VisibleElement& next, const ViewFrustum& view, uint64_t lastTime) {
    if (_nextIndex == -1) {
        // only get here for the root Fork at the very beginning of traversal
        // safe to assume this element intersects view
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        // root case is special: its intersection is always INTERSECT
        // and we can skip it if the content hasn't changed
        if (element->getLastChangedContent() > lastTime) {
            next.element = element;
            next.intersection = ViewFrustum::INTERSECT;
            return;
        }
    }
    if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement && nextElement->getLastChanged() > lastTime) {
                    ViewFrustum::intersection intersection = view.calculateCubeKeyholeIntersection(nextElement->getAACube());
                    if (intersection != ViewFrustum::OUTSIDE) {
                        next.element = nextElement;
                        next.intersection = intersection;
                        return;
                    }
                }
            }
        }
    }
    next.element.reset();
    next.intersection = ViewFrustum::OUTSIDE;
}

void Fork::getNextVisibleElementDifferential(VisibleElement& next,
        const ViewFrustum& view, const ViewFrustum& lastView, uint64_t lastTime) {
    if (_nextIndex == -1) {
        // only get here for the root Fork at the very beginning of traversal
        // safe to assume this element intersects view
        ++_nextIndex;
        EntityTreeElementPointer element = _weakElement.lock();
        // root case is special: its intersection is always INTERSECT
        // and we can skip it if the content hasn't changed
        if (element->getLastChangedContent() > lastTime) {
            next.element = element;
            next.intersection = ViewFrustum::INTERSECT;
            return;
        }
    }
    if (_nextIndex < NUMBER_OF_CHILDREN) {
        EntityTreeElementPointer element = _weakElement.lock();
        if (element) {
            while (_nextIndex < NUMBER_OF_CHILDREN) {
                EntityTreeElementPointer nextElement = element->getChildAtIndex(_nextIndex);
                ++_nextIndex;
                if (nextElement) {
                    AACube cube = nextElement->getAACube();
                    // NOTE: for differential case next.intersection is against the _completedView
                    ViewFrustum::intersection intersection = lastView.calculateCubeKeyholeIntersection(cube);
                    if ( lastView.calculateCubeKeyholeIntersection(cube) != ViewFrustum::OUTSIDE &&
                            !(intersection == ViewFrustum::INSIDE && nextElement->getLastChanged() < lastTime)) {
                        next.element = nextElement;
                        next.intersection = intersection;
                        return;
                    }
                }
            }
        }
    }
    next.element.reset();
    next.intersection = ViewFrustum::OUTSIDE;
}

EntityTreeSendThread::EntityTreeSendThread(OctreeServer* myServer, const SharedNodePointer& node)
    : OctreeSendThread(myServer, node) {
    const int32_t MIN_PATH_DEPTH = 16;
    _traversalPath.reserve(MIN_PATH_DEPTH);
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
    // BEGIN EXPERIMENTAL DIFFERENTIAL TRAVERSAL
    {
        // DEBUG HACK: trigger traversal (Again) every so often
        const uint64_t TRAVERSE_AGAIN_PERIOD = 2 * USECS_PER_SECOND;
        if (!viewFrustumChanged && usecTimestampNow() > _startOfCompletedTraversal + TRAVERSE_AGAIN_PERIOD) {
            viewFrustumChanged = true;
        }
    }
    if (nodeData->getUsesFrustum()) {
        if (viewFrustumChanged) {
            ViewFrustum viewFrustum;
            nodeData->copyCurrentViewFrustum(viewFrustum);
            EntityTreeElementPointer root = std::dynamic_pointer_cast<EntityTreeElement>(_myServer->getOctree()->getRoot());
            startNewTraversal(viewFrustum, root);
        }
    }
    if (!_traversalPath.empty()) {
        uint64_t startTime = usecTimestampNow();
        uint64_t now = startTime;

        VisibleElement next;
        getNextVisibleElement(next);
        while (next.element) {
            if (next.element->hasContent()) {
                _scanNextElementCallback(next);
            }

            // TODO: pick a reasonable budget for each partial traversal
            const uint64_t PARTIAL_TRAVERSAL_TIME_BUDGET = 100000; // usec
            now = usecTimestampNow();
            if (now - startTime > PARTIAL_TRAVERSAL_TIME_BUDGET) {
                break;
            }
            getNextVisibleElement(next);
        }

        uint64_t dt = now - startTime;
        std::cout << "adebug traversal complete " << "  Q.size = " << _sendQueue.size() << "  dt = " << dt << std::endl;  // adebug
    }
    if (!_sendQueue.empty()) {
        // print what needs to be sent
        while (!_sendQueue.empty()) {
            PrioritizedEntity entry = _sendQueue.top();
            EntityItemPointer entity = entry.getEntity();
            if (entity) {
                std::cout << "adebug    '" << entity->getName().toStdString() << "'"
                    << "  :  " << entry.getPriority() << std::endl;  // adebug
            }
            _sendQueue.pop();
            std::cout << "adebug" << std::endl;     // adebug
        }
    }
    // END EXPERIMENTAL DIFFERENTIAL TRAVERSAL

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
    // there are three types of traversal:
    //
    //      (1) FirstTime = at login --> find everything in view
    //      (2) Again = view hasn't changed --> find what has changed since last complete traversal
    //      (3) Differential = view has changed --> find what has changed or in new view but not old
    //
    // For each traversal type we define two callback lambdas:
    //
    //      _getNextVisibleElementCallback = identifies elements that need to be traversed,i
    //          updates VisibleElement ref argument with pointer-to-element and view-intersection
    //          (INSIDE, INTERSECT, or OUTSIDE)
    //
    //      _scanNextElementCallback = identifies entities that need to be appended to _sendQueue
    //
    // The _conicalView is updated here as a cached view approximation used by the lambdas for efficient
    // computation of entity sorting priorities.
    //
    if (_startOfCompletedTraversal == 0) {
        // first time
        _currentView = viewFrustum;
        _conicalView.set(_currentView);

        _getNextVisibleElementCallback = [&](VisibleElement& next) {
            _traversalPath.back().getNextVisibleElementFirstTime(next, _currentView);
        };

        _scanNextElementCallback = [&](VisibleElement& next) {
            next.element->forEachEntity([&](EntityItemPointer entity) {
                bool success = false;
                AACube cube = entity->getQueryAACube(success);
                if (success) {
                    if (_currentView.cubeIntersectsKeyhole(cube)) {
                        float priority = _conicalView.computePriority(cube);
                        _sendQueue.push(PrioritizedEntity(entity, priority));
                    }
                } else {
                    const float WHEN_IN_DOUBT_PRIORITY = 1.0f;
                    _sendQueue.push(PrioritizedEntity(entity, WHEN_IN_DOUBT_PRIORITY));
                }
            });
        };
    } else if (_currentView.isVerySimilar(viewFrustum)) {
        // again
        _getNextVisibleElementCallback = [&](VisibleElement& next) {
            _traversalPath.back().getNextVisibleElementAgain(next, _currentView, _startOfCompletedTraversal);
        };

        _scanNextElementCallback = [&](VisibleElement& next) {
            if (next.element->getLastChangedContent() > _startOfCompletedTraversal) {
                next.element->forEachEntity([&](EntityItemPointer entity) {
                    if (entity->getLastEdited() > _startOfCompletedTraversal) {
                        bool success = false;
                        AACube cube = entity->getQueryAACube(success);
                        if (success) {
                            if (next.intersection == ViewFrustum::INSIDE || _currentView.cubeIntersectsKeyhole(cube)) {
                                float priority = _conicalView.computePriority(cube);
                                _sendQueue.push(PrioritizedEntity(entity, priority));
                            }
                        } else {
                            const float WHEN_IN_DOUBT_PRIORITY = 1.0f;
                            _sendQueue.push(PrioritizedEntity(entity, WHEN_IN_DOUBT_PRIORITY));
                        }
                    }
                });
            }
        };
    } else {
        // differential
        _currentView = viewFrustum;
        _conicalView.set(_currentView);

        _getNextVisibleElementCallback = [&](VisibleElement& next) {
            _traversalPath.back().getNextVisibleElementDifferential(next, _currentView, _completedView, _startOfCompletedTraversal);
        };

        _scanNextElementCallback = [&](VisibleElement& next) {
            // NOTE: for differential case next.intersection is against _completedView not _currentView
            if (next.element->getLastChangedContent() > _startOfCompletedTraversal || next.intersection != ViewFrustum::INSIDE) {
                next.element->forEachEntity([&](EntityItemPointer entity) {
                    bool success = false;
                    AACube cube = entity->getQueryAACube(success);
                    if (success) {
                        if (_currentView.cubeIntersectsKeyhole(cube) &&
                                (entity->getLastEdited() > _startOfCompletedTraversal ||
                                    !_completedView.cubeIntersectsKeyhole(cube))) {
                            float priority = _conicalView.computePriority(cube);
                            _sendQueue.push(PrioritizedEntity(entity, priority));
                        }
                    } else {
                        const float WHEN_IN_DOUBT_PRIORITY = 1.0f;
                        _sendQueue.push(PrioritizedEntity(entity, WHEN_IN_DOUBT_PRIORITY));
                    }
                });
            }
        };
    }

    _traversalPath.clear();
    assert(root);
    _traversalPath.push_back(Fork(root));
    // set root fork's index such that root element returned at getNextElement()
    _traversalPath.back().initRootNextIndex();

    _startOfCurrentTraversal = usecTimestampNow();
}

void EntityTreeSendThread::getNextVisibleElement(VisibleElement& next) {
    if (_traversalPath.empty()) {
        next.element.reset();
        next.intersection = ViewFrustum::OUTSIDE;
        return;
    }
    _getNextVisibleElementCallback(next);
    if (next.element) {
        int8_t nextIndex = _traversalPath.back().getNextIndex();
        if (nextIndex > 0) {
            // next.element needs to be added to the path
            _traversalPath.push_back(Fork(next.element));
        }
    } else {
        // we're done at this level
        while (!next.element) {
            // pop one level
            _traversalPath.pop_back();
            if (_traversalPath.empty()) {
                // we've traversed the entire tree
                _completedView = _currentView;
                _startOfCompletedTraversal = _startOfCurrentTraversal;
                return;
            }
            // keep looking for next
            _getNextVisibleElementCallback(next);
            if (next.element) {
                // we've descended one level so add it to the path
                _traversalPath.push_back(Fork(next.element));
            }
        }
    }
}

// DEBUG method: delete later
void EntityTreeSendThread::dump() const {
    for (size_t i = 0; i < _traversalPath.size(); ++i) {
        std::cout << (int32_t)(_traversalPath[i].getNextIndex()) << "-->";
    }
}
