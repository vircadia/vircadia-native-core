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


void Transaction::reset(ProxyID id, const ProxyPayload& payload, const Owner& owner) {
    _resetItems.emplace_back(Reset{ id, payload, owner });
}

void Transaction::remove(ProxyID id) {
    _removedItems.emplace_back(id);
}

void Transaction::update(ProxyID id, const ProxyPayload& payload) {
    _updatedItems.emplace_back(id, payload);
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


void Transaction::reset(const Resets& resets) {
    copyElements(_resetItems, resets);
}

void Transaction::remove(const Removes& removes) {
    copyElements(_removedItems, removes);
}

void Transaction::update(const Updates& updates) {
    copyElements(_updatedItems, updates);
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
}

Collection::~Collection() {
}

void Collection::clear() {
    std::unique_lock<std::mutex> lock(_transactionQueueMutex);
    _transactionQueue.clear();
    _transactionFrames.clear();
}

ProxyID Collection::allocateID() {
    // Just increment and return the previous value initialized at 0
    return _IDAllocator.allocateIndex();
}

bool Collection::isAllocatedID(const ProxyID& id) const {
    return _IDAllocator.checkIndex(id);
}

/// Enqueue change batch to the Collection
void Collection::enqueueTransaction(const Transaction& transaction) {
    std::unique_lock<std::mutex> lock(_transactionQueueMutex);
    _transactionQueue.emplace_back(transaction);
}

void Collection::enqueueTransaction(Transaction&& transaction) {
    std::unique_lock<std::mutex> lock(_transactionQueueMutex);
    _transactionQueue.emplace_back(std::move(transaction));
}

uint32_t Collection::enqueueFrame() {
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
