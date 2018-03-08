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
#include "HighlightStage.h"

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
    _addedTransitions.emplace_back(id, transition, boundId);
}

void Transaction::removeTransitionFromItem(ItemID id) {
    _addedTransitions.emplace_back(id, Transition::NONE, render::Item::INVALID_ITEM_ID);
}

void Transaction::reApplyTransitionToItem(ItemID id) {
    _reAppliedTransitions.emplace_back(id);
}

void Transaction::queryTransitionOnItem(ItemID id, TransitionQueryFunc func) {
    _queriedTransitions.emplace_back(id, func);
}

void Transaction::updateItem(ItemID id, const UpdateFunctorPointer& functor) {
    _updatedItems.emplace_back(id, functor);
}

void Transaction::resetSelection(const Selection& selection) {
    _resetSelections.emplace_back(selection);
}

void Transaction::resetSelectionHighlight(const std::string& selectionName, const HighlightStyle& style) {
    _highlightResets.emplace_back(selectionName, style );
}

void Transaction::removeHighlightFromSelection(const std::string& selectionName) {
    _highlightRemoves.emplace_back(selectionName);
}

void Transaction::querySelectionHighlight(const std::string& selectionName, const SelectionHighlightQueryFunc& func) {
    _highlightQueries.emplace_back(selectionName, func);
}

void Transaction::reserve(const std::vector<Transaction>& transactionContainer) {
    size_t resetItemsCount = 0;
    size_t removedItemsCount = 0;
    size_t updatedItemsCount = 0;
    size_t resetSelectionsCount = 0;
    size_t addedTransitionsCount = 0;
    size_t queriedTransitionsCount = 0;
    size_t reAppliedTransitionsCount = 0;
    size_t highlightResetsCount = 0;
    size_t highlightRemovesCount = 0;
    size_t highlightQueriesCount = 0;

    for (const auto& transaction : transactionContainer) {
        resetItemsCount += transaction._resetItems.size();
        removedItemsCount += transaction._removedItems.size();
        updatedItemsCount += transaction._updatedItems.size();
        resetSelectionsCount += transaction._resetSelections.size();
        addedTransitionsCount += transaction._addedTransitions.size();
        queriedTransitionsCount += transaction._queriedTransitions.size();
        reAppliedTransitionsCount += transaction._reAppliedTransitions.size();
        highlightResetsCount += transaction._highlightResets.size();
        highlightRemovesCount += transaction._highlightRemoves.size();
        highlightQueriesCount += transaction._highlightQueries.size();
    }

    _resetItems.reserve(resetItemsCount);
    _removedItems.reserve(removedItemsCount);
    _updatedItems.reserve(updatedItemsCount);
    _resetSelections.reserve(resetSelectionsCount);
    _addedTransitions.reserve(addedTransitionsCount);
    _queriedTransitions.reserve(queriedTransitionsCount);
    _reAppliedTransitions.reserve(reAppliedTransitionsCount);
    _highlightResets.reserve(highlightResetsCount);
    _highlightRemoves.reserve(highlightRemovesCount);
    _highlightQueries.reserve(highlightQueriesCount);
}

void Transaction::merge(const std::vector<Transaction>& transactionContainer) {
    reserve(transactionContainer);
    for (const auto& transaction : transactionContainer) {
        merge(transaction);
    }
}


void Transaction::merge(std::vector<Transaction>&& transactionContainer) {
    reserve(transactionContainer);
    auto begin = std::make_move_iterator(transactionContainer.begin());
    auto end = std::make_move_iterator(transactionContainer.end());
    for (auto itr = begin; itr != end; ++itr) {
        merge(*itr);
    }
    transactionContainer.clear();
}


template <typename T>
void moveElements(T& target, T& source) {
    target.insert(target.end(), std::make_move_iterator(source.begin()), std::make_move_iterator(source.end()));
    source.clear();
}

template <typename T>
void copyElements(T& target, const T& source) {
    target.insert(target.end(), source.begin(), source.end());
}


void Transaction::merge(Transaction&& transaction) {
    moveElements(_resetItems, transaction._resetItems);
    moveElements(_removedItems, transaction._removedItems);
    moveElements(_updatedItems, transaction._updatedItems);
    moveElements(_resetSelections, transaction._resetSelections);
    moveElements(_addedTransitions, transaction._addedTransitions);
    moveElements(_queriedTransitions, transaction._queriedTransitions);
    moveElements(_reAppliedTransitions, transaction._reAppliedTransitions);
    moveElements(_highlightResets, transaction._highlightResets);
    moveElements(_highlightRemoves, transaction._highlightRemoves);
    moveElements(_highlightQueries, transaction._highlightQueries);
}

void Transaction::merge(const Transaction& transaction) {
    copyElements(_resetItems, transaction._resetItems);
    copyElements(_removedItems, transaction._removedItems);
    copyElements(_updatedItems, transaction._updatedItems);
    copyElements(_resetSelections, transaction._resetSelections);
    copyElements(_addedTransitions, transaction._addedTransitions);
    copyElements(_queriedTransitions, transaction._queriedTransitions);
    copyElements(_reAppliedTransitions, transaction._reAppliedTransitions);
    copyElements(_highlightResets, transaction._highlightResets);
    copyElements(_highlightRemoves, transaction._highlightRemoves);
    copyElements(_highlightQueries, transaction._highlightQueries);
}

