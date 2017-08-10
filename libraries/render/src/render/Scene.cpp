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
#include "TransitionStage.h"

// Comment this to disable transitions (fades)
#define SCENE_ENABLE_TRANSITIONS

using namespace render;

void Transaction::resetItem(ItemID id, const PayloadPointer& payload) {
    if (payload) {
        _resetItems.emplace_back(Reset{ id, payload });
    } else {
        qCDebug(renderlogging) << "WARNING: Transaction::resetItem with a null payload!";
        removeItem(id);
    }
}

void Transaction::removeItem(ItemID id) {
    _removedItems.emplace_back(id);
}

void Transaction::addTransitionToItem(ItemID id, Transition::Type transition, ItemID boundId) {
    _addedTransitions.emplace_back(TransitionAdd{ id, transition, boundId });
}

void Transaction::removeTransitionFromItem(ItemID id) {
    _addedTransitions.emplace_back(TransitionAdd{ id, Transition::NONE, render::Item::INVALID_ITEM_ID });
}

void Transaction::reApplyTransitionToItem(ItemID id) {
    _reAppliedTransitions.emplace_back(TransitionReApply{ id });
}

void Transaction::queryTransitionOnItem(ItemID id, TransitionQueryFunc func) {
    _queriedTransitions.emplace_back(TransitionQuery{ id, func });
}

void Transaction::updateItem(ItemID id, const UpdateFunctorPointer& functor) {
    _updatedItems.emplace_back(Update{ id, functor });
}

void Transaction::resetSelection(const Selection& selection) {
    _resetSelections.emplace_back(selection);
}

void Transaction::merge(const Transaction& transaction) {
    _resetItems.insert(_resetItems.end(), transaction._resetItems.begin(), transaction._resetItems.end());
    _removedItems.insert(_removedItems.end(), transaction._removedItems.begin(), transaction._removedItems.end());
    _updatedItems.insert(_updatedItems.end(), transaction._updatedItems.begin(), transaction._updatedItems.end());
    _resetSelections.insert(_resetSelections.end(), transaction._resetSelections.begin(), transaction._resetSelections.end());
    _addedTransitions.insert(_addedTransitions.end(), transaction._addedTransitions.begin(), transaction._addedTransitions.end());
    _queriedTransitions.insert(_queriedTransitions.end(), transaction._queriedTransitions.begin(), transaction._queriedTransitions.end());
    _reAppliedTransitions.insert(_reAppliedTransitions.end(), transaction._reAppliedTransitions.begin(), transaction._reAppliedTransitions.end());
}


Scene::Scene(glm::vec3 origin, float size) :
    _masterSpatialTree(origin, size)
{
    _items.push_back(Item()); // add the itemID #0 to nothing
}

Scene::~Scene() {
    qCDebug(renderlogging) << "Scene::~Scene()";
}

ItemID Scene::allocateID() {
    // Just increment and return the proevious value initialized at 0
    return _IDAllocator.fetch_add(1);
}

bool Scene::isAllocatedID(const ItemID& id) const {
    return Item::isValidID(id) && (id < _numAllocatedItems.load());
}

/// Enqueue change batch to the scene
void Scene::enqueueTransaction(const Transaction& transaction) {
    _transactionQueueMutex.lock();
    _transactionQueue.push(transaction);
    _transactionQueueMutex.unlock();
}

void consolidateTransaction(TransactionQueue& queue, Transaction& singleBatch) {
    while (!queue.empty()) {
        const auto& transaction = queue.front();
        singleBatch.merge(transaction);
        queue.pop();
    };
}
 
void Scene::processTransactionQueue() {
    PROFILE_RANGE(render, __FUNCTION__);
    Transaction consolidatedTransaction;

    {
        std::unique_lock<std::mutex> lock(_transactionQueueMutex);
        consolidateTransaction(_transactionQueue, consolidatedTransaction);
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
        // capture anything coming from the transaction

        // resets and potential NEW items
        resetItems(consolidatedTransaction._resetItems);

        // Update the numItemsAtomic counter AFTER the reset changes went through
        _numAllocatedItems.exchange(maxID);

        // updates
        updateItems(consolidatedTransaction._updatedItems);

        // removes
        removeItems(consolidatedTransaction._removedItems);

#ifdef SCENE_ENABLE_TRANSITIONS
        // add transitions
        transitionItems(consolidatedTransaction._addedTransitions);
        reApplyTransitions(consolidatedTransaction._reAppliedTransitions);
        queryTransitionItems(consolidatedTransaction._queriedTransitions);
#endif
        // Update the numItemsAtomic counter AFTER the pending changes went through
        _numAllocatedItems.exchange(maxID);
    }

    if (consolidatedTransaction.touchTransactions()) {
        std::unique_lock<std::mutex> lock(_selectionsMutex);

        // resets and potential NEW items
        resetSelections(consolidatedTransaction._resetSelections);
    }
}

void Scene::resetItems(const Transaction::Resets& transactions) {
    for (auto& reset : transactions) {
        // Access the true item
        auto itemId = std::get<0>(reset);
        auto& item = _items[itemId];
        auto oldKey = item.getKey();
        auto oldCell = item.getCell();

        // Reset the item with a new payload
        item.resetPayload(std::get<1>(reset));
        auto newKey = item.getKey();

        // Update the item's container
        assert((oldKey.isSpatial() == newKey.isSpatial()) || oldKey._flags.none());
        if (newKey.isSpatial()) {
            auto newCell = _masterSpatialTree.resetItem(oldCell, oldKey, item.getBound(), itemId, newKey);
            item.resetCell(newCell, newKey.isSmall());
        } else {
            _masterNonspatialSet.insert(itemId);
        }
    }
}

