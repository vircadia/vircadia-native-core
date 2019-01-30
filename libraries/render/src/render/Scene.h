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
#include "Transition.h"
#include "HighlightStyle.h"

namespace render {

class RenderEngine;
class Scene;

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
    friend class Scene;
public:

    typedef std::function<void(ItemID, const Transition*)> TransitionQueryFunc;
    typedef std::function<void(HighlightStyle const*)> SelectionHighlightQueryFunc;

    Transaction() {}
    ~Transaction() {}

    // Item transactions
    void resetItem(ItemID id, const PayloadPointer& payload);
    void removeItem(ItemID id);
    bool hasRemovedItems() const { return !_removedItems.empty(); }

    void addTransitionToItem(ItemID id, Transition::Type transition, ItemID boundId = render::Item::INVALID_ITEM_ID);
    void removeTransitionFromItem(ItemID id);
    void reApplyTransitionToItem(ItemID id);
    void queryTransitionOnItem(ItemID id, TransitionQueryFunc func);

    template <class T> void updateItem(ItemID id, std::function<void(T&)> func) {
        updateItem(id, std::make_shared<UpdateFunctor<T>>(func));
    }

    void updateItem(ItemID id, const UpdateFunctorPointer& functor);
    void updateItem(ItemID id) { updateItem(id, nullptr); }

    // Selection transactions
    void resetSelection(const Selection& selection);

    void resetSelectionHighlight(const std::string& selectionName, const HighlightStyle& style = HighlightStyle());
    void removeHighlightFromSelection(const std::string& selectionName);
    void querySelectionHighlight(const std::string& selectionName, const SelectionHighlightQueryFunc& func);

    void reserve(const std::vector<Transaction>& transactionContainer);
    void merge(const std::vector<Transaction>& transactionContainer);
    void merge(std::vector<Transaction>&& transactionContainer);
    void merge(const Transaction& transaction);
    void merge(Transaction&& transaction);
    void clear();

    // Checkers if there is work to do when processing the transaction
    bool touchTransactions() const { return !_resetSelections.empty(); }

protected:

    using Reset = std::tuple<ItemID, PayloadPointer>;
    using Remove = ItemID;
    using Update = std::tuple<ItemID, UpdateFunctorPointer>;
    using TransitionAdd = std::tuple<ItemID, Transition::Type, ItemID>;
    using TransitionQuery = std::tuple<ItemID, TransitionQueryFunc>;
    using TransitionReApply = ItemID;
    using SelectionReset = Selection;
    using HighlightReset = std::tuple<std::string, HighlightStyle>;
    using HighlightRemove = std::string;
    using HighlightQuery = std::tuple<std::string, SelectionHighlightQueryFunc>;

    using Resets = std::vector<Reset>;
    using Removes = std::vector<Remove>;
    using Updates = std::vector<Update>;
    using TransitionAdds = std::vector<TransitionAdd>;
    using TransitionQueries = std::vector<TransitionQuery>;
    using TransitionReApplies = std::vector<TransitionReApply>;
    using SelectionResets = std::vector<SelectionReset>;
    using HighlightResets = std::vector<HighlightReset>;
    using HighlightRemoves = std::vector<HighlightRemove>;
    using HighlightQueries = std::vector<HighlightQuery>;

    Resets _resetItems;
    Removes _removedItems;
    Updates _updatedItems;
    TransitionAdds _addedTransitions;
    TransitionQueries _queriedTransitions;
    TransitionReApplies _reAppliedTransitions;
    SelectionResets _resetSelections;
    HighlightResets _highlightResets;
    HighlightRemoves _highlightRemoves;
    HighlightQueries _highlightQueries;
};
typedef std::vector<Transaction> TransactionQueue;


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

    // Enqueue transaction to the scene
    void enqueueTransaction(Transaction&& transaction);

    // Enqueue end of frame transactions boundary
    uint32_t enqueueFrame();

    // Process the pending transactions queued
    void processTransactionQueue();

    // Access a particular selection (empty if doesn't exist)
    // Thread safe
    Selection getSelection(const Selection::Name& name) const;

    // Check if a particular selection is empty (returns true if doesn't exist)
    // Thread safe
    bool isSelectionEmpty(const Selection::Name& name) const;

    // This next call are  NOT threadsafe, you have to call them from the correct thread to avoid any potential issues

    // Access a particular item from its ID
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

    void setItemTransition(ItemID id, Index transitionId);
    void resetItemTransition(ItemID id);

protected:

    // Thread safe elements that can be accessed from anywhere
    std::atomic<unsigned int> _IDAllocator{ 1 }; // first valid itemID will be One
    std::atomic<unsigned int> _numAllocatedItems{ 1 }; // num of allocated items, matching the _items.size()
    std::mutex _transactionQueueMutex;
    TransactionQueue _transactionQueue;

    
    std::mutex _transactionFramesMutex;
    using TransactionFrames = std::vector<Transaction>;
    TransactionFrames _transactionFrames;
    uint32_t _transactionFrameNumber{ 0 };

    // Process one transaction frame 
    void processTransactionFrame(const Transaction& transaction);

    // The actual database
    // database of items is protected for editing by a mutex
    std::mutex _itemsMutex;
    Item::Vector _items;
    ItemSpatialTree _masterSpatialTree;
    ItemIDSet _masterNonspatialSet;

    void resetItems(const Transaction::Resets& transactions);
    void removeItems(const Transaction::Removes& transactions);
    void updateItems(const Transaction::Updates& transactions);
    void transitionItems(const Transaction::TransitionAdds& transactions);
    void reApplyTransitions(const Transaction::TransitionReApplies& transactions);
    void queryTransitionItems(const Transaction::TransitionQueries& transactions);
    void resetHighlights(const Transaction::HighlightResets& transactions);
    void removeHighlights(const Transaction::HighlightRemoves& transactions);
    void queryHighlights(const Transaction::HighlightQueries& transactions);

    void collectSubItems(ItemID parentId, ItemIDs& subItems) const;

    // The Selection map
    mutable std::mutex _selectionsMutex; // mutable so it can be used in the thread safe getSelection const method
    SelectionMap _selections;

    void resetSelections(const Transaction::SelectionResets& transactions);
  // More actions coming to selections soon:
  //  void removeFromSelection(const Selection& selection);
  //  void appendToSelection(const Selection& selection);
  //  void mergeWithSelection(const Selection& selection);

    // The Stage map
    mutable std::mutex _stagesMutex; // mutable so it can be used in the thread safe getStage const method
    StageMap _stages;


    friend class RenderEngine;
};

typedef std::shared_ptr<Scene> ScenePointer;
typedef std::vector<ScenePointer> Scenes;

}

#endif // hifi_render_Scene_h