void Transaction::clear() {
    _resetItems.clear();
    _removedItems.clear();
    _updatedItems.clear();
    _resetSelections.clear();
    _addedTransitions.clear();
    _queriedTransitions.clear();
    _reAppliedTransitions.clear();
    _highlightResets.clear();
    _highlightRemoves.clear();
    _highlightQueries.clear();
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
    std::unique_lock<std::mutex> lock(_transactionQueueMutex);
    _transactionQueue.emplace_back(transaction);
}

void Scene::enqueueTransaction(Transaction&& transaction) {
    std::unique_lock<std::mutex> lock(_transactionQueueMutex);
    _transactionQueue.emplace_back(std::move(transaction));
}

uint32_t Scene::enqueueFrame() {
    PROFILE_RANGE(render, __FUNCTION__);
    TransactionQueue localTransactionQueue;
    {
        std::unique_lock<std::mutex> lock(_transactionQueueMutex);
        localTransactionQueue.swap(_transactionQueue);
    }

    Transaction consolidatedTransaction;
    consolidatedTransaction.merge(std::move(localTransactionQueue));
    {
        std::unique_lock<std::mutex> lock(_transactionFramesMutex);
        _transactionFrames.push_back(consolidatedTransaction);
    }

    return ++_transactionFrameNumber;
}

 
void Scene::processTransactionQueue() {
    PROFILE_RANGE(render, __FUNCTION__);

    static TransactionFrames queuedFrames;
    {
        // capture the queued frames and clear the queue
        std::unique_lock<std::mutex> lock(_transactionFramesMutex);
        queuedFrames.swap(_transactionFrames);
    }

    // go through the queue of frames and process them
    for (auto& frame : queuedFrames) {
        processTransactionFrame(frame);
    }

    queuedFrames.clear();
}

void Scene::processTransactionFrame(const Transaction& transaction) {
    PROFILE_RANGE(render, __FUNCTION__);
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
        resetItems(transaction._resetItems);

        // Update the numItemsAtomic counter AFTER the reset changes went through
        _numAllocatedItems.exchange(maxID);

        // updates
        updateItems(transaction._updatedItems);

        // removes
        removeItems(transaction._removedItems);

        // add transitions
        transitionItems(transaction._addedTransitions);
        reApplyTransitions(transaction._reAppliedTransitions);
        queryTransitionItems(transaction._queriedTransitions);

        // Update the numItemsAtomic counter AFTER the pending changes went through
        _numAllocatedItems.exchange(maxID);
    }

    if (transaction.touchTransactions()) {
        std::unique_lock<std::mutex> lock(_selectionsMutex);

        // resets and potential NEW items
        resetSelections(transaction._resetSelections);
    }

    resetHighlights(transaction._highlightResets);
    removeHighlights(transaction._highlightRemoves);
    queryHighlights(transaction._highlightQueries);
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

        // If item doesn't exist it cannot be updated
        if (!item.exist()) {
            continue;
        }

        // Good to go, deal with the update
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

void Scene::resetHighlights(const Transaction::HighlightResets& transactions) {
    auto outlineStage = getStage<HighlightStage>(HighlightStage::getName());

    for (auto& transaction : transactions) {
        const auto& selectionName = std::get<0>(transaction);
        const auto& newStyle = std::get<1>(transaction);
        auto outlineId = outlineStage->getHighlightIdBySelection(selectionName);

        if (HighlightStage::isIndexInvalid(outlineId)) {
            outlineStage->addHighlight(selectionName, newStyle);
        } else {
            outlineStage->editHighlight(outlineId)._style = newStyle;
        }
    }
}

void Scene::removeHighlights(const Transaction::HighlightRemoves& transactions) {
    auto outlineStage = getStage<HighlightStage>(HighlightStage::getName());

    for (auto& selectionName : transactions) {
        auto outlineId = outlineStage->getHighlightIdBySelection(selectionName);

        if (!HighlightStage::isIndexInvalid(outlineId)) {
            outlineStage->removeHighlight(outlineId);
        }
    }
}

void Scene::queryHighlights(const Transaction::HighlightQueries& transactions) {
    auto outlineStage = getStage<HighlightStage>(HighlightStage::getName());

    for (auto& transaction : transactions) {
        const auto& selectionName = std::get<0>(transaction);
        const auto& func = std::get<1>(transaction);
        auto outlineId = outlineStage->getHighlightIdBySelection(selectionName);

        if (!HighlightStage::isIndexInvalid(outlineId)) {
            func(&outlineStage->editHighlight(outlineId)._style);
        } else {
            func(nullptr);
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

// This function is thread safe
Selection Scene::getSelection(const Selection::Name& name) const {
    std::unique_lock<std::mutex> lock(_selectionsMutex);
    auto found = _selections.find(name);
    if (found == _selections.end()) {
        return Selection();
    } else {
        return (*found).second;
    }
}

// This function is thread safe
bool Scene::isSelectionEmpty(const Selection::Name& name) const {
    std::unique_lock<std::mutex> lock(_selectionsMutex);
    auto found = _selections.find(name);
    if (found == _selections.end()) {
        return true;
    } else {
        return (*found).second.isEmpty();
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