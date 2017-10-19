//
//  Created by Sam Gondelman 10/17/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_Pick_h
#define hifi_Pick_h

#include <memory>
#include <stdint.h>
#include <bitset>

#include <QtCore/QUuid>
#include <QVector>
#include <QVariant>

#include <shared/ReadWriteLockable.h>

enum IntersectionType {
    NONE = 0,
    ENTITY,
    OVERLAY,
    AVATAR,
    HUD
};

class PickFilter {
public:
    enum FlagBit {
        PICK_ENTITIES = 0,
        PICK_OVERLAYS,
        PICK_AVATARS,
        PICK_HUD,

        PICK_COARSE, // if not set, does precise intersection, otherwise, doesn't

        PICK_INCLUDE_INVISIBLE, // if not set, will not intersect invisible elements, otherwise, intersects both visible and invisible elements
        PICK_INCLUDE_NONCOLLIDABLE, // if not set, will not intersect noncollidable elements, otherwise, intersects both collidable and noncollidable elements

        // NOT YET IMPLEMENTED
        PICK_ALL_INTERSECTIONS, // if not set, returns closest intersection, otherwise, returns list of all intersections

        NUM_FLAGS, // Not a valid flag
    };
    typedef std::bitset<NUM_FLAGS> Flags;

    // The key is the Flags
    Flags _flags;

    PickFilter() {}
    PickFilter(const Flags& flags) : _flags(flags) {}

    bool operator== (const PickFilter& rhs) const { return _flags == rhs._flags; }
    bool operator!= (const PickFilter& rhs) const { return _flags != rhs._flags; }

    void setFlag(FlagBit flag, bool value) { _flags[flag] = value; }

    bool doesPickNothing() const { return _flags == NOTHING._flags; }
    bool doesPickEntities() const { return _flags[PICK_ENTITIES]; }
    bool doesPickOverlays() const { return _flags[PICK_OVERLAYS]; }
    bool doesPickAvatars() const { return _flags[PICK_AVATARS]; }
    bool doesPickHUD() const { return _flags[PICK_HUD]; }

    bool doesPickCoarse() const { return _flags[PICK_COARSE]; }
    bool doesPickInvisible() const { return _flags[PICK_INCLUDE_INVISIBLE]; }
    bool doesPickNonCollidable() const { return _flags[PICK_INCLUDE_NONCOLLIDABLE]; }

    bool doesWantAllIntersections() const { return _flags[PICK_ALL_INTERSECTIONS]; }

    // Helpers for RayPickManager
    Flags getEntityFlags() const {
        unsigned int toReturn = getBitMask(PICK_ENTITIES);
        if (doesPickInvisible()) {
            toReturn |= getBitMask(PICK_INCLUDE_INVISIBLE);
        }
        if (doesPickNonCollidable()) {
            toReturn |= getBitMask(PICK_INCLUDE_NONCOLLIDABLE);
        }
        if (doesPickCoarse()) {
            toReturn |= getBitMask(PICK_COARSE);
        }
        return Flags(toReturn);
    }
    Flags getOverlayFlags() const {
        unsigned int toReturn = getBitMask(PICK_OVERLAYS);
        if (doesPickInvisible()) {
            toReturn |= getBitMask(PICK_INCLUDE_INVISIBLE);
        }
        if (doesPickNonCollidable()) {
            toReturn |= getBitMask(PICK_INCLUDE_NONCOLLIDABLE);
        }
        if (doesPickCoarse()) {
            toReturn |= getBitMask(PICK_COARSE);
        }
        return Flags(toReturn);
    }
    Flags getAvatarFlags() const { return Flags(getBitMask(PICK_AVATARS)); }
    Flags getHUDFlags() const { return Flags(getBitMask(PICK_HUD)); }

    static constexpr unsigned int getBitMask(FlagBit bit) { return 1 << bit; }

    static const PickFilter NOTHING;
};

class PickResult {
public:
    PickResult() {}
    PickResult(const QVariantMap& pickVariant) : pickVariant(pickVariant) {}

    virtual QVariantMap toVariantMap() const {
        return pickVariant;
    }

    virtual bool doesIntersect() const = 0;

    // for example: if we want the closest result, compare based on distance
    // if we want all results, combine them
    // must return a new pointer
    virtual std::shared_ptr<PickResult> compareAndProcessNewResult(const std::shared_ptr<PickResult> newRes) = 0;

