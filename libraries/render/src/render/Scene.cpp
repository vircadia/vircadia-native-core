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
#include "gpu/Batch.h"

using namespace render;

void ItemBucketMap::insert(const ItemID& id, const ItemKey& key) {
    // Insert the itemID in every bucket where it filters true
    for (auto& bucket : (*this)) {
        if (bucket.first.test(key)) {
            bucket.second.insert(id);
        }
    }
}
void ItemBucketMap::erase(const ItemID& id, const ItemKey& key) {
    // Remove the itemID in every bucket where it filters true
    for (auto& bucket : (*this)) {
        if (bucket.first.test(key)) {
            bucket.second.erase(id);
        }
    }
}

void ItemBucketMap::reset(const ItemID& id, const ItemKey& oldKey, const ItemKey& newKey) {
    // Reset the itemID in every bucket,
    // Remove from the buckets where oldKey filters true AND newKey filters false
    // Insert into the buckets where newKey filters true
    for (auto& bucket : (*this)) {
        if (bucket.first.test(oldKey)) {
            if (!bucket.first.test(newKey)) {
                bucket.second.erase(id);
            }
        } else if (bucket.first.test(newKey)) {
            bucket.second.insert(id);
        }
    }
}

void ItemBucketMap::allocateStandardOpaqueTranparentBuckets() {
    (*this)[ItemFilter::Builder::opaqueShape().withoutLayered()];
    (*this)[ItemFilter::Builder::transparentShape().withoutLayered()];
    (*this)[ItemFilter::Builder::light()];
    (*this)[ItemFilter::Builder::background()];
    (*this)[ItemFilter::Builder::opaqueShape().withLayered()];
    (*this)[ItemFilter::Builder::transparentShape().withLayered()];
}


void PendingChanges::resetItem(ItemID id, const PayloadPointer& payload) {
    _resetItems.push_back(id);
    _resetPayloads.push_back(payload);
}

void PendingChanges::resortItem(ItemID id, ItemKey oldKey, ItemKey newKey) {
    _resortItems.push_back(id);
    _resortOldKeys.push_back(oldKey);
    _resortNewKeys.push_back(newKey);
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
    _resortItems.insert(_resortItems.end(), changes._resortItems.begin(), changes._resortItems.end());
    _resortOldKeys.insert(_resortOldKeys.end(), changes._resortOldKeys.begin(), changes._resortOldKeys.end());
    _resortNewKeys.insert(_resortNewKeys.end(), changes._resortNewKeys.begin(), changes._resortNewKeys.end());
    _removedItems.insert(_removedItems.end(), changes._removedItems.begin(), changes._removedItems.end());
    _updatedItems.insert(_updatedItems.end(), changes._updatedItems.begin(), changes._updatedItems.end());
    _updateFunctors.insert(_updateFunctors.end(), changes._updateFunctors.begin(), changes._updateFunctors.end());
}

Scene::Scene(glm::vec3 origin, float size) :
    _masterSpatialTree(origin, size)
{
    _items.push_back(Item()); // add the itemID #0 to nothing
    _masterBucketMap.allocateStandardOpaqueTranparentBuckets();
}

ItemID Scene::allocateID() {
    // Just increment and return the proevious value initialized at 0
    return _IDAllocator.fetch_add(1);
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
    PROFILE_RANGE(__FUNCTION__);
    _changeQueueMutex.lock();
    PendingChanges consolidatedPendingChanges;
    consolidateChangeQueue(_changeQueue, consolidatedPendingChanges);
    _changeQueueMutex.unlock();
    
    _itemsMutex.lock();
        // Here we should be able to check the value of last ItemID allocated 
        // and allocate new items accordingly
        ItemID maxID = _IDAllocator.load();
        if (maxID > _items.size()) {
            _items.resize(maxID + 100); // allocate the maxId and more
        }
        // Now we know for sure that we have enough items in the array to
        // capture anything coming from the pendingChanges
        resetItems(consolidatedPendingChanges._resetItems, consolidatedPendingChanges._resetPayloads);
        updateItems(consolidatedPendingChanges._updatedItems, consolidatedPendingChanges._updateFunctors);
        resortItems(consolidatedPendingChanges._resortItems, consolidatedPendingChanges._resortOldKeys, consolidatedPendingChanges._resortNewKeys);
        removeItems(consolidatedPendingChanges._removedItems);

     // ready to go back to rendering activities
    _itemsMutex.unlock();
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


        // Reset the item in the Bucket map
        _masterBucketMap.reset(resetID, oldKey, newKey);

        // Reset the item in the spatial tree
        auto newCell = _masterSpatialTree.resetItem(oldCell, oldKey, item.getBound(), resetID, newKey);
        item.resetCell(newCell, newKey.isSmall());

        // next loop
        resetPayload++;
    }
}

void Scene::resortItems(const ItemIDs& ids, ItemKeys& oldKeys, ItemKeys& newKeys) {
    auto resortID = ids.begin();
    auto oldKey = oldKeys.begin();
    auto newKey = newKeys.begin();
    for (; resortID != ids.end(); resortID++, oldKey++, newKey++) {
        _masterBucketMap.reset(*resortID, *oldKey, *newKey);
    }
}

void Scene::removeItems(const ItemIDs& ids) {
    for (auto removedID :ids) {
        // Access the true item
        auto& item = _items[removedID];
        auto oldCell = item.getCell();
        auto oldKey = item.getKey();

        // Remove from Bucket map
        _masterBucketMap.erase(removedID, item.getKey());

        // Remove from spatial tree
        _masterSpatialTree.removeItem(oldCell, oldKey, removedID);

        // Kill it
        item.kill();
    }
}

void Scene::updateItems(const ItemIDs& ids, UpdateFunctors& functors) {

    auto updateFunctor = functors.begin();
    for (auto updateID : ids) {
        // Access the true item
        auto& item = _items[updateID];
        auto oldCell = item.getCell();
        auto oldKey = item.getKey();

        // Update it
        _items[updateID].update((*updateFunctor));

        auto newKey = item.getKey();

        // Update the citem in the spatial tree if needed
        auto newCell = _masterSpatialTree.resetItem(oldCell, oldKey, item.getBound(), updateID, newKey);
        item.resetCell(newCell, newKey.isSmall());

        // next loop
        updateFunctor++;
    }
}
