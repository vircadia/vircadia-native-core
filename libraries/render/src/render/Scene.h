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
        UNVISIBLE,      // Visible or not? could be just here to cast shadow
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
        virtual const State&& getState() const = 0;
        virtual const Bound&& getBound() const = 0;
        virtual void render(Context& context) = 0;

        ~PayloadInterface() {}
    protected:
    };

    template <class T> class Payload : public PayloadInterface {
    public:
        virtual const State&& getState() const { return getState<T>(*this); }
        virtual const Bound&& getBound() const { return getBound<T>(*this); }
        virtual void render(Context& context) { render<T>(this*, context); }
    protected:
    };

    typedef std::shared_ptr<PayloadInterface> PayloadPointer;

    Item(PayloadPointer& payload):
        _payload(payload) {}

    ~Item() {}

    // Check heuristic flags of the state
    const State& getState() const { return _state; }

    bool isOpaque() const { return !_state[TRANSLUCENT]; }
    bool isTranslucent() const { return _state[TRANSLUCENT]; }

    bool isWorldSpace() const { return !_state[VIEW_SPACE]; }
    bool isViewSpace() const { return _state[VIEW_SPACE]; }
 
    bool isStatic() const { return !_state[DYNAMIC]; }
    bool isDynamic() const { return _state[DYNAMIC]; }
    bool isDeformed() const { return _state[DEFORMED]; }
 
    bool isVisible() const { return !_state[UNVISIBLE]; }
    bool isUnvisible() const { return _state[UNVISIBLE]; }

    bool isShadowCaster() const { return _state[SHADOW_CASTER]; }

    bool isPickable() const { return _state[PICKABLE]; }
 
    // Payload Interface
    const Bound&& getBound() const { return _payload->getBound(); }
    void render(Context& context) { _payload->render(context); }

protected:
    PayloadPointer _payload;
    State _state;

    friend class Scene;
};

typedef Item::PayloadInterface ItemData;
typedef Item::PayloadPointer ItemDataPointer;

class Scene {
public:
    typedef Item::Vector Items;
    typedef Item::ID ID;
    typedef std::vector<Item::ID> ItemList;
    typedef std::map<Item::State, ItemList> ItemLists;

    enum ChangeType {
        ADD = 0,
        REMOVE,
        MOVE,
        RESET,

        NUM_CHANGE_TYPES,
    };
    typedef ItemList ChangeLists[NUM_CHANGE_TYPES];
    class ChangePacket {
    public:
        ChangePacket(int frame) :
            _frame(frame) {}
        ~ChangePacket();

        int _frame;
        ChangeLists _changeLists;
    protected:
    };

    Scene() {}
    ~Scene() {}


    ID addItem(ItemDataPointer& itemData);
    void removeItem(ID id);
    void moveItem(ID id);


protected:
    Items _items;
    ItemLists _buckets;
};

}

#endif // hifi_render_Scene_h