    // returns true if this result contains any valid results with distance < maxDistance
    // can also filter out results with distance >= maxDistance
    virtual bool checkOrFilterAgainstMaxDistance(float maxDistance) = 0;

    QVariantMap pickVariant;
};

using PickResultPointer = std::shared_ptr<PickResult>;

template<typename T>
class Pick : protected ReadWriteLockable {

public:
    Pick(const PickFilter& filter, const float maxDistance, const bool enabled);

    virtual const T getMathematicalPick() const = 0;
    virtual PickResultPointer getDefaultResult(const QVariantMap& pickVariant) const = 0;
    virtual PickResultPointer getEntityIntersection(const T& pick) = 0;
    virtual PickResultPointer getOverlayIntersection(const T& pick) = 0;
    virtual PickResultPointer getAvatarIntersection(const T& pick) = 0;
    virtual PickResultPointer getHUDIntersection(const T& pick) = 0;

    void enable(bool enabled = true);
    void disable() { enable(false); }

    PickFilter getFilter() const;
    float getMaxDistance() const;
    bool isEnabled() const;

    void setPrecisionPicking(bool precisionPicking);

    PickResultPointer getPrevPickResult() const;
    void setPickResult(const PickResultPointer& pickResult);

    QVector<QUuid> getIgnoreItems() const;
    QVector<QUuid> getIncludeItems() const;

    template <typename S>
    QVector<S> getIgnoreItemsAs() const {
        QVector<S> result;
        withReadLock([&] {
            for (const auto& uid : _ignoreItems) {
                result.push_back(uid);
            }
        });
        return result;
    }

    template <typename S>
    QVector<S> getIncludeItemsAs() const {
        QVector<S> result;
        withReadLock([&] {
            for (const auto& uid : _includeItems) {
                result.push_back(uid);
            }
        });
        return result;
    }

    void setIgnoreItems(const QVector<QUuid>& items);
    void setIncludeItems(const QVector<QUuid>& items);

private:
    PickFilter _filter;
    const float _maxDistance;
    bool _enabled;
    PickResultPointer _prevResult;

    QVector<QUuid> _ignoreItems;
    QVector<QUuid> _includeItems;
};

template<typename T>
Pick<T>::Pick(const PickFilter& filter, const float maxDistance, const bool enabled) :
    _filter(filter),
    _maxDistance(maxDistance),
    _enabled(enabled) {
}

template<typename T>
void Pick<T>::enable(bool enabled) {
    withWriteLock([&] {
        _enabled = enabled;
    });
}

template<typename T>
PickFilter Pick<T>::getFilter() const {
    return resultWithReadLock<PickFilter>([&] {
        return _filter;
    });
}

template<typename T>
float Pick<T>::getMaxDistance() const {
    return _maxDistance;
}

template<typename T>
bool Pick<T>::isEnabled() const {
    return resultWithReadLock<bool>([&] {
        return _enabled;
    });
}

template<typename T>
void Pick<T>::setPrecisionPicking(bool precisionPicking) {
    withWriteLock([&] {
        _filter.setFlag(PickFilter::PICK_COARSE, !precisionPicking);
    });
}

template<typename T>
void Pick<T>::setPickResult(const PickResultPointer& pickResult) {
    withWriteLock([&] {
        _prevResult = pickResult;
    });
}

template<typename T>
QVector<QUuid> Pick<T>::getIgnoreItems() const {
    return resultWithReadLock<QVector<QUuid>>([&] {
        return _ignoreItems;
    });
}

template<typename T>
QVector<QUuid> Pick<T>::getIncludeItems() const {
    return resultWithReadLock<QVector<QUuid>>([&] {
        return _includeItems;
    });
}

template<typename T>
PickResultPointer Pick<T>::getPrevPickResult() const {
    return resultWithReadLock<PickResultPointer>([&] {
        return _prevResult;
    });
}

template<typename T>
void Pick<T>::setIgnoreItems(const QVector<QUuid>& ignoreItems) {
    withWriteLock([&] {
        _ignoreItems = ignoreItems;
    });
}

template<typename T>
void Pick<T>::setIncludeItems(const QVector<QUuid>& includeItems) {
    withWriteLock([&] {
        _includeItems = includeItems;
    });
}

#endif // hifi_Pick_h
