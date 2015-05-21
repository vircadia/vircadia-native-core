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

#include <bitset>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <AABox.h>
#include <atomic>
#include <mutex>
#include <queue>

namespace render {

class Context;
    
// Key is the KEY to filter Items and create specialized lists
class ItemKey {
public:
    enum FlagBit {
        TYPE_SHAPE = 0, // Item is a Shape
        TYPE_LIGHT,     // Item is a Light
        TRANSLUCENT,    // Translucent and not opaque
        VIEW_SPACE,     // Transformed in view space, and not in world space
        DYNAMIC,        // Dynamic and bound will change unlike static item
        DEFORMED,       // Deformed within bound, not solid 
        INVISIBLE,      // Visible or not? could be just here to cast shadow
        SHADOW_CASTER,  // Item cast shadows
        PICKABLE,       // Item can be picked/selected
    
        INVERT_FLAG,     // If true then the meaning of the other flags is inverted

        NUM_FLAGS,      // Not a valid flag
    };
    typedef std::bitset<NUM_FLAGS> Flags; 
    
    // THe key is the Flags
    Flags _flags;

    ItemKey() : _flags(0) {}
    ItemKey(const Flags& flags) : _flags(flags) {}

    class Builder {
        Flags _flags;
    public:
        Builder() {}

        const ItemKey& build() const { return ItemKey(_flags); }

        Builder& withTypeShape() { _flags.set(TYPE_SHAPE); return (*this); }
        Builder& withTypeLight() { _flags.set(TYPE_LIGHT); return (*this); }
        Builder& withTranslucent() { _flags.set(TRANSLUCENT); return (*this); }
        Builder& withViewSpace() { _flags.set(VIEW_SPACE); return (*this); }
        Builder& withDynamic() { _flags.set(DYNAMIC); return (*this); }
        Builder& withDeformed() { _flags.set(DEFORMED); return (*this); }
        Builder& withInvisible() { _flags.set(INVISIBLE); return (*this); }
        Builder& withShadowCaster() { _flags.set(SHADOW_CASTER); return (*this); }
        Builder& withPickable() { _flags.set(PICKABLE); return (*this); }
    };


    bool isOpaque() const { return !_flags[TRANSLUCENT]; }
    bool isTranslucent() const { return _flags[TRANSLUCENT]; }

    bool isWorldSpace() const { return !_flags[VIEW_SPACE]; }
    bool isViewSpace() const { return _flags[VIEW_SPACE]; }
 
    bool isStatic() const { return !_flags[DYNAMIC]; }
    bool isDynamic() const { return _flags[DYNAMIC]; }

    bool isRigid() const { return !_flags[DEFORMED]; }
    bool isDeformed() const { return _flags[DEFORMED]; }
 
    bool isVisible() const { return !_flags[INVISIBLE]; }
    bool isUnvisible() const { return _flags[INVISIBLE]; }

    bool isShadowCaster() const { return _flags[SHADOW_CASTER]; }

    bool isPickable() const { return _flags[PICKABLE]; }
 
 
    // Item Key operator testing if a key pass a filter test
    // the filter can have several flags on and the test is true if
    // (key AND filter) == filter
    // IF the filter has the INVERT_FLAGS On then we need to use the
    // !key value instead of key
    bool filterTest(const ItemKey& filter) const {
        if (filter._flags[INVERT_FLAG]) {
            return (filter._flags & ~_flags) == filter._flags;
        } else {
            return (filter._flags & _flags) == filter._flags;
        }
    }
};

class RenderArgs;

class Item {
public:
    typedef std::vector<Item> Vector;
    typedef unsigned int ID;

    // Bound is the AABBox fully containing this item
    typedef AABox Bound;
    
    // Stats records the life history and performances of this item while performing at rendering and updating.
    // This is Used for monitoring and dynamically adjust the quality 
    class Stats {
    public:
        int _firstFrame;
    };

    // Payload is whatever is in this Item and implement the Payload Interface
    class PayloadInterface {
    public:
        virtual const ItemKey getKey() const = 0;
        virtual const Bound getBound() const = 0;
        virtual void render(RenderArgs* args) = 0;

        ~PayloadInterface() {}
    protected:
    };
    
    typedef std::shared_ptr<PayloadInterface> PayloadPointer;
    
    Item() {}
    Item(PayloadPointer& payload):
        _payload(payload) {}

    ~Item() {}

    void resetPayload(PayloadPointer& payload);
    void kill();
    void move();

    // Check heuristic key
    const ItemKey& getKey() const { return _key; }

    // Payload Interface
    const Bound getBound() const { return _payload->getBound(); }
    void render(RenderArgs* args) { _payload->render(args); }

protected:
    PayloadPointer _payload;
    ItemKey _key;

