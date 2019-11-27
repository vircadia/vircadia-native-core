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

#include <EntityNodeData.h>
#include <EntityTypes.h>
#include <OctreeUtils.h>

#include "EntityServer.h"

EntityTreeSendThread::EntityTreeSendThread(OctreeServer* myServer, const SharedNodePointer& node) :
    OctreeSendThread(myServer, node)
{
    connect(std::static_pointer_cast<EntityTree>(myServer->getOctree()).get(), &EntityTree::editingEntityPointer, this, &EntityTreeSendThread::editingEntityPointer, Qt::QueuedConnection);
    connect(std::static_pointer_cast<EntityTree>(myServer->getOctree()).get(), &EntityTree::deletingEntityPointer, this, &EntityTreeSendThread::deletingEntityPointer, Qt::QueuedConnection);

    // connect to connection ID change on EntityNodeData so we can clear state for this receiver
    auto nodeData = static_cast<EntityNodeData*>(node->getLinkedData());
    connect(nodeData, &EntityNodeData::incomingConnectionIDChanged, this, &EntityTreeSendThread::resetState);
}

void EntityTreeSendThread::resetState() {
    qCDebug(entities) << "Clearing known EntityTreeSendThread state for" << _nodeUuid;

    _knownState.clear();
    _traversal.reset();
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

bool EntityTreeSendThread::traverseTreeAndSendContents(SharedNodePointer node, OctreeQueryNode* nodeData,
            bool viewFrustumChanged, bool isFullScene) {
    if (viewFrustumChanged || _traversal.finished()) {
        EntityTreeElementPointer root = std::dynamic_pointer_cast<EntityTreeElement>(_myServer->getOctree()->getRoot());


        DiffTraversal::View newView;
        newView.viewFrustums = nodeData->getCurrentViews();

        int32_t lodLevelOffset = nodeData->getBoundaryLevelAdjust() + (viewFrustumChanged ? LOW_RES_MOVING_ADJUST : NO_BOUNDARY_ADJUST);
        newView.lodScaleFactor = powf(2.0f, lodLevelOffset);
        
        startNewTraversal(newView, root, isFullScene);

        // When the viewFrustum changed the sort order may be incorrect, so we re-sort
        // and also use the opportunity to cull anything no longer in view
        if (viewFrustumChanged && !_sendQueue.empty()) {
            EntityPriorityQueue prevSendQueue;
            std::swap(_sendQueue, prevSendQueue);
            assert(_sendQueue.empty());

            // Re-add elements from previous traversal if they still need to be sent
            while (!prevSendQueue.empty()) {
                EntityItemPointer entity = prevSendQueue.top().getEntity();
                bool forceRemove = prevSendQueue.top().shouldForceRemove();
                prevSendQueue.pop();
                if (entity) {
                    float priority = PrioritizedEntity::DO_NOT_SEND;

                    if (forceRemove) {
                        priority = PrioritizedEntity::FORCE_REMOVE;
                    } else {
                        const auto& view = _traversal.getCurrentView();
                        priority = view.computePriority(entity);
                    }

                    if (priority != PrioritizedEntity::DO_NOT_SEND) {
                        _sendQueue.emplace(entity, priority, forceRemove);
                    }
                }
            }
        }
    }

    if (!_traversal.finished()) {
        quint64 startTime = usecTimestampNow();

        #ifdef DEBUG
        const uint64_t TIME_BUDGET = 400; // usec
        #else
        const uint64_t TIME_BUDGET = 200; // usec
        #endif
        _traversal.traverse(TIME_BUDGET);
        OctreeServer::trackTreeTraverseTime((float)(usecTimestampNow() - startTime));
    }

    bool sendComplete = OctreeSendThread::traverseTreeAndSendContents(node, nodeData, viewFrustumChanged, isFullScene);

    if (sendComplete && nodeData->wantReportInitialCompletion() && _traversal.finished()) {
        // Dealt with all nearby entities.
        nodeData->setReportInitialCompletion(false);
        // initial stats and entity packets are reliable until the initial query is complete
        // to guarantee all entity data is available for safe landing/physics start.  Afterwards
        // the packets are unreliable for performance.
        nodeData->stats.getStatsMessage().setReliable(false);
        nodeData->getPacket().setReliable(false);

        // Send EntityQueryInitialResultsComplete reliable packet ...
        auto initialCompletion = NLPacket::create(PacketType::EntityQueryInitialResultsComplete,
            sizeof(OCTREE_PACKET_SEQUENCE), true);
        initialCompletion->writePrimitive(OCTREE_PACKET_SEQUENCE(nodeData->getSequenceNumber()));
        DependencyManager::get<NodeList>()->sendPacket(std::move(initialCompletion), *node);
    }

    return sendComplete;
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

    // since we didn't have a parent, neither of our parents or ancestors could be new additions
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

void EntityTreeSendThread::startNewTraversal(const DiffTraversal::View& view, EntityTreeElementPointer root,
                                             bool forceFirstPass) {

    DiffTraversal::Type type = _traversal.prepareNewTraversal(view, root, forceFirstPass);
    // there are three types of traversal:
    //
    //      (1) FirstTime = at login --> find everything in view
    //      (2) Repeat = view hasn't changed --> find what has changed since last complete traversal
    //      (3) Differential = view has changed --> find what has changed or in new view but not old
    //
    // The "scanCallback" we provide to the traversal depends on the type:

    switch (type) {
        case DiffTraversal::First:
            // When we get to a First traversal, clear the _knownState
            _knownState.clear();
            _traversal.setScanCallback([this](DiffTraversal::VisibleElement& next) {
                next.element->forEachEntity([&](EntityItemPointer entity) {
                    // Bail early if we've already checked this entity this frame
                    if (_sendQueue.contains(entity.get())) {
                        return;
                    }
                    const auto& view = _traversal.getCurrentView();
                    float priority = view.computePriority(entity);

                    if (priority != PrioritizedEntity::DO_NOT_SEND) {
                        _sendQueue.emplace(entity, priority);
                    }
                });
            });
            break;
        case DiffTraversal::Repeat:
            _traversal.setScanCallback([this](DiffTraversal::VisibleElement& next) {
                uint64_t startOfCompletedTraversal = _traversal.getStartOfCompletedTraversal();
                if (next.element->getLastChangedContent() > startOfCompletedTraversal) {
                    next.element->forEachEntity([&](EntityItemPointer entity) {
                        // Bail early if we've already checked this entity this frame
                        if (_sendQueue.contains(entity.get())) {
                            return;
                        }
                        float priority = PrioritizedEntity::DO_NOT_SEND;

                        auto knownTimestamp = _knownState.find(entity.get());
                        if (knownTimestamp == _knownState.end()) {
                            const auto& view = _traversal.getCurrentView();
                            priority = view.computePriority(entity);

                        } else if (entity->getLastEdited() > knownTimestamp->second ||
                                   entity->getLastChangedOnServer() > knownTimestamp->second) {
                            // it is known and it changed --> put it on the queue with any priority
                            // TODO: sort these correctly
                            priority = PrioritizedEntity::WHEN_IN_DOUBT_PRIORITY;
                        }

                        if (priority != PrioritizedEntity::DO_NOT_SEND) {
                            _sendQueue.emplace(entity, priority);
                        }
                    });
                }
            });
            break;
        case DiffTraversal::Differential:
            assert(view.usesViewFrustums());
            _traversal.setScanCallback([this] (DiffTraversal::VisibleElement& next) {
                next.element->forEachEntity([&](EntityItemPointer entity) {
                    // Bail early if we've already checked this entity this frame
                    if (_sendQueue.contains(entity.get())) {
                        return;
                    }
                    float priority = PrioritizedEntity::DO_NOT_SEND;

                    auto knownTimestamp = _knownState.find(entity.get());
                    if (knownTimestamp == _knownState.end()) {
                        const auto& view = _traversal.getCurrentView();
                        priority = view.computePriority(entity);

                    } else if (entity->getLastEdited() > knownTimestamp->second ||
                               entity->getLastChangedOnServer() > knownTimestamp->second) {
                        // it is known and it changed --> put it on the queue with any priority
                        // TODO: sort these correctly
                        priority = PrioritizedEntity::WHEN_IN_DOUBT_PRIORITY;
                    }

                    if (priority != PrioritizedEntity::DO_NOT_SEND) {
                        _sendQueue.emplace(entity, priority);
                    }
                });
            });
            break;
    }
}

bool EntityTreeSendThread::traverseTreeAndBuildNextPacketPayload(EncodeBitstreamParams& params, const QJsonObject& jsonFilters) {
    if (_sendQueue.empty()) {
        params.stopReason = EncodeBitstreamParams::FINISHED;
        OctreeServer::trackEncodeTime(OctreeServer::SKIP_TIME);
        return false;
    }
    quint64 encodeStart = usecTimestampNow();
    if (!_packetData.hasContent()) {
        // This is the beginning of a new packet.
        // We pack minimal data for this to be accepted as an OctreeElement payload for the root element.
        // The Octree header bytes look like this:
        //
        // 0x00  octalcode for root
        // 0x00  colors (1 bit where recipient should call: child->readElementDataFromBuffer())
        // 0xXX  childrenInTreeMask (when params.includeExistsBits is true: 1 bit where child is existant)
        // 0x00  childrenInBufferMask (1 bit where recipient should call: child->readElementData() recursively)
        const uint8_t zeroByte = 0;
        _packetData.appendValue(zeroByte); // octalcode
        _packetData.appendValue(zeroByte); // colors
        if (params.includeExistsBits) {
            uint8_t childrenExistBits = 0;
            EntityTreeElementPointer root = std::dynamic_pointer_cast<EntityTreeElement>(_myServer->getOctree()->getRoot());
            for (int32_t i = 0; i < NUMBER_OF_CHILDREN; ++i) {
                if (root->getChildAtIndex(i)) {
                    childrenExistBits += (1 << i);
                }
            }
            _packetData.appendValue(childrenExistBits); // childrenInTreeMask
        }
        _packetData.appendValue(zeroByte); // childrenInBufferMask

        // Pack zero for numEntities.
        // But before we do: grab current byteOffset so we can come back later
        // and update this with the real number.
        _numEntities = 0;
        _numEntitiesOffset = _packetData.getUncompressedByteOffset();
        _packetData.appendValue(_numEntities);
    }

    LevelDetails entitiesLevel = _packetData.startLevel();
    uint64_t sendTime = usecTimestampNow();
    auto nodeData = static_cast<OctreeQueryNode*>(params.nodeData);
    nodeData->stats.encodeStarted();
    auto entityNode = _node.toStrongRef();
    auto entityNodeData = static_cast<EntityNodeData*>(entityNode->getLinkedData());
    while(!_sendQueue.empty()) {
        PrioritizedEntity queuedItem = _sendQueue.top();
        EntityItemPointer entity = queuedItem.getEntity();
        if (entity) {
            const QUuid& entityID = entity->getID();
            // Only send entities that match the jsonFilters, but keep track of everything we've tried to send so we don't try to send it again;
            // also send if we previously matched since this represents change to a matched item.
            bool entityMatchesFilters = entity->matchesJSONFilters(jsonFilters);
            bool entityPreviouslyMatchedFilter = entityNodeData->sentFilteredEntity(entityID);

            if (entityMatchesFilters || entityNodeData->isEntityFlaggedAsExtra(entityID) || entityPreviouslyMatchedFilter) {
                if (!jsonFilters.isEmpty() && entityMatchesFilters) {
                    // Record explicitly filtered-in entity so that extra entities can be flagged.
                    entityNodeData->insertSentFilteredEntity(entityID);
                }
                OctreeElement::AppendState appendEntityState = entity->appendEntityData(&_packetData, params, _extraEncodeData, entityNode->getCanGetAndSetPrivateUserData());

                if (appendEntityState != OctreeElement::COMPLETED) {
                    if (appendEntityState == OctreeElement::PARTIAL) {
                        ++_numEntities;
                    }
                    params.stopReason = EncodeBitstreamParams::DIDNT_FIT;
                    break;
                }

                if (entityPreviouslyMatchedFilter && !entityMatchesFilters) {
                    entityNodeData->removeSentFilteredEntity(entityID);
                }
                ++_numEntities;
            }
            if (queuedItem.shouldForceRemove()) {
                _knownState.erase(entity.get());
            } else {
                _knownState[entity.get()] = sendTime;
            }
        }
        _sendQueue.pop();
    }
    nodeData->stats.encodeStopped();
    if (_sendQueue.empty()) {
        assert(_sendQueue.empty());
        params.stopReason = EncodeBitstreamParams::FINISHED;
        _extraEncodeData->entities.clear();
    }

    if (_numEntities == 0) {
        _packetData.discardLevel(entitiesLevel);
        OctreeServer::trackEncodeTime((float)(usecTimestampNow() - encodeStart));
        return false;
    }
    _packetData.endLevel(entitiesLevel);
    _packetData.updatePriorBytes(_numEntitiesOffset, (const unsigned char*)&_numEntities, sizeof(_numEntities));
    OctreeServer::trackEncodeTime((float)(usecTimestampNow() - encodeStart));
    return true;
}

void EntityTreeSendThread::editingEntityPointer(const EntityItemPointer& entity) {
    if (entity) {
        if (!_sendQueue.contains(entity.get()) && _knownState.find(entity.get()) != _knownState.end()) {
            const auto& view = _traversal.getCurrentView();
            float priority = view.computePriority(entity);

            // We can force a removal from _knownState if the current view is used and entity is out of view
            if (priority == PrioritizedEntity::DO_NOT_SEND) {
                _sendQueue.emplace(entity, PrioritizedEntity::FORCE_REMOVE, true);
            } else if (priority == PrioritizedEntity::WHEN_IN_DOUBT_PRIORITY) {
                _sendQueue.emplace(entity, PrioritizedEntity::WHEN_IN_DOUBT_PRIORITY, true);
            }
        }
    }
}

void EntityTreeSendThread::deletingEntityPointer(EntityItem* entity) {
    _knownState.erase(entity);
}
