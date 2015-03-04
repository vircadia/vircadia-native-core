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

void Scene::ChangeBatch::resetItem(ID id, ItemDataPointer& itemData) {
    _resetItems.push_back(id);
    _resetItemDatas.push_back(itemData);
}

void Scene::ChangeBatch::removeItem(ID id) {
    _removedItems.push_back(id);
}

void Scene::ChangeBatch::moveItem(ID id) {
    _movedItems.push_back(id);
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
void Scene::enqueueChangeBatch(const ChangeBatch& changeBatch) {
    _changeQueueMutex.lock();
    _changeQueue.push_back(changeBatch);
    _changeQueueMutex.unlock();
}

void Scene::processChangeBatchQueue() {
    _itemsMutex.lock();
        for (auto changeBatch : _changeQueue) {
            for (auto reset : 
        }
    _itemsMutex.unlock();
}

void Scene::resetItem(ID id, ItemDataPointer& itemData) {
    /*ID id = _items.size();
    _items.push_back(Item(itemData));
    */
}

void Scene::removeItem(ID id) {
}

void Scene::moveItem(ID id) {
}
