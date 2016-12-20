//
//  Scene.cpp
//  render/src/render
//
//  Created by Sam Gateau on 1/11/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Scene.h"

#include <numeric>
#include <gpu/Batch.h>
#include "Logging.h"

using namespace render;

void PendingChanges::resetItem(ItemID id, const PayloadPointer& payload) {
    if (payload) {
        _resetItems.push_back(id);
        _resetPayloads.push_back(payload);
    } else {
        qDebug() << "WARNING: PendingChanges::resetItem with a null payload!";
        removeItem(id);
    }
}

void PendingChanges::removeItem(ItemID id) {
    _removedItems.push_back(id);
}

void PendingChanges::updateItem(ItemID id, const UpdateFunctorPointer& functor) {
    _updatedItems.push_back(id);
    _updateFunctors.push_back(functor);
}

void PendingChanges::merge(PendingChanges& changes) {
    _resetItems.insert(_resetItems.end(), changes._resetItems.begin(), changes._resetItems.end());
    _resetPayloads.insert(_resetPayloads.end(), changes._resetPayloads.begin(), changes._resetPayloads.end());
    _removedItems.insert(_removedItems.end(), changes._removedItems.begin(), changes._removedItems.end());
    _updatedItems.insert(_updatedItems.end(), changes._updatedItems.begin(), changes._updatedItems.end());
    _updateFunctors.insert(_updateFunctors.end(), changes._updateFunctors.begin(), changes._updateFunctors.end());
}

Scene::Scene(glm::vec3 origin, float size) :
    _masterSpatialTree(origin, size)
{
    _items.push_back(Item()); // add the itemID #0 to nothing
}

Scene::~Scene() {
    qDebug() << "Scene::~Scene()";
}

ItemID Scene::allocateID() {
    // Just increment and return the proevious value initialized at 0
    return _IDAllocator.fetch_add(1);
}

bool Scene::isAllocatedID(const ItemID& id) {
    return Item::isValidID(id) && (id < _numAllocatedItems.load());
}

/// Enqueue change batch to the scene
void Scene::enqueuePendingChanges(const PendingChanges& pendingChanges) {
    _changeQueueMutex.lock();
    _changeQueue.push(pendingChanges);
    _changeQueueMutex.unlock();
}

void consolidateChangeQueue(PendingChangesQueue& queue, PendingChanges& singleBatch) {
    while (!queue.empty()) {
        auto pendingChanges = queue.front();
        singleBatch.merge(pendingChanges);
        queue.pop();
    };
}
 
void Scene::processPendingChangesQueue() {
    PROFILE_RANGE(render, __FUNCTION__);
    PendingChanges consolidatedPendingChanges;

    {
        std::unique_lock<std::mutex> lock(_changeQueueMutex);
        consolidateChangeQueue(_changeQueue, consolidatedPendingChanges);
    }
    
    {
        std::unique_lock<std::mutex> lock(_itemsMutex);
        // Here we should be able to check the value of last ItemID allocated 
        // and allocate new items accordingly
        ItemID maxID = _IDAllocator.load();
        if (maxID > _items.size()) {
            _items.resize(maxID + 100); // allocate the maxId and more
        }
        // Now we know for sure that we have enough items in the array to
        // capture anything coming from the pendingChanges

        // resets and potential NEW items
        resetItems(consolidatedPendingChanges._resetItems, consolidatedPendingChanges._resetPayloads);

        // Update the numItemsAtomic counter AFTER the reset changes went through
        _numAllocatedItems.exchange(maxID);

        // updates
        updateItems(consolidatedPendingChanges._updatedItems, consolidatedPendingChanges._updateFunctors);

        // removes
        removeItems(consolidatedPendingChanges._removedItems);

        // Update the numItemsAtomic counter AFTER the pending changes went through
        _numAllocatedItems.exchange(maxID);
    }
}

void Scene::resetItems(const ItemIDs& ids, Payloads& payloads) {
    auto resetPayload = payloads.begin();
    for (auto resetID : ids) {
        // Access the true item
        auto& item = _items[resetID];
        auto oldKey = item.getKey();
        auto oldCell = item.getCell();

        // Reset the item with a new payload
        item.resetPayload(*resetPayload);
        auto newKey = item.getKey();

        // Update the item's container
        assert((oldKey.isSpatial() == newKey.isSpatial()) || oldKey._flags.none());
        if (newKey.isSpatial()) {
            auto newCell = _masterSpatialTree.resetItem(oldCell, oldKey, item.getBound(), resetID, newKey);
            item.resetCell(newCell, newKey.isSmall());
        } else {
            _masterNonspatialSet.insert(resetID);
        }

        // next loop
        resetPayload++;
    }
}

void Scene::removeItems(const ItemIDs& ids) {
    for (auto removedID :ids) {
        // Access the true item
        auto& item = _items[removedID];
        auto oldCell = item.getCell();
        auto oldKey = item.getKey();

        // Remove the item
        if (oldKey.isSpatial()) {
            _masterSpatialTree.removeItem(oldCell, oldKey, removedID);
        } else {
            _masterNonspatialSet.erase(removedID);
        }

        // Kill it
        item.kill();
    }
}

void Scene::updateItems(const ItemIDs& ids, UpdateFunctors& functors) {

    auto updateFunctor = functors.begin();
    for (auto updateID : ids) {
        if (updateID == Item::INVALID_ITEM_ID) {
            updateFunctor++;
            continue;
        }

        // Access the true item
        auto& item = _items[updateID];
        auto oldCell = item.getCell();
        auto oldKey = item.getKey();

        // Update the item
        item.update((*updateFunctor));
        auto newKey = item.getKey();

        // Update the item's container
        if (oldKey.isSpatial() == newKey.isSpatial()) {
            if (newKey.isSpatial()) {
                auto newCell = _masterSpatialTree.resetItem(oldCell, oldKey, item.getBound(), updateID, newKey);
                item.resetCell(newCell, newKey.isSmall());
            }
        } else {
            if (newKey.isSpatial()) {
                _masterNonspatialSet.erase(updateID);

                auto newCell = _masterSpatialTree.resetItem(oldCell, oldKey, item.getBound(), updateID, newKey);
                item.resetCell(newCell, newKey.isSmall());
            } else {
                _masterSpatialTree.removeItem(oldCell, oldKey, updateID);
                item.resetCell();

                _masterNonspatialSet.insert(updateID);
            }
        }


        // next loop
        updateFunctor++;
    }
}
