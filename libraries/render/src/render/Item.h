//
//  Item.h
//  render/src/render
//
//  Created by Sam Gateau on 1/26/16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Item_h
#define hifi_render_Item_h

#include <atomic>
#include <bitset>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <vector>

#include <AABox.h>

#include "Args.h"

#include <graphics/Material.h>
#include "ShapePipeline.h"

namespace render {

typedef int32_t Index;
const Index INVALID_INDEX{ -1 };

class Context;

// Key is the KEY to filter Items and create specialized lists
class ItemKey {
public:
    // 8 tags are available to organize the items and filter them against as fields of the ItemKey.
    // TAG & TAG_BITS are defined from several bits in the Key.
    // An Item can be tagged and filtering can rely on the tags to keep or exclude items
    // ItemKey are not taged by default
    enum Tag : uint8_t {
        TAG_0 = 0, // 8 Tags
        TAG_1,
        TAG_2,
        TAG_3,
        TAG_4,
        TAG_5,
        TAG_6,
        TAG_7,

        NUM_TAGS
    };
    // Tag bits are derived from the Tag enum
    const static uint8_t TAG_BITS_ALL;
    const static uint8_t TAG_BITS_NONE;
    const static uint8_t TAG_BITS_0;
    const static uint8_t TAG_BITS_1;
    const static uint8_t TAG_BITS_2;
    const static uint8_t TAG_BITS_3;
    const static uint8_t TAG_BITS_4;
    const static uint8_t TAG_BITS_5;
    const static uint8_t TAG_BITS_6;
    const static uint8_t TAG_BITS_7;

    enum FlagBit : uint32_t {
        TYPE_SHAPE = 0,   // Item is a Shape
        TYPE_LIGHT,       // Item is a Light
        TYPE_META,        // Item is a Meta: meanning it s used to represent a higher level object, potentially represented by other render items

        TRANSLUCENT,      // Transparent and not opaque, for some odd reason TRANSPARENCY doesn't work...
        VIEW_SPACE,       // Transformed in view space, and not in world space
        DYNAMIC,          // Dynamic and bound will change unlike static item
        DEFORMED,         // Deformed within bound, not solid
        INVISIBLE,        // Visible or not in the scene?
        SHADOW_CASTER,    // Item cast shadows
        LAYERED,          // Item belongs to one of the layers different from the default layer

        FIRST_TAG_BIT, // 8 Tags available to organize the items and filter them against
        LAST_TAG_BIT = FIRST_TAG_BIT + NUM_TAGS,

        __SMALLER,        // Reserved bit for spatialized item to indicate that it is smaller than expected in the cell in which it belongs (probably because it overlaps over several smaller cells)

        NUM_FLAGS,      // Not a valid flag
    };
    typedef std::bitset<NUM_FLAGS> Flags;

    // All the bits touching tag bits sets to true
    const static uint32_t KEY_TAG_BITS_MASK;
    static uint32_t evalTagBitsWithKeyBits(uint8_t tagBits, const uint32_t keyBits) {
        return (keyBits & ~KEY_TAG_BITS_MASK) | (((uint32_t)tagBits) << FIRST_TAG_BIT);
    }

    // The key is the Flags
    Flags _flags;

    ItemKey() : _flags(0) {}
    ItemKey(const Flags& flags) : _flags(flags) {}

    bool operator== (const ItemKey& rhs) const { return _flags == rhs._flags; }
    bool operator!= (const ItemKey& rhs) const { return _flags != rhs._flags; }

    class Builder {
        friend class ItemKey;
        Flags _flags{ 0 };
    public:
        Builder() {}
        Builder(const ItemKey& key) : _flags{ key._flags } {}

        ItemKey build() const { return ItemKey(_flags); }

        Builder& withTypeShape() { _flags.set(TYPE_SHAPE); return (*this); }
        Builder& withTypeLight() { _flags.set(TYPE_LIGHT); return (*this); }
        Builder& withTypeMeta() { _flags.set(TYPE_META); return (*this); }
        Builder& withTransparent() { _flags.set(TRANSLUCENT); return (*this); }
        Builder& withViewSpace() { _flags.set(VIEW_SPACE); return (*this); }
        Builder& withDynamic() { _flags.set(DYNAMIC); return (*this); }
        Builder& withDeformed() { _flags.set(DEFORMED); return (*this); }
        Builder& withInvisible() { _flags.set(INVISIBLE); return (*this); }
        Builder& withShadowCaster() { _flags.set(SHADOW_CASTER); return (*this); }
        Builder& withLayered() { _flags.set(LAYERED); return (*this); }

        Builder& withTag(Tag tag) { _flags.set(FIRST_TAG_BIT + tag); return (*this); }
        // Set ALL the tags in one call using the Tag bits
        Builder& withTagBits(uint8_t tagBits) { _flags = evalTagBitsWithKeyBits(tagBits, _flags.to_ulong()); return (*this); }

        // Convenient standard keys that we will keep on using all over the place
        static Builder opaqueShape() { return Builder().withTypeShape(); }
        static Builder transparentShape() { return Builder().withTypeShape().withTransparent(); }
        static Builder light() { return Builder().withTypeLight(); }
        static Builder background() { return Builder().withViewSpace().withLayered(); }
    };
    ItemKey(const Builder& builder) : ItemKey(builder._flags) {}

    bool isShape() const { return _flags[TYPE_SHAPE]; }
    bool isLight() const { return _flags[TYPE_LIGHT]; }
    bool isMeta() const { return _flags[TYPE_META]; }

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

    bool isLayered() const { return _flags[LAYERED]; }
    bool isSpatial() const { return !isLayered(); }

    bool isTag(Tag tag) const { return _flags[FIRST_TAG_BIT + tag]; }
    uint8_t getTagBits() const { return ((_flags.to_ulong() & KEY_TAG_BITS_MASK) >> FIRST_TAG_BIT); }

    // Probably not public, flags used by the scene
    bool isSmall() const { return _flags[__SMALLER]; }
    void setSmaller(bool smaller) { (smaller ? _flags.set(__SMALLER) : _flags.reset(__SMALLER)); }

    bool operator==(const ItemKey& key) { return (_flags == key._flags); }
    bool operator!=(const ItemKey& key) { return (_flags != key._flags); }
};
using ItemKeys = std::vector<ItemKey>;

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
        Builder& withTypeMeta()         { _value.set(ItemKey::TYPE_META); _mask.set(ItemKey::TYPE_META); return (*this); }

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

        Builder& withoutLayered()       { _value.reset(ItemKey::LAYERED); _mask.set(ItemKey::LAYERED); return (*this); }
        Builder& withLayered()          { _value.set(ItemKey::LAYERED);  _mask.set(ItemKey::LAYERED); return (*this); }

        Builder& withoutTag(ItemKey::Tag tagIndex)    { _value.reset(ItemKey::FIRST_TAG_BIT + tagIndex);  _mask.set(ItemKey::FIRST_TAG_BIT + tagIndex); return (*this); }
        Builder& withTag(ItemKey::Tag tagIndex)       { _value.set(ItemKey::FIRST_TAG_BIT + tagIndex);  _mask.set(ItemKey::FIRST_TAG_BIT + tagIndex); return (*this); }
        // Set ALL the tags in one call using the Tag bits and the Tag bits touched
        Builder& withTagBits(uint8_t tagBits, uint8_t tagMask) { _value = ItemKey::evalTagBitsWithKeyBits(tagBits, _value.to_ulong()); _mask = ItemKey::evalTagBitsWithKeyBits(tagMask, _mask.to_ulong()); return (*this); }

        Builder& withNothing()          { _value.reset(); _mask.reset(); return (*this); }

        // Convenient standard keys that we will keep on using all over the place
        static Builder visibleWorldItems() { return Builder().withVisible().withWorldSpace(); }
        static Builder opaqueShape() { return Builder().withTypeShape().withOpaque().withWorldSpace(); }
        static Builder transparentShape() { return Builder().withTypeShape().withTransparent().withWorldSpace(); }
        static Builder light() { return Builder().withTypeLight(); }
        static Builder meta() { return Builder().withTypeMeta(); }
        static Builder background() { return Builder().withViewSpace().withLayered(); }
        static Builder opaqueShapeLayered() { return Builder().withTypeShape().withOpaque().withWorldSpace().withLayered(); }
        static Builder transparentShapeLayered() { return Builder().withTypeShape().withTransparent().withWorldSpace().withLayered(); }
        static Builder nothing() { return Builder().withNothing(); }
    };

    ItemFilter(const Builder& builder) : ItemFilter(builder._value, builder._mask) {}

    // Item Filter operator testing if a key pass the filter
    bool test(const ItemKey& key) const { return (key._flags & _mask) == (_value & _mask); }
    bool selectsNothing() const { return !_mask.any(); }

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

// Handy type to just pass the ID and the bound of an item
class ItemBound {
    public:
        ItemBound(ItemID id) : id(id) { }
        ItemBound(ItemID id, const AABox& bound) : id(id), bound(bound) { }

        ItemID id;
        AABox bound;
        uint32_t padding;
};

// many Item Bounds in a vector
using ItemBounds = std::vector<ItemBound>;

// Item is the proxy to a bounded "object" in the scene
// An item is described by its Key
class Item {
public:
    typedef std::vector<Item> Vector;
    typedef ItemID ID;

    static const ID INVALID_ITEM_ID;
    static const ItemCell INVALID_CELL;

    // Convenient function to clear an ID or check it s valid
    static void clearID(ID& id) { id = INVALID_ITEM_ID; }
    static bool isValidID(const ID id) { return id != INVALID_ITEM_ID; }

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
            unsigned char _color = 0xFF;
            unsigned char _icon = 0xFF;
        public:
            const static Value INVALID; // Invalid value meanss the status won't show

            Value() {}
            Value(float scale, float hue, unsigned char icon = 0xFF) { setScale(scale); setColor(hue); setIcon(icon); }

            // It can be scaled in the range [0, 1]
            void setScale(float scale);
            // the color hue  in the range [0, 360] representing the color wheel hue
            void setColor(float hue);
            // the icon to display in the range [0, 255], where 0 means no icon, just filled quad and anything else would
            // hopefully have an icon available to display (see DrawStatusJob)
            void setIcon(unsigned char icon);

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

        size_t getNumValues() const { return _values.size(); }

        using Values = std::vector <Value>;
        Values getCurrentValues() const;
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

        virtual const ShapeKey getShapeKey() const = 0;

        virtual uint32_t fetchMetaSubItems(ItemIDs& subItems) const = 0;

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

    // Item exists if it has a valid payload
    bool exist() const { return (bool)(_payload); }

    // Main scene / item managment interface reset/update/kill
    void resetPayload(const PayloadPointer& payload);
    void resetCell(ItemCell cell = INVALID_CELL, bool _small = false) { _cell = cell; _key.setSmaller(_small); }
    void update(const UpdateFunctorPointer& updateFunctor); // communicate update to payload
    void kill() { _payload.reset(); resetCell(); _key._flags.reset(); } // forget the payload, key, cell

    // Check heuristic key
    const ItemKey& getKey() const { return _key; }

    // Check spatial cell
    const ItemCell& getCell() const { return _cell; }

    // Payload Interface

    // Get the bound of the item expressed in world space (or eye space depending on the key.isWorldSpace())
    const Bound getBound() const { return _payload->getBound(); }

    // Get the layer where the item belongs.
    int getLayer() const { return _payload->getLayer(); }

    static const int LAYER_2D;
    static const int LAYER_3D;
    static const int LAYER_3D_FRONT;
    static const int LAYER_3D_HUD;

    // Render call for the item
    void render(RenderArgs* args) const { _payload->render(args); }

    // Shape Type Interface
    const ShapeKey getShapeKey() const;

    // Meta Type Interface
    uint32_t fetchMetaSubItems(ItemIDs& subItems) const { return _payload->fetchMetaSubItems(subItems); }

    // Access the status
    const StatusPointer& getStatus() const { return _payload->getStatus(); }

    void setTransitionId(Index id) { _transitionId = id; }
    Index getTransitionId() const { return _transitionId; }

protected:
    PayloadPointer _payload;
    ItemKey _key;
    ItemCell _cell{ INVALID_CELL };
    Index _transitionId{ INVALID_INDEX };

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

// Item shared interface supported by the payload
template <class T> const ItemKey payloadGetKey(const std::shared_ptr<T>& payloadData) { return ItemKey(); }
template <class T> const Item::Bound payloadGetBound(const std::shared_ptr<T>& payloadData) { return Item::Bound(); }
template <class T> int payloadGetLayer(const std::shared_ptr<T>& payloadData) { return 0; }
template <class T> void payloadRender(const std::shared_ptr<T>& payloadData, RenderArgs* args) { }

// Shape type interface
// This allows shapes to characterize their pipeline via a ShapeKey, to be picked with a subclass of Shape.
// When creating a new shape payload you need to create a specialized version, or the ShapeKey will be ownPipeline,
// implying that the shape will setup its own pipeline without the use of the ShapeKey.
template <class T> const ShapeKey shapeGetShapeKey(const std::shared_ptr<T>& payloadData) { return ShapeKey::Builder::ownPipeline(); }

// Meta Type Interface
// Meta items act as the grouping object for several sub items (typically shapes).
template <class T> uint32_t metaFetchMetaSubItems(const std::shared_ptr<T>& payloadData, ItemIDs& subItems) { return 0; }

// THe Payload class is the real Payload to be used
// THis allow anything to be turned into a Payload as long as the required interface functions are available
// When creating a new kind of payload from a new "stuff" class then you need to create specialized version for "stuff"
// of the Payload interface
template <class T> class Payload : public Item::PayloadInterface {
public:
    typedef std::shared_ptr<T> DataPointer;
    typedef UpdateFunctor<T> Updater;

    Payload(const DataPointer& data) : _data(data) {}

    // Payload general interface
    virtual const ItemKey getKey() const override { return payloadGetKey<T>(_data); }
    virtual const Item::Bound getBound() const override { return payloadGetBound<T>(_data); }
    virtual int getLayer() const override { return payloadGetLayer<T>(_data); }

    virtual void render(RenderArgs* args) override { payloadRender<T>(_data, args); }

    // Shape Type interface
    virtual const ShapeKey getShapeKey() const override { return shapeGetShapeKey<T>(_data); }

    // Meta Type Interface
    virtual uint32_t fetchMetaSubItems(ItemIDs& subItems) const override { return metaFetchMetaSubItems<T>(_data, subItems); }

protected:
    DataPointer _data;

    // Update mechanics
    virtual void update(const UpdateFunctorPointer& functor) override {
        std::static_pointer_cast<Updater>(functor)->_func((*_data));
    }
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

class PayloadProxyInterface {
public:
    using ProxyPayload = Payload<PayloadProxyInterface>;
    using Pointer = ProxyPayload::DataPointer;

    virtual ItemKey getKey() = 0;
    virtual ShapeKey getShapeKey() = 0;
    virtual Item::Bound getBound() = 0;
    virtual void render(RenderArgs* args) = 0;
    virtual uint32_t metaFetchMetaSubItems(ItemIDs& subItems) = 0;
};

template <> const ItemKey payloadGetKey(const PayloadProxyInterface::Pointer& payload);
template <> const Item::Bound payloadGetBound(const PayloadProxyInterface::Pointer& payload);
template <> void payloadRender(const PayloadProxyInterface::Pointer& payload, RenderArgs* args);
template <> uint32_t metaFetchMetaSubItems(const PayloadProxyInterface::Pointer& payload, ItemIDs& subItems);
template <> const ShapeKey shapeGetShapeKey(const PayloadProxyInterface::Pointer& payload);


typedef Item::PayloadPointer PayloadPointer;
typedef std::vector< PayloadPointer > Payloads;

// A map of items by ShapeKey to optimize rendering pipeline assignments
using ShapeBounds = std::unordered_map<ShapeKey, ItemBounds, ShapeKey::Hash, ShapeKey::KeyEqual>;

}

#endif // hifi_render_Item_h
