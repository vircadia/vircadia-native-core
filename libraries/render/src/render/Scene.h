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

#include <atomic>
#include <bitset>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <vector>

#include <AABox.h>
#include <RenderArgs.h>

#include "model/Material.h"

namespace render {

class Context;
    
// Key is the KEY to filter Items and create specialized lists
class ItemKey {
public:
    enum FlagBit {
        TYPE_SHAPE = 0,   // Item is a Shape
        TYPE_LIGHT,       // Item is a Light
        TRANSLUCENT,      // Transparent and not opaque, for some odd reason TRANSPARENCY doesn't work...
        VIEW_SPACE,       // Transformed in view space, and not in world space
        DYNAMIC,          // Dynamic and bound will change unlike static item
        DEFORMED,         // Deformed within bound, not solid
        INVISIBLE,        // Visible or not? could be just here to cast shadow
        SHADOW_CASTER,    // Item cast shadows
        PICKABLE,         // Item can be picked/selected
        LAYERED,          // Item belongs to one of the layers different from the default layer

        NUM_FLAGS,      // Not a valid flag
    };
    typedef std::bitset<NUM_FLAGS> Flags; 
    
    // The key is the Flags
    Flags _flags;

    ItemKey() : _flags(0) {}
    ItemKey(const Flags& flags) : _flags(flags) {}

    class Builder {
        friend class ItemKey;
        Flags _flags{ 0 };
    public:
        Builder() {}

        ItemKey build() const { return ItemKey(_flags); }

        Builder& withTypeShape() { _flags.set(TYPE_SHAPE); return (*this); }
        Builder& withTypeLight() { _flags.set(TYPE_LIGHT); return (*this); }
        Builder& withTransparent() { _flags.set(TRANSLUCENT); return (*this); }
        Builder& withViewSpace() { _flags.set(VIEW_SPACE); return (*this); }
        Builder& withDynamic() { _flags.set(DYNAMIC); return (*this); }
        Builder& withDeformed() { _flags.set(DEFORMED); return (*this); }
        Builder& withInvisible() { _flags.set(INVISIBLE); return (*this); }
        Builder& withShadowCaster() { _flags.set(SHADOW_CASTER); return (*this); }
        Builder& withPickable() { _flags.set(PICKABLE); return (*this); }
        Builder& withLayered() { _flags.set(LAYERED); return (*this); }

        // Convenient standard keys that we will keep on using all over the place
        static Builder opaqueShape() { return Builder().withTypeShape(); }
        static Builder transparentShape() { return Builder().withTypeShape().withTransparent(); }
        static Builder light() { return Builder().withTypeLight(); }
        static Builder background() { return Builder().withViewSpace().withLayered(); }
    };
    ItemKey(const Builder& builder) : ItemKey(builder._flags) {}

    bool isOpaque() const { return !_flags[TRANSLUCENT]; }
    bool isTransparent() const { return _flags[TRANSLUCENT]; }

    bool isWorldSpace() const { return !_flags[VIEW_SPACE]; }
    bool isViewSpace() const { return _flags[VIEW_SPACE]; }

    bool isStatic() const { return !_flags[DYNAMIC]; }
    bool isDynamic() const { return _flags[DYNAMIC]; }

    bool isRigid() const { return !_flags[DEFORMED]; }
    bool isDeformed() const { return _flags[DEFORMED]; }
 
    bool isVisible() const { return !_flags[INVISIBLE]; }
    bool isInvisible() const { return _flags[INVISIBLE]; }

    bool isShadowCaster() const { return _flags[SHADOW_CASTER]; }

    bool isPickable() const { return _flags[PICKABLE]; }

    bool isLayered() const { return _flags[LAYERED]; }
};

inline QDebug operator<<(QDebug debug, const ItemKey& itemKey) {
    debug << "[ItemKey: isOpaque:" << itemKey.isOpaque()
                 << ", isStatic:" << itemKey.isStatic() 
                 << ", isWorldSpace:" << itemKey.isWorldSpace() 
                 << "]";
    return debug;
}
 
class ItemFilter {
public:
    ItemKey::Flags _value{ 0 };
    ItemKey::Flags _mask{ 0 };


    ItemFilter(const ItemKey::Flags& value = ItemKey::Flags(0), const ItemKey::Flags& mask = ItemKey::Flags(0)) : _value(value), _mask(mask) {}

    class Builder {
        friend class ItemFilter;
        ItemKey::Flags _value{ 0 };
        ItemKey::Flags _mask{ 0 };
    public:
        Builder() {}

