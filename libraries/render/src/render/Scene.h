//
//  Scene.h
//  render/src/render
//
//  Created by Sam Gateau on 1/11/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Scene_h
#define hifi_render_Scene_h

#include "Item.h"
#include "SpatialTree.h"
#include "Stage.h"
#include "Selection.h"

namespace render {

class Engine;

// Transaction is the mechanism to make any change to the scene.
// Whenever a new item need to be reset,
// or when an item changes its position or its size
// or when an item's payload has to be be updated with new states (coming from outside the scene knowledge)
// or when an item is destroyed
// These changes must be expressed through the corresponding command from the Transaction
// THe Transaction is then queued on the Scene so all the pending transactions can be consolidated and processed at the time
// of updating the scene before it s rendered.
// 
class Transaction {
public:
    Transaction() {}
    ~Transaction() {}

    // Item transactions
    void resetItem(ItemID id, const PayloadPointer& payload);
    void removeItem(ItemID id);

    template <class T> void updateItem(ItemID id, std::function<void(T&)> func) {
        updateItem(id, std::make_shared<UpdateFunctor<T>>(func));
    }

    void updateItem(ItemID id, const UpdateFunctorPointer& functor);
    void updateItem(ItemID id) { updateItem(id, nullptr); }

    // Selection transactions
    void resetSelection(const Selection& selection);

    void merge(const Transaction& transaction);

    // Checkers if there is work to do when processing the transaction
    bool touchTransactions() const { return !_resetSelections.empty(); }

    ItemIDs _resetItems; 
    Payloads _resetPayloads;
    ItemIDs _removedItems;
    ItemIDs _updatedItems;
    UpdateFunctors _updateFunctors;

    Selections _resetSelections;

protected:
};
typedef std::queue<Transaction> TransactionQueue;


// Scene is a container for Items
// Items are introduced, modified or erased in the scene through Transaction
// Once per Frame, the Transaction are all flushed
// During the flush the standard buckets are updated
// Items are notified accordingly on any update message happening
class Scene {
public:
    Scene(glm::vec3 origin, float size);
    ~Scene();

    // This call is thread safe, can be called from anywhere to allocate a new ID
    ItemID allocateID();

    // Check that the ID is valid and allocated for this scene, this a threadsafe call
    bool isAllocatedID(const ItemID& id) const;

    // THis is the total number of allocated items, this a threadsafe call
    size_t getNumItems() const { return _numAllocatedItems.load(); }

    // Enqueue transaction to the scene
    void enqueueTransaction(const Transaction& transaction);

    // Process the pending transactions queued
    void processTransactionQueue();

    // Access a particular selection (empty if doesn't exist)
    // Thread safe
    Selection getSelection(const Selection::Name& name) const;

    // This next call are  NOT threadsafe, you have to call them from the correct thread to avoid any potential issues

    // Access a particular item form its ID
    // WARNING, There is No check on the validity of the ID, so this could return a bad Item
    const Item& getItem(const ItemID& id) const { return _items[id]; }

    // Same as getItem, checking if the id is valid
    const Item getItemSafe(const ItemID& id) const { if (isAllocatedID(id)) { return _items[id]; } else { return Item(); } }

    // Access the spatialized items
    const ItemSpatialTree& getSpatialTree() const { return _masterSpatialTree; }

    // Access non-spatialized items (overlays, backgrounds)
    const ItemIDSet& getNonspatialSet() const { return _masterNonspatialSet; }



    // Access a particular Stage (empty if doesn't exist)
    // Thread safe
    StagePointer getStage(const Stage::Name& name) const;
    template <class T>
    std::shared_ptr<T> getStage(const Stage::Name& name = T::getName()) const {
        auto stage = getStage(name);
        return (stage ? std::static_pointer_cast<T>(stage) : std::shared_ptr<T>());
    }
    void resetStage(const Stage::Name& name, const StagePointer& stage);


protected:
    // Thread safe elements that can be accessed from anywhere
    std::atomic<unsigned int> _IDAllocator{ 1 }; // first valid itemID will be One
    std::atomic<unsigned int> _numAllocatedItems{ 1 }; // num of allocated items, matching the _items.size()
    std::mutex _transactionQueueMutex;
    TransactionQueue _transactionQueue;

    // The actual database
    // database of items is protected for editing by a mutex
    std::mutex _itemsMutex;
    Item::Vector _items;
    ItemSpatialTree _masterSpatialTree;
    ItemIDSet _masterNonspatialSet;

    void resetItems(const ItemIDs& ids, Payloads& payloads);
    void removeItems(const ItemIDs& ids);
    void updateItems(const ItemIDs& ids, UpdateFunctors& functors);

    // The Selection map
    mutable std::mutex _selectionsMutex; // mutable so it can be used in the thread safe getSelection const method
    SelectionMap _selections;

    void resetSelections(const Selections& selections);
  // More actions coming to selections soon:
  //  void removeFromSelection(const Selection& selection);
  //  void appendToSelection(const Selection& selection);
  //  void mergeWithSelection(const Selection& selection);

    // The Stage map
    mutable std::mutex _stagesMutex; // mutable so it can be used in the thread safe getStage const method
    StageMap _stages;


    friend class Engine;
};

typedef std::shared_ptr<Scene> ScenePointer;
typedef std::vector<ScenePointer> Scenes;

}

#endif // hifi_render_Scene_h