void Scene::removeItems(const Transaction::Removes& transactions) {
    for (auto removedID : transactions) {
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

        // Remove the transition to prevent updating it for nothing
        resetItemTransition(removedID);

        // Kill it
        item.kill();
    }
}

void Scene::updateItems(const Transaction::Updates& transactions) {
    for (auto& update : transactions) {
        auto updateID = std::get<0>(update);
        if (updateID == Item::INVALID_ITEM_ID) {
            continue;
        }

        // Access the true item
        auto& item = _items[updateID];
        auto oldCell = item.getCell();
        auto oldKey = item.getKey();

        // Update the item
        item.update(std::get<1>(update));
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
    }
}

void Scene::transitionItems(const Transaction::TransitionAdds& transactions) {
    auto transitionStage = getStage<TransitionStage>(TransitionStage::getName());

    for (auto& add : transactions) {
        auto itemId = std::get<0>(add);
        // Access the true item
        const auto& item = _items[itemId];
        auto transitionId = item.getTransitionId();
        auto transitionType = std::get<1>(add);
        auto boundId = std::get<2>(add);

        // Remove pre-existing transition, if need be
        if (!TransitionStage::isIndexInvalid(transitionId)) {
            transitionStage->removeTransition(transitionId);
            transitionId = TransitionStage::INVALID_INDEX;
        }
        // Add a new one.
        if (transitionType != Transition::NONE) {
            transitionId = transitionStage->addTransition(itemId, transitionType, boundId);
        }

        setItemTransition(itemId, transitionId);
    }
}

void Scene::reApplyTransitions(const Transaction::TransitionReApplies& transactions) {
    for (auto itemId : transactions) {
        // Access the true item
        const auto& item = _items[itemId];
        auto transitionId = item.getTransitionId();
        setItemTransition(itemId, transitionId);
    }
}

void Scene::queryTransitionItems(const Transaction::TransitionQueries& transactions) {
    auto transitionStage = getStage<TransitionStage>(TransitionStage::getName());

    for (auto& query : transactions) {
        auto itemId = std::get<0>(query);
        // Access the true item
        const auto& item = _items[itemId];
        auto func = std::get<1>(query);
        if (item.exist() && func != nullptr) {
            auto transitionId = item.getTransitionId();

            if (!TransitionStage::isIndexInvalid(transitionId)) {
                auto& transition = transitionStage->getTransition(transitionId);
                func(itemId, &transition);
            } else {
                func(itemId, nullptr);
            }
        }
    }
}

void Scene::collectSubItems(ItemID parentId, ItemIDs& subItems) const {
    // Access the true item
    auto& item = _items[parentId];

    if (item.exist()) {
        // Recursivelly collect the subitems
        auto subItemBeginIndex = subItems.size();
        auto subItemCount = item.fetchMetaSubItems(subItems);
        for (auto i = subItemBeginIndex; i < (subItemBeginIndex + subItemCount); i++) {
            auto subItemId = subItems[i];
            // Bizarrely, subItemId == parentId can happen for metas... See metaFetchMetaSubItems in RenderableEntityItem.cpp
            if (subItemId != parentId) {
                collectSubItems(subItemId, subItems);
            }
        }
    }
}

void Scene::setItemTransition(ItemID itemId, Index transitionId) {
    // Access the true item
    auto& item = _items[itemId];

    item.setTransitionId(transitionId);
    if (item.exist()) {
        ItemIDs subItems;

        // Sub-items share the same transition Id
        collectSubItems(itemId, subItems);
        for (auto subItemId : subItems) {
            // Curiously... this can happen
            if (subItemId != itemId) {
                setItemTransition(subItemId, transitionId);
            }
        }
    }
}

void Scene::resetItemTransition(ItemID itemId) {
    auto& item = _items[itemId];
    if (!render::TransitionStage::isIndexInvalid(item.getTransitionId())) {
        auto transitionStage = getStage<TransitionStage>(TransitionStage::getName());
        transitionStage->removeTransition(item.getTransitionId());
        setItemTransition(itemId, render::TransitionStage::INVALID_INDEX);
    }
}

// THis fucntion is thread safe
Selection Scene::getSelection(const Selection::Name& name) const {
    std::unique_lock<std::mutex> lock(_selectionsMutex);
    auto found = _selections.find(name);
    if (found == _selections.end()) {
        return Selection();
    } else {
        return (*found).second;
    }
}

void Scene::resetSelections(const Transaction::SelectionResets& transactions) {
    for (auto selection : transactions) {
        auto found = _selections.find(selection.getName());
        if (found == _selections.end()) {
            _selections.insert(SelectionMap::value_type(selection.getName(), selection));
        } else {
            (*found).second = selection;
        }
    }
}

// Access a particular Stage (empty if doesn't exist)
// Thread safe
StagePointer Scene::getStage(const Stage::Name& name) const {
    std::unique_lock<std::mutex> lock(_stagesMutex);
    auto found = _stages.find(name);
    if (found == _stages.end()) {
        return StagePointer();
    } else {
        return (*found).second;
    }

}

void Scene::resetStage(const Stage::Name& name, const StagePointer& stage) {
    std::unique_lock<std::mutex> lock(_stagesMutex);
    auto found = _stages.find(name);
    if (found == _stages.end()) {
        _stages.insert(StageMap::value_type(name, stage));
    } else {
        (*found).second = stage;
    }
}