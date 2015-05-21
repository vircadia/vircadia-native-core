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
#include <map>
#include <AABox.h>
#include <atomic>
#include <mutex>
#include <queue>

namespace render {

class Context;
    
    


    
class Item {
public:
    typedef std::vector<Item> Vector;
    typedef unsigned int ID;

    // State is the KEY to filter Items and create specialized lists
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
    
        NUM_FLAGS,      // Not a valid flag
    };
    typedef std::bitset<NUM_FLAGS> State; 

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
        virtual const State getState() const = 0;
        virtual const Bound getBound() const = 0;
        virtual void render(Context& context) = 0;

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

    // Check heuristic flags of the state
    const State& getState() const { return _state; }

    bool isOpaque() const { return !_state[TRANSLUCENT]; }
    bool isTranslucent() const { return _state[TRANSLUCENT]; }

    bool isWorldSpace() const { return !_state[VIEW_SPACE]; }
    bool isViewSpace() const { return _state[VIEW_SPACE]; }
 
    bool isStatic() const { return !_state[DYNAMIC]; }
    bool isDynamic() const { return _state[DYNAMIC]; }
    bool isDeformed() const { return _state[DEFORMED]; }
 
    bool isVisible() const { return !_state[INVISIBLE]; }
    bool isUnvisible() const { return _state[INVISIBLE]; }

    bool isShadowCaster() const { return _state[SHADOW_CASTER]; }

    bool isPickable() const { return _state[PICKABLE]; }
 
    // Payload Interface
    const Bound getBound() const { return _payload->getBound(); }
    void render(Context& context) { _payload->render(context); }

protected:
    PayloadPointer _payload;
    State _state = 0;

    friend class Scene;
};

template <class T> const Item::State payloadGetState(const T* payload) { return Item::State(); }
template <class T> const Item::Bound payloadGetBound(const T* payload) { return Item::Bound(); }
template <class T> void payloadRender(const T* payload) { }
    
template <class T> class ItemPayload : public Item::PayloadInterface {
public:
    virtual const Item::State getState() const { return payloadGetState<T>(_pointee); }
    virtual const Item::Bound getBound() const { return payloadGetBound<T>(_pointee); }
    virtual void render(Context& context) { payloadRender<T>(_pointee, context); }
    
    ItemPayload(std::shared_ptr<T>& pointee) : _pointee(pointee) {}
protected:
    std::shared_ptr<T> _pointee;
};
    
typedef Item::PayloadInterface Payload;
typedef Item::PayloadPointer PayloadPointer;
typedef std::vector< PayloadPointer > Payloads;

class Engine;
class Observer;

class Scene {
public:
    typedef Item::Vector Items;
    typedef Item::ID ID;
    typedef std::vector<Item::ID> ItemIDs;
    typedef std::map<Item::State, ItemIDs> ItemLists;

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

        void resetItem(ID id, PayloadPointer& payload);
        void removeItem(ID id);
        void moveItem(ID id);

        void mergeBatch(PendingChanges& newBatch);

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
    ID allocateID();

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
    Items _items;
    ItemLists _buckets;

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
