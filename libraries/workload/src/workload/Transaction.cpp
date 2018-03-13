//
//  Transaction.cpp
//  libraries/workload/src/workload
//
//  Created by Sam Gateau 2018.03.12
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Transaction.h"

using namespace workload;


void Transaction::resetItem(ItemID id, PayloadPointer& payload) {
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

void Transaction::updateItem(ItemID id, const UpdateFunctorPointer& functor) {
    _updatedItems.emplace_back(id, functor);
}

void Transaction::reserve(const std::vector<Transaction>& transactionContainer) {
    size_t resetItemsCount = 0;
    size_t removedItemsCount = 0;
    size_t updatedItemsCount = 0;

    for (const auto& transaction : transactionContainer) {
        resetItemsCount += transaction._resetItems.size();
        removedItemsCount += transaction._removedItems.size();
        updatedItemsCount += transaction._updatedItems.size();
    }

    _resetItems.reserve(resetItemsCount);
    _removedItems.reserve(removedItemsCount);
    _updatedItems.reserve(updatedItemsCount);
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
}

void Transaction::merge(const Transaction& transaction) {
    copyElements(_resetItems, transaction._resetItems);
    copyElements(_removedItems, transaction._removedItems);
    copyElements(_updatedItems, transaction._updatedItems);
}

void Transaction::clear() {
    _resetItems.clear();
    _removedItems.clear();
    _updatedItems.clear();
}




Collection::Collection() {
    _items.push_back(Item()); // add the itemID #0 to nothing
}

Collection::~Collection() {
    qCDebug(renderlogging) << "Scene::~Scene()";
}

ItemID Collection::allocateID() {
    // Just increment and return the proevious value initialized at 0
    return _IDAllocator.fetch_add(1);
}

bool Collection::isAllocatedID(const ItemID& id) const {
    return Item::isValidID(id) && (id < _numAllocatedItems.load());
}

/// Enqueue change batch to the scene
void Collection::enqueueTransaction(const Transaction& transaction) {
    std::unique_lock<std::mutex> lock(_transactionQueueMutex);
    _transactionQueue.emplace_back(transaction);
}

void Collection::enqueueTransaction(Transaction&& transaction) {
    std::unique_lock<std::mutex> lock(_transactionQueueMutex);
    _transactionQueue.emplace_back(std::move(transaction));
}

uint32_t Collection::enqueueFrame() {
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


void Collection::processTransactionQueue() {
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

void Collection::processTransactionFrame(const Transaction& transaction) {
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
}

void Collection::resetItems(const Transaction::Resets& transactions) {
    for (auto& reset : transactions) {
        // Access the true item
        auto itemId = std::get<0>(reset);
        auto& item = _items[itemId];

        // Reset the item with a new payload
        item.resetPayload(std::get<1>(reset));
    }
}

void Collection::removeItems(const Transaction::Removes& transactions) {
    for (auto removedID : transactions) {
        // Access the true item
        auto& item = _items[removedID];

        // Kill it
        item.kill();
    }
}

void Collection::updateItems(const Transaction::Updates& transactions) {
    for (auto& update : transactions) {
        auto updateID = std::get<0>(update);
        if (updateID == Item::INVALID_ITEM_ID) {
            continue;
        }

        // Access the true item
        auto& item = _items[updateID];

        // Update the item
        item.update(std::get<1>(update));
    }
}