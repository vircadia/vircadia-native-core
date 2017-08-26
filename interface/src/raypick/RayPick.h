//
//  RayPick.h
//  interface/src/raypick
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
#include "RegisteredMetaTypes.h"

#include "EntityItemID.h"
#include "ui/overlays/Overlay.h"

class RayPickFilter {
public:
    enum FlagBit {
        PICK_NOTHING = 0,
        PICK_ENTITIES,
        PICK_OVERLAYS,
        PICK_AVATARS,
        PICK_HUD,

        PICK_COURSE, // if not set, does precise intersection, otherwise, doesn't

        PICK_INCLUDE_INVISIBLE, // if not set, will not intersect invisible elements, otherwise, intersects both visible and invisible elements
        PICK_INCLUDE_NONCOLLIDABLE, // if not set, will not intersect noncollidable elements, otherwise, intersects both collidable and noncollidable elements

        // NOT YET IMPLEMENTED
        PICK_ALL_INTERSECTIONS, // if not set, returns closest intersection, otherwise, returns list of all intersections

        NUM_FLAGS, // Not a valid flag
    };
    typedef std::bitset<NUM_FLAGS> Flags;

    // The key is the Flags
    Flags _flags;

    RayPickFilter() : _flags(getBitMask(PICK_NOTHING)) {}
    RayPickFilter(const Flags& flags) : _flags(flags) {}

    bool operator== (const RayPickFilter& rhs) const { return _flags == rhs._flags; }
    bool operator!= (const RayPickFilter& rhs) const { return _flags != rhs._flags; }

    void setFlag(FlagBit flag, bool value) { _flags[flag] = value; }

    bool doesPickNothing() const { return _flags[PICK_NOTHING]; }
    bool doesPickEntities() const { return _flags[PICK_ENTITIES]; }
    bool doesPickOverlays() const { return _flags[PICK_OVERLAYS]; }
    bool doesPickAvatars() const { return _flags[PICK_AVATARS]; }
    bool doesPickHUD() const { return _flags[PICK_HUD]; }

    bool doesPickCourse() const { return _flags[PICK_COURSE]; }
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
        return Flags(toReturn);
    }
    Flags getAvatarFlags() const { return Flags(getBitMask(PICK_AVATARS)); }
    Flags getHUDFlags() const { return Flags(getBitMask(PICK_HUD)); }

    static unsigned int getBitMask(FlagBit bit) { return 1 << bit; }

};

class RayPick {

public:
    RayPick(const RayPickFilter& filter, const float maxDistance, const bool enabled);

    virtual const PickRay getPickRay(bool& valid) const = 0;

    void enable() { _enabled = true; }
    void disable() { _enabled = false; }

    const RayPickFilter& getFilter() { return _filter; }
    float getMaxDistance() { return _maxDistance; }
    bool isEnabled() { return _enabled; }
    const RayPickResult& getPrevRayPickResult() { return _prevResult; }

    void setPrecisionPicking(bool precisionPicking) { _filter.setFlag(RayPickFilter::PICK_COURSE, !precisionPicking); }

    void setRayPickResult(const RayPickResult& rayPickResult) { _prevResult = rayPickResult; }

    const QVector<EntityItemID>& getIgnoreEntites() { return _ignoreEntities; }
    const QVector<EntityItemID>& getIncludeEntites() { return _includeEntities; }
    const QVector<OverlayID>& getIgnoreOverlays() { return _ignoreOverlays; }
    const QVector<OverlayID>& getIncludeOverlays() { return _includeOverlays; }
    const QVector<EntityItemID>& getIgnoreAvatars() { return _ignoreAvatars; }
    const QVector<EntityItemID>& getIncludeAvatars() { return _includeAvatars; }
    void setIgnoreEntities(const QScriptValue& ignoreEntities) { _ignoreEntities = qVectorEntityItemIDFromScriptValue(ignoreEntities); }
    void setIncludeEntities(const QScriptValue& includeEntities) { _includeEntities = qVectorEntityItemIDFromScriptValue(includeEntities); }
    void setIgnoreOverlays(const QScriptValue& ignoreOverlays) { _ignoreOverlays = qVectorOverlayIDFromScriptValue(ignoreOverlays); }
    void setIncludeOverlays(const QScriptValue& includeOverlays) { _includeOverlays = qVectorOverlayIDFromScriptValue(includeOverlays); }
    void setIgnoreAvatars(const QScriptValue& ignoreAvatars) { _ignoreAvatars = qVectorEntityItemIDFromScriptValue(ignoreAvatars); }
    void setIncludeAvatars(const QScriptValue& includeAvatars) { _includeAvatars = qVectorEntityItemIDFromScriptValue(includeAvatars); }

private:
    RayPickFilter _filter;
    float _maxDistance;
    bool _enabled;
    RayPickResult _prevResult;

    QVector<EntityItemID> _ignoreEntities;
    QVector<EntityItemID> _includeEntities;
    QVector<OverlayID> _ignoreOverlays;
    QVector<OverlayID> _includeOverlays;
    QVector<EntityItemID> _ignoreAvatars;
    QVector<EntityItemID> _includeAvatars;
};

#endif // hifi_RayPick_h
