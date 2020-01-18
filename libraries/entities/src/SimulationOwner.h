//
//  SimulationOwner.h
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2015.06.19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SimulationOwner_h
#define hifi_SimulationOwner_h

#include <QtCore/QDebug>
#include <QtCore/QByteArray>

#include <SharedUtil.h>
#include <UUID.h>

// HighFidelity uses a distributed physics simulation where multiple "participants" simulate portions
// of the same domain.  Even when portions overlap only one participant is allowed to be the current
// authority for any particular object's physical simulation.  The authoritative participant is called the
// "simulation owner" and its duty is to send "state synchronization" (transform/velocity) updates for an
// entity to the entity-server.  The entity-server relays updates to other participants who apply them to
// their own simulation.
//
// Participants acquire ownership by sending a "bid" to the entity-server.  The bid is a properties update:
// {   "simulationOwner": { "ownerID" : sessionID, "priority" : priority },
//     transform/velocity properties
// }
//
// The entity-server is the authority as to who owns what and may reject a bid.  The rules for handling a
// bid are as follows:
//
//     (1) A bid may be refused for special ownership restrictions, but otherwise...
//
//     (2) A bid at higher priority is accepted
//
//     (3) A bid at equal priority is accepted unless it was received shortly after (within 200msec) of the
//         last ownership change.  This to avoid rapid ownership transitions should multiple participants
//         bid simultaneously.
//
//     (4) The current owner is the only participant allowed to clear their ownership or adjust priority.
//
//     (5) If an owner does not update the transform or velocities of an owned entity within some period
//         (5 seconds) then ownership is cleared and the entity's velocities are zeroed.  This to handle
//         the case when an owner drops off the network.
//
// The priority of a participant's bid depends on how "interested" it is in the entity's motion.  The rules
// for bidding are as follows:
//
//     (6) A participant (almost) never assumes its bid is accepted by the entity-server.  It packs the
//         simulation owner properties as if they really did change but doesn't actually modify them
//         locally.  Instead it waits to hear back from the entity-server for bid acceptance.  If the entity
//         remains unowned the participant will resend the bid (assuming the bid pakcet was lost). The
//         exception is when the participant creates a moving entity: it assumes it starts off owning any
//         moving entities it creates.
//
//     (7) When an entity becomes active in the physics simulation but is not owned the participant will
//         start a timer and if it is still unowned after expiry (0.5 seconds) the participant will
//         bid at priority = VOLUNTEER (=2).  The entity-server never grants ownership at VOLUNTEER
//         priority: when a VOLUNTEER bid is accepted the entity-server always promotes the priority to
//         RECRUIT (=VOLUNTEER + 1); this to avoid a race-condition which might rapidly transition ownership
//         when multiple participants (with variable ping-times to the server) bid simultaneously for a
//         recently activated entity.
//
//     (8) When a participant's script changes an entity's transform/velocity the participant will bid at
//         priority = POKE (=127)
//
//     (9) When an entity collides against MyAvatar the participant will bid at priority = POKE.
//
//    (10) When a participant grabs an entity it will bid at priority = GRAB (=128).  This to allow UserA
//         to whack UserB with a "sword" without losing ownership, since UserB will bid at POKE.  If UserB
//         wants to contest for ownership they must also GRAB it.
//
//    (11) When EntityA, locally owned at priority = N, collides with an unowned EntityB the owner will
//         also bid for EntityB at priority = N-1 (or VOLUNTEER, whichever is larger).
//
//    (12) When an entity comes to rest and is deactivated in the physics simulation the owner will send
//         an update to: clear their ownerhsip, set priority to zero, and set the entity's velocities to
//         zero.  As per a normal bid, the owner does NOT assume its ownership has been cleared until
//         it hears back from the entity-server.  Thus, if the packet is lost the owner will re-send after
//         expiry.
//
//    (13) When an entity is still active but the owner no longer wants to own it, the owner will drop its
//         priority to YIELD (=1, below VOLUNTEER) thereby signalling to other participants to bid for it.
//
//    (14) When an entity's ownership priority drops to YIELD (=1, below VOLUNTEER) other participants may
//         bid for it immediately at VOLUNTEER.
//
/* These declarations temporarily moved to SimulationFlags.h while we unravel some spaghetti dependencies.
 * The intent is to move them back here once the dust settles.
const uint8_t YIELD_SIMULATION_PRIORITY = 1;
const uint8_t VOLUNTEER_SIMULATION_PRIORITY = YIELD_SIMULATION_PRIORITY + 1;
const uint8_t RECRUIT_SIMULATION_PRIORITY = VOLUNTEER_SIMULATION_PRIORITY + 1;

// When poking objects with scripts an observer will bid at SCRIPT_EDIT priority.
const uint8_t SCRIPT_GRAB_SIMULATION_PRIORITY = 128;
const uint8_t SCRIPT_POKE_SIMULATION_PRIORITY = SCRIPT_GRAB_SIMULATION_PRIORITY - 1;

// PERSONAL priority (needs a better name) is the level at which a simulation observer owns its own avatar
// which really just means: things that collide with it will be bid at a priority level one lower
const uint8_t PERSONAL_SIMULATION_PRIORITY = SCRIPT_GRAB_SIMULATION_PRIORITY;
const uint8_t AVATAR_ENTITY_SIMULATION_PRIORITY = PERSONAL_SIMULATION_PRIORITY;
*/


class SimulationOwner {
public:
    static const int NUM_BYTES_ENCODED;

    SimulationOwner();
    SimulationOwner(const QUuid& id, uint8_t priority);
    SimulationOwner(const SimulationOwner &) = default;

    const QUuid& getID() const { return _id; }
    const uint64_t& getExpiry() const { return _expiry; }
    uint8_t getPriority() const { return _priority; }

    QByteArray toByteArray() const;
    bool fromByteArray(const QByteArray& data);

    void clear();

    void setPriority(uint8_t priority);

    // return true if id is changed
    bool setID(const QUuid& id);
    bool set(const QUuid& id, uint8_t priority);
    bool set(const SimulationOwner& owner);

    bool isNull() const { return _id.isNull(); }
    bool matchesValidID(const QUuid& id) const { return _id == id && !_id.isNull(); }

    void updateExpiry();
    bool hasExpired() const { return usecTimestampNow() > _expiry; }

    void clearCurrentOwner();

    bool operator>=(uint8_t priority) const { return _priority >= priority; }
    bool operator==(const SimulationOwner& other) { return (_id == other._id && _priority == other._priority); }

    bool operator!=(const SimulationOwner& other);
    SimulationOwner& operator=(const SimulationOwner& other);

    friend QDebug& operator<<(QDebug& d, const SimulationOwner& simOwner);

    // debug
    static void test();

private:
    QUuid _id; // owner
    uint64_t _expiry; // time when ownership can transition at equal priority
    uint8_t _priority; // priority of current owner
};



#endif // hifi_SimulationOwner_h