        ItemFilter build() const { return ItemFilter(_value, _mask); }

        Builder& withTypeShape()        { _value.set(ItemKey::TYPE_SHAPE); _mask.set(ItemKey::TYPE_SHAPE); return (*this); }
        Builder& withTypeLight()        { _value.set(ItemKey::TYPE_LIGHT); _mask.set(ItemKey::TYPE_LIGHT); return (*this); }
        
        Builder& withOpaque()           { _value.reset(ItemKey::TRANSLUCENT); _mask.set(ItemKey::TRANSLUCENT); return (*this); }
        Builder& withTransparent()      { _value.set(ItemKey::TRANSLUCENT); _mask.set(ItemKey::TRANSLUCENT); return (*this); }
        
        Builder& withWorldSpace()       { _value.reset(ItemKey::VIEW_SPACE); _mask.set(ItemKey::VIEW_SPACE); return (*this); }
        Builder& withViewSpace()        { _value.set(ItemKey::VIEW_SPACE);  _mask.set(ItemKey::VIEW_SPACE); return (*this); }

        Builder& withStatic()           { _value.reset(ItemKey::DYNAMIC); _mask.set(ItemKey::DYNAMIC); return (*this); }
        Builder& withDynamic()          { _value.set(ItemKey::DYNAMIC);  _mask.set(ItemKey::DYNAMIC); return (*this); }

        Builder& withRigid()            { _value.reset(ItemKey::DEFORMED); _mask.set(ItemKey::DEFORMED); return (*this); }
        Builder& withDeformed()         { _value.set(ItemKey::DEFORMED);  _mask.set(ItemKey::DEFORMED); return (*this); }

        Builder& withVisible()          { _value.reset(ItemKey::INVISIBLE); _mask.set(ItemKey::INVISIBLE); return (*this); }
        Builder& withInvisible()        { _value.set(ItemKey::INVISIBLE);  _mask.set(ItemKey::INVISIBLE); return (*this); }

        Builder& withNoShadowCaster()   { _value.reset(ItemKey::SHADOW_CASTER); _mask.set(ItemKey::SHADOW_CASTER); return (*this); }
        Builder& withShadowCaster()     { _value.set(ItemKey::SHADOW_CASTER);  _mask.set(ItemKey::SHADOW_CASTER); return (*this); }

        Builder& withPickable()         { _value.set(ItemKey::PICKABLE);  _mask.set(ItemKey::PICKABLE); return (*this); }

        Builder& withoutLayered()       { _value.reset(ItemKey::LAYERED); _mask.set(ItemKey::LAYERED); return (*this); }
        Builder& withLayered()          { _value.set(ItemKey::LAYERED);  _mask.set(ItemKey::LAYERED); return (*this); }

        // Convenient standard keys that we will keep on using all over the place
        static Builder opaqueShape() { return Builder().withTypeShape().withOpaque().withWorldSpace(); }
        static Builder transparentShape() { return Builder().withTypeShape().withTransparent().withWorldSpace(); }
        static Builder light() { return Builder().withTypeLight(); }
        static Builder background() { return Builder().withViewSpace().withLayered(); }
    };

    ItemFilter(const Builder& builder) : ItemFilter(builder._value, builder._mask) {}

    // Item Filter operator testing if a key pass the filter
    bool test(const ItemKey& key) const { return (key._flags & _mask) == (_value & _mask); }

    class Less {
    public:
        bool operator() (const ItemFilter& left, const ItemFilter& right) const {
            if (left._value.to_ulong() == right._value.to_ulong()) {
                return left._mask.to_ulong() < right._mask.to_ulong();
            } else {
                return left._value.to_ulong() < right._value.to_ulong();
            }
        }
    };
};

inline QDebug operator<<(QDebug debug, const ItemFilter& me) {
    debug << "[ItemFilter: opaqueShape:" << me.test(ItemKey::Builder::opaqueShape().build())
                 << "]";
    return debug;
}


class Item {
public:
    typedef std::vector<Item> Vector;
    typedef unsigned int ID;

    static const ID INVALID_ITEM_ID = 0;

    // Bound is the AABBox fully containing this item
    typedef AABox Bound;
    
    // Status records the life history and performances of this item while performing at rendering and updating.
    // This is Used for monitoring and dynamically adjust the quality 
    class Status {
    public:

        // Status::Value class is the data used to represent the transient information of a status as a square icon
        // The "icon" is a square displayed in the 3D scene over the render::Item AABB center.
        // It can be scaled in the range [0, 1] and the color hue  in the range [0, 360] representing the color wheel hue
        class Value {
            unsigned short _scale = 0xFFFF;
            unsigned short _color = 0xFFFF;
        public:
            const static Value INVALID; // Invalid value meanss the status won't show

