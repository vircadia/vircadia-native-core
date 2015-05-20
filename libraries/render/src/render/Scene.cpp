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

using namespace render;

void Scene::PendingChanges::resetItem(ID id, PayloadPointer& payload) {
    _resetItems.push_back(id);
    _resetPayloads.push_back(payload);
}

void Scene::PendingChanges::removeItem(ID id) {
    _removedItems.push_back(id);
}

void Scene::PendingChanges::moveItem(ID id) {
    _movedItems.push_back(id);
}

        
void Scene::PendingChanges::mergeBatch(PendingChanges& newBatch) {
    _resetItems.insert(_resetItems.end(), newBatch._resetItems.begin(), newBatch._resetItems.end());
    _resetPayloads.insert(_resetPayloads.end(), newBatch._resetPayloads.begin(), newBatch._resetPayloads.end());
    _removedItems.insert(_removedItems.end(), newBatch._removedItems.begin(), newBatch._removedItems.end());
    _movedItems.insert(_movedItems.end(), newBatch._movedItems.begin(), newBatch._movedItems.end());
}

Scene::Scene() :
    _IDAllocator(0)
{
}

Item::ID Scene::allocateID() {
    // Just increment and return the proevious value initialized at 0
    return _IDAllocator.fetch_add(1);
}

/// Enqueue change batch to the scene
void Scene::enqueuePendingChanges(const PendingChanges& pendingChanges) {
    _changeQueueMutex.lock();
    _changeQueue.push(pendingChanges);
    _changeQueueMutex.unlock();
}

void consolidateChangeQueue(Scene::PendingChangesQueue& queue, Scene::PendingChanges& singleBatch) {
    while (!queue.empty()) {
        auto pendingChanges = queue.front();
        singleBatch.mergeBatch(pendingChanges);
        queue.pop();
    };
}
 
void Scene::processPendingChangesQueue() {
    _changeQueueMutex.lock();
    PendingChanges consolidatedPendingChanges;
    consolidateChangeQueue(_changeQueue, consolidatedPendingChanges);
    _changeQueueMutex.unlock();
    
    _itemsMutex.lock();
        // Here we should be able to check the value of last ID allocated 
        // and allocate new items accordingly
        ID maxID = _IDAllocator.load();
        if (maxID > _items.size()) {
            _items.resize(maxID + 100); // allocate the maxId and more
        }
        // Now we know for sure that we have enough items in the array to
        // capture anything coming from the pendingChanges
        resetItems(consolidatedPendingChanges._resetItems, consolidatedPendingChanges._resetPayloads);
        removeItems(consolidatedPendingChanges._removedItems);
        moveItems(consolidatedPendingChanges._movedItems);

     // ready to go back to rendering activities
    _itemsMutex.unlock();
}

void Scene::resetItems(const ItemIDs& ids, Payloads& payloads) {
    auto resetID = ids.begin();
    auto resetPayload = payloads.begin();
    for (;resetID != ids.end(); resetID++, resetPayload++) {
        _items[(*resetID)].resetPayload(*resetPayload);
    }
}

void Scene::removeItems(const ItemIDs& ids) {
    for (auto removedID :ids) {
        _items[removedID].kill();
    }
}

void Scene::moveItems(const ItemIDs& ids) {
    for (auto movedID :ids) {
        _items[movedID].move();
    }
}

void Scene::registerObserver(ObserverPointer& observer) {
    // make sure it's a valid observer
    if (observer && (observer->getScene() == nullptr)) {
        // Then register the observer
        _observers.push_back(observer);

        // And let it do what it wants to do
        observer->registerScene(this);
    }
}

void Scene::unregisterObserver(ObserverPointer& observer) {
    // make sure it's a valid observer currently registered
    if (observer && (observer->getScene() == this)) {
        // let it do what it wants to do
        observer->unregisterScene();

        // Then unregister the observer
        auto it = std::find(_observers.begin(), _observers.end(), observer);
        _observers.erase(it);
    }
}

