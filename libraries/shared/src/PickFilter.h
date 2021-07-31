//
//  Created by Sam Gondelman on 12/7/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PickFilter_h
#define hifi_PickFilter_h

#include <bitset>
#include <iostream> // adebug

class PickFilter {
public:

    /*@jsdoc
     * <p>A set of flags for a pick filter. The value is constructed by using the <code>|</code> (bitwise OR) operator on the 
     * individual flag values.</p>
     * <table>
     *   <thead>
     *     <tr><th>Flag Name</th><th>Value</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td>PICK_DOMAIN_ENTITIES</td><td><code>1</code></td><td>Include domain entities when intersecting.</td></tr>
     *     <tr><td>PICK_AVATAR_ENTITIES</td><td><code>2</code></td><td>Include avatar entities when intersecting.</td></tr>
     *     <tr><td>PICK_LOCAL_ENTITIES</td><td><code>4</code></td><td>Include local entities when intersecting.</td></tr>
     *     <tr><td>PICK_AVATATRS</td><td><code>8</code></td><td>Include avatars when intersecting.</td></tr>
     *     <tr><td>PICK_HUD</td><td><code>16</code></td><td>Include the HUD surface when intersecting in HMD mode.</td></tr>
     *     <tr><td>PICK_INCLUDE_VISIBLE</td><td><code>32</code></td><td>Include visible objects when intersecting.</td></tr>
     *     <tr><td>PICK_INCLUDE_INVISIBLE</td><td><code>64</code></td><td>Include invisible objects when intersecting.</td></tr>
     *     <tr><td>PICK_INCLUDE_COLLIDABLE</td><td><code>128</code></td><td>Include collidable objects when 
     *       intersecting.</td></tr>
     *     <tr><td>PICK_INCLUDE_NONCOLLIDABLE</td><td><code>256</code></td><td>Include non-collidable objects when 
     *       intersecting.</td></tr>
     *     <tr><td>PICK_PRECISE</td><td><code>512</code></td><td>Pick against exact meshes.</td></tr>
     *     <tr><td>PICK_COARSE</td><td><code>1024</code></td><td>Pick against coarse meshes.</td></tr>
     *     <tr><td>PICK_ALL_INTERSECTIONS</td><td><code>2048</code></td><td>Return all intersections instead of just the 
     *       closest.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} FilterFlags
     */
    enum FlagBit {
        DOMAIN_ENTITIES = 0,
        AVATAR_ENTITIES,
        LOCAL_ENTITIES,
        AVATARS,
        HUD,

        VISIBLE,
        INVISIBLE,

        COLLIDABLE,
        NONCOLLIDABLE,

        PRECISE,
        COARSE,

        // NOT YET IMPLEMENTED
        PICK_ALL_INTERSECTIONS, // if not set, returns closest intersection, otherwise, returns list of all intersections

        PICK_BYPASS_IGNORE, // for debug purposes

        NUM_FLAGS, // Not a valid flag
    };
    typedef std::bitset<NUM_FLAGS> Flags;

    // The key is the Flags
    Flags _flags;

    PickFilter() {}
    PickFilter(const Flags& flags) : _flags(flags) {}

    bool operator==(const PickFilter& rhs) const { return _flags == rhs._flags; }
    bool operator!=(const PickFilter& rhs) const { return _flags != rhs._flags; }

    void setFlag(FlagBit flag, bool value) { _flags[flag] = value; }

    // There are different groups of related flags.  If none of the flags in a group are set, the search filter includes them all.
    bool doesPickDomainEntities() const { return _flags[DOMAIN_ENTITIES] || !(_flags[AVATAR_ENTITIES] || _flags[LOCAL_ENTITIES] || _flags[AVATARS] || _flags[HUD]); }
    bool doesPickAvatarEntities() const { return _flags[AVATAR_ENTITIES] || !(_flags[DOMAIN_ENTITIES] || _flags[LOCAL_ENTITIES] || _flags[AVATARS] || _flags[HUD]); }
    bool doesPickLocalEntities() const { return _flags[LOCAL_ENTITIES] || !(_flags[DOMAIN_ENTITIES] || _flags[AVATAR_ENTITIES] || _flags[AVATARS] || _flags[HUD]); }
    bool doesPickAvatars() const { return _flags[AVATARS] || !(_flags[DOMAIN_ENTITIES] || _flags[AVATAR_ENTITIES] || _flags[LOCAL_ENTITIES] || _flags[HUD]); }
    bool doesPickHUD() const { return _flags[HUD] || !(_flags[DOMAIN_ENTITIES] || _flags[AVATAR_ENTITIES] || _flags[LOCAL_ENTITIES] || _flags[AVATARS]); }

    bool doesPickVisible() const { return _flags[VISIBLE] || !_flags[INVISIBLE]; }
    bool doesPickInvisible() const { return _flags[INVISIBLE] || !_flags[VISIBLE]; }

    bool doesPickCollidable() const { return _flags[COLLIDABLE] || !_flags[NONCOLLIDABLE]; }
    bool doesPickNonCollidable() const { return _flags[NONCOLLIDABLE] || !_flags[COLLIDABLE]; }

    bool isPrecise() const { return _flags[PRECISE] || !_flags[COARSE]; }
    bool isCoarse() const { return _flags[COARSE]; }

    bool doesWantAllIntersections() const { return _flags[PICK_ALL_INTERSECTIONS]; }

    bool bypassIgnore() const { return _flags[PICK_BYPASS_IGNORE]; }

    // Helpers for RayPickManager
    Flags getEntityFlags() const {
        unsigned int toReturn = 0;
        for (int i = DOMAIN_ENTITIES; i <= LOCAL_ENTITIES; i++) {
            if (_flags[i]) {
                toReturn |= getBitMask(FlagBit(i));
            }
        }
        for (int i = HUD + 1; i < NUM_FLAGS; i++) {
            if (_flags[i]) {
                toReturn |= getBitMask(FlagBit(i));
            }
        }
        return Flags(toReturn);
    }
    Flags getAvatarFlags() const { return Flags(getBitMask(AVATARS)); }
    Flags getHUDFlags() const { return Flags(getBitMask(HUD)); }

    static constexpr unsigned int getBitMask(FlagBit bit) { return 1 << bit; }
};

#endif // hifi_PickFilter_h