            Value() {}
            Value(float scale, float hue) { setScale(scale); setColor(hue); }

            // It can be scaled in the range [0, 1] 
            void setScale(float scale);
            // the color hue  in the range [0, 360] representing the color wheel hue
            void setColor(float hue);

            // Standard color Hue
            static const float RED; // 0.0f;
            static const float YELLOW; // 60.0f;
            static const float GREEN; // 120.0f;
            static const float CYAN; // 180.0f;
            static const float BLUE; // 240.0f;
            static const float MAGENTA; // 300.0f;

            // Retreive the Value data tightely packed as an int
            int getPackedData() const { return *((const int*) this); }
        };

        typedef std::function<Value()> Getter;
        typedef std::vector<Getter> Getters;

        Getters _values;

        void addGetter(const Getter& getter) { _values.push_back(getter); }

        void getPackedValues(glm::ivec4& values) const;
    };
    typedef std::shared_ptr<Status> StatusPointer;

    // Update Functor
    class UpdateFunctorInterface {
    public:
        virtual ~UpdateFunctorInterface() {}
    };
    typedef std::shared_ptr<UpdateFunctorInterface> UpdateFunctorPointer;

    // Payload is whatever is in this Item and implement the Payload Interface
    class PayloadInterface {
    public:
        virtual const ItemKey getKey() const = 0;
        virtual const Bound getBound() const = 0;
        virtual int getLayer() const = 0;

        virtual void render(RenderArgs* args) = 0;

        virtual const model::MaterialKey getMaterialKey() const = 0;

        ~PayloadInterface() {}

        // Status interface is local to the base class
        const StatusPointer& getStatus() const { return _status; }
        void addStatusGetter(const Status::Getter& getter);
        void addStatusGetters(const Status::Getters& getters);

    protected:
        StatusPointer _status;

        friend class Item;
        virtual void update(const UpdateFunctorPointer& functor) = 0;
    };
    typedef std::shared_ptr<PayloadInterface> PayloadPointer;

    Item() {}
    ~Item() {}

    // Main scene / item managment interface Reset/Update/Kill
    void resetPayload(const PayloadPointer& payload);
    void update(const UpdateFunctorPointer& updateFunctor)  { _payload->update(updateFunctor); } // Communicate update to the payload
    void kill() { _payload.reset(); _key._flags.reset(); } // Kill means forget the payload and key

    // Check heuristic key
    const ItemKey& getKey() const { return _key; }

    // Payload Interface

    // Get the bound of the item expressed in world space (or eye space depending on the key.isWorldSpace())
    const Bound getBound() const { return _payload->getBound(); }

    // Get the layer where the item belongs. 0 by default meaning NOT LAYERED
    int getLayer() const { return _payload->getLayer(); }

    // Render call for the item
    void render(RenderArgs* args) { _payload->render(args); }

    // Shape Type Interface
    const model::MaterialKey getMaterialKey() const { return _payload->getMaterialKey(); }

    // Access the status
    const StatusPointer& getStatus() const { return _payload->getStatus(); }
    glm::ivec4 getStatusPackedValues() const;

protected:
    PayloadPointer _payload;
    ItemKey _key;

