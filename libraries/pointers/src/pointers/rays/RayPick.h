//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RayPick_h
#define hifi_RayPick_h

#include <stdint.h>
#include <bitset>

#include <QtCore/QUuid>

#include <RegisteredMetaTypes.h>
#include <shared/ReadWriteLockable.h>

class RayPickFilter {
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

    RayPickFilter() {}
    RayPickFilter(const Flags& flags) : _flags(flags) {}

    bool operator== (const RayPickFilter& rhs) const { return _flags == rhs._flags; }
    bool operator!= (const RayPickFilter& rhs) const { return _flags != rhs._flags; }

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

    static const RayPickFilter NOTHING;
};

class RayPick : protected ReadWriteLockable {

public:
    using Pointer = std::shared_ptr<RayPick>;

    RayPick(const RayPickFilter& filter, const float maxDistance, const bool enabled);

    virtual const PickRay getPickRay(bool& valid) const = 0;

    void enable(bool enabled = true);
    void disable() { enable(false); }

    RayPickFilter getFilter() const;
    float getMaxDistance() const;
    bool isEnabled() const;
    RayPickResult getPrevRayPickResult() const;

    void setPrecisionPicking(bool precisionPicking);

    void setRayPickResult(const RayPickResult& rayPickResult);

    QVector<QUuid> getIgnoreItems() const;
    QVector<QUuid> getIncludeItems() const;

    template <typename T> 
    QVector<T> getIgnoreItemsAs() const {
        QVector<T> result;
        withReadLock([&] {
            for (const auto& uid : _ignoreItems) {
                result.push_back(uid);
            }
        });
        return result;
    }

    template <typename T>
    QVector<T> getIncludeItemsAs() const {
        QVector<T> result;
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
    RayPickFilter _filter;
    const float _maxDistance;
    bool _enabled;
    RayPickResult _prevResult;

    QVector<QUuid> _ignoreItems;
    QVector<QUuid> _includeItems;
};

#endif // hifi_RayPick_h