    friend class Scene;
};

// THe Payload class is the real Payload to be used
// THis allow anything to be turned into a Payload as long as the required interface functions are available
// When creating a new kind of payload from a new "stuff" class then you need to create specialized version for "stuff"
// of the Payload interface
template <class T> const ItemKey payloadGetKey(const std::shared_ptr<T>& payloadData) { return ItemKey(); }
template <class T> const Item::Bound payloadGetBound(const std::shared_ptr<T>& payloadData) { return Item::Bound(); }
template <class T> void payloadRender(const std::shared_ptr<T>& payloadData, RenderArgs* args) { }
    
template <class T> class Payload : public Item::PayloadInterface {
public:
    typedef std::shared_ptr<T> DataPointer;

    virtual const ItemKey getKey() const { return payloadGetKey<T>(_data); }
    virtual const Item::Bound getBound() const { return payloadGetBound<T>(_data); }
    virtual void render(RenderArgs* args) { payloadRender<T>(_data, args); }
    
    Payload(DataPointer& data) : _data(data) {}
protected:
    DataPointer _data;
};

// Let's show how to make a simple FooPayload example:
/*
class Foo {
public:
    mutable ItemKey _myownKey;
    void makeMywnKey() const {
        _myownKey = ItemKey::Builder().withTypeShape().build();
    }

    const Item::Bound evaluateMyBound() {
        // Do stuff here to get your final Bound
        return Item::Bound();
    }
};

typedef Payload<Foo> FooPayload;
typedef std::shared_ptr<Foo> FooPointer;

// In a Source file, not a header, implement the Payload interface function specialized for Foo:
template <> const ItemKey payloadGetKey(const FooPointer& foo) {
    // Foo's way of provinding its Key
    foo->makeMyKey();
    return foo->_myownKey;
}
template <> const Item::Bound payloadGetBound(const FooPointer& foo) {
    // evaluate Foo's own bound
    return foo->evaluateMyBound();
}

// In this example, do not specialize the payloadRender call which means the compiler will use the default version which does nothing 

*/
// End of the example

typedef Item::PayloadPointer PayloadPointer;
typedef std::vector< PayloadPointer > Payloads;

// A few typedefs for standard containers of ItemIDs 
typedef Item::ID ItemID;
typedef std::vector<ItemID> ItemIDs;
typedef std::set<ItemID> ItemIDSet;

// A map of ItemIDSets allowing to create bucket lists of items which are filtering correctly
class ItemBucketMap : public std::map<ItemKey, ItemIDSet> {
public:

    ItemBucketMap() {}

    void insert(const ItemID& id, const ItemKey& key);
    void erase(const ItemID& id, const ItemKey& key);
    void reset(const ItemID& id, const ItemKey& oldKey, const ItemKey& newKey);

};

class Engine;
class Observer;

// Scene is a container for Items
// Items are introduced, modified or erased in the scene through PendingChanges
// Once per Frame, the PendingChanges are all flushed
// During the flush the standard buckets are updated
// Items are notified accordingly on any update message happening
class Scene {
public:
 
    class Observer {
    public:
        Observer(Scene* scene) {
            
        }
        ~Observer() {}

        const Scene* getScene() const { return _scene; }
        Scene* editScene() { return _scene; }

    protected:
        Scene* _scene = nullptr;
        virtual void onRegisterScene() {}
        virtual void onUnregisterScene() {}

        friend class Scene;
        void registerScene(Scene* scene) {
            _scene = scene;
            onRegisterScene();
        }

        void unregisterScene() {
            onUnregisterScene();
            _scene = 0;
        }
    };
    typedef std::shared_ptr< Observer > ObserverPointer;
    typedef std::vector< ObserverPointer > Observers;

    class PendingChanges {
    public:
        PendingChanges() {}
        ~PendingChanges();

        void resetItem(ItemID id, PayloadPointer& payload);
        void removeItem(ItemID id);
        void moveItem(ItemID id);

        void merge(PendingChanges& changes);

        Payloads _resetPayloads;
        ItemIDs _resetItems; 
        ItemIDs _removedItems;
        ItemIDs _movedItems;

    protected:
    };
    typedef std::queue<PendingChanges> PendingChangesQueue;

    Scene();
    ~Scene() {}

    /// This call is thread safe, can be called from anywhere to allocate a new ID
    ItemID allocateID();

    /// Enqueue change batch to the scene
    void enqueuePendingChanges(const PendingChanges& pendingChanges);

    /// Scene Observer listen to any change and get notified
    void registerObserver(ObserverPointer& observer);
    void unregisterObserver(ObserverPointer& observer);

protected:
    // Thread safe elements that can be accessed from anywhere
    std::atomic<unsigned int> _IDAllocator;
    std::mutex _changeQueueMutex;
    PendingChangesQueue _changeQueue;

    // The actual database
    // database of items is protected for editing by a mutex
    std::mutex _itemsMutex;
    Item::Vector _items;
    ItemBucketMap _buckets;

    void processPendingChangesQueue();
    void resetItems(const ItemIDs& ids, Payloads& payloads);
    void removeItems(const ItemIDs& ids);
    void moveItems(const ItemIDs& ids);


    // The scene context listening for any change to the database
    Observers _observers;

    friend class Engine;
};



typedef std::shared_ptr<Scene> ScenePointer;
typedef std::vector<ScenePointer> Scenes;

}

#endif // hifi_render_Scene_h