    friend class Scene;
};


typedef Item::UpdateFunctorInterface UpdateFunctorInterface;
typedef Item::UpdateFunctorPointer UpdateFunctorPointer;
typedef std::vector<UpdateFunctorPointer> UpdateFunctors;

template <class T> class UpdateFunctor : public Item::UpdateFunctorInterface {
public:
    typedef std::function<void(T&)> Func;
    Func _func;

    UpdateFunctor(Func func): _func(func) {}
    ~UpdateFunctor() {}
};


inline QDebug operator<<(QDebug debug, const Item& item) {
    debug << "[Item: _key:" << item.getKey() << ", bounds:" << item.getBound() << "]";
    return debug;
}

// THe Payload class is the real Payload to be used
// THis allow anything to be turned into a Payload as long as the required interface functions are available
// When creating a new kind of payload from a new "stuff" class then you need to create specialized version for "stuff"
// of the Payload interface
template <class T> const ItemKey payloadGetKey(const std::shared_ptr<T>& payloadData) { return ItemKey(); }
template <class T> const Item::Bound payloadGetBound(const std::shared_ptr<T>& payloadData) { return Item::Bound(); }
template <class T> int payloadGetLayer(const std::shared_ptr<T>& payloadData) { return 0; }
template <class T> void payloadRender(const std::shared_ptr<T>& payloadData, RenderArgs* args) { }
    
// Shape type interface
template <class T> const model::MaterialKey shapeGetMaterialKey(const std::shared_ptr<T>& payloadData) { return model::MaterialKey(); }

template <class T> class Payload : public Item::PayloadInterface {
public:
    typedef std::shared_ptr<T> DataPointer;
    typedef UpdateFunctor<T> Updater;

    Payload(const DataPointer& data) : _data(data) {}

    // Payload general interface
    virtual const ItemKey getKey() const { return payloadGetKey<T>(_data); }
    virtual const Item::Bound getBound() const { return payloadGetBound<T>(_data); }
    virtual int getLayer() const { return payloadGetLayer<T>(_data); }


    virtual void render(RenderArgs* args) { payloadRender<T>(_data, args); } 

    // Shape Type interface
    virtual const model::MaterialKey getMaterialKey() const { return shapeGetMaterialKey<T>(_data); }

protected:
    DataPointer _data;

    // Update mechanics
    virtual void update(const UpdateFunctorPointer& functor) { std::static_pointer_cast<Updater>(functor)->_func((*_data)); }
    friend class Item;
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

class ItemIDAndBounds {
public:
    ItemIDAndBounds(ItemID id) : id(id) { }
    ItemIDAndBounds(ItemID id, const AABox& bounds) : id(id), bounds(bounds) { }
    
    ItemID id;
    AABox bounds;
};

typedef std::vector< ItemIDAndBounds > ItemIDsBounds;


// A map of ItemIDSets allowing to create bucket lists of items which are filtering correctly
class ItemBucketMap : public std::map<ItemFilter, ItemIDSet, ItemFilter::Less> {
public:

    ItemBucketMap() {}

    void insert(const ItemID& id, const ItemKey& key);
    void erase(const ItemID& id, const ItemKey& key);
    void reset(const ItemID& id, const ItemKey& oldKey, const ItemKey& newKey);

    // standard builders allocating the main buckets
    void allocateStandardOpaqueTranparentBuckets();
    
};

class Engine;

class PendingChanges {
public:
    PendingChanges() {}
    ~PendingChanges() {}

    void resetItem(ItemID id, const PayloadPointer& payload);
    void removeItem(ItemID id);

    template <class T> void updateItem(ItemID id, std::function<void(T&)> func) {
        updateItem(id, std::make_shared<UpdateFunctor<T>>(func));
    }

    void updateItem(ItemID id, const UpdateFunctorPointer& functor);

    void merge(PendingChanges& changes);

    Payloads _resetPayloads;
    ItemIDs _resetItems; 
    ItemIDs _removedItems;
    ItemIDs _updatedItems;
    UpdateFunctors _updateFunctors;

protected:
};
typedef std::queue<PendingChanges> PendingChangesQueue;


// Scene is a container for Items
// Items are introduced, modified or erased in the scene through PendingChanges
// Once per Frame, the PendingChanges are all flushed
// During the flush the standard buckets are updated
// Items are notified accordingly on any update message happening
class Scene {
public:
    Scene();
    ~Scene() {}

    /// This call is thread safe, can be called from anywhere to allocate a new ID
    ItemID allocateID();

    /// Enqueue change batch to the scene
    void enqueuePendingChanges(const PendingChanges& pendingChanges);

    /// Access the main bucketmap of items
    const ItemBucketMap& getMasterBucket() const { return _masterBucketMap; }

    /// Access a particular item form its ID
    /// WARNING, There is No check on the validity of the ID, so this could return a bad Item
    const Item& getItem(const ItemID& id) const { return _items[id]; }

    unsigned int getNumItems() const { return _items.size(); }


    void processPendingChangesQueue();

protected:
    // Thread safe elements that can be accessed from anywhere
    std::atomic<unsigned int> _IDAllocator{ 1 }; // first valid itemID will be One
    std::mutex _changeQueueMutex;
    PendingChangesQueue _changeQueue;

    // The actual database
    // database of items is protected for editing by a mutex
    std::mutex _itemsMutex;
    Item::Vector _items;
    ItemBucketMap _masterBucketMap;

    void resetItems(const ItemIDs& ids, Payloads& payloads);
    void removeItems(const ItemIDs& ids);
    void updateItems(const ItemIDs& ids, UpdateFunctors& functors);

    friend class Engine;
};



typedef std::shared_ptr<Scene> ScenePointer;
typedef std::vector<ScenePointer> Scenes;

}

#endif // hifi_render_Scene_h
