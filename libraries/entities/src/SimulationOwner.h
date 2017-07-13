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

// Simulation observers will bid to simulate unowned active objects at the lowest possible priority
// which is VOLUNTEER.  If the server accepts a VOLUNTEER bid it will automatically bump it
// to RECRUIT priority so that other volunteers don't accidentally take over.
const quint8 VOLUNTEER_SIMULATION_PRIORITY = 0x01;
const quint8 RECRUIT_SIMULATION_PRIORITY = VOLUNTEER_SIMULATION_PRIORITY + 1;

// When poking objects with scripts an observer will bid at SCRIPT_EDIT priority.
const quint8 SCRIPT_GRAB_SIMULATION_PRIORITY = 0x80;
const quint8 SCRIPT_POKE_SIMULATION_PRIORITY = SCRIPT_GRAB_SIMULATION_PRIORITY - 1;
const quint8 AVATAR_ENTITY_SIMULATION_PRIORITY = SCRIPT_GRAB_SIMULATION_PRIORITY + 1;

// PERSONAL priority (needs a better name) is the level at which a simulation observer owns its own avatar
// which really just means: things that collide with it will be bid at a priority level one lower
const quint8 PERSONAL_SIMULATION_PRIORITY = SCRIPT_GRAB_SIMULATION_PRIORITY;


class SimulationOwner {
public:
    static const int NUM_BYTES_ENCODED;

    SimulationOwner();
    SimulationOwner(const QUuid& id, quint8 priority);

    const QUuid& getID() const { return _id; }
    const quint64& getExpiry() const { return _expiry; }
    quint8 getPriority() const { return _priority; }

    QByteArray toByteArray() const;
    bool fromByteArray(const QByteArray& data);

    void clear();

    void setPriority(quint8 priority);
    void promotePriority(quint8 priority);

    // return true if id is changed
    bool setID(const QUuid& id);
    bool set(const QUuid& id, quint8 priority);
    bool set(const SimulationOwner& owner);
    void setPendingPriority(quint8 priority, const quint64& timestamp);

    bool isNull() const { return _id.isNull(); }
    bool matchesValidID(const QUuid& id) const { return _id == id && !_id.isNull(); }

    void updateExpiry();

    bool hasExpired() const { return usecTimestampNow() > _expiry; }

    uint8_t getPendingPriority() const { return _pendingBidPriority; }
    bool pendingRelease(const quint64& timestamp); // return true if valid pending RELEASE
    bool pendingTake(const quint64& timestamp); // return true if valid pending TAKE
    void clearCurrentOwner();

    bool operator>=(quint8 priority) const { return _priority >= priority; }
    bool operator==(const SimulationOwner& other) { return (_id == other._id && _priority == other._priority); }

    bool operator!=(const SimulationOwner& other);
    SimulationOwner& operator=(const SimulationOwner& other);

    friend QDebug& operator<<(QDebug& d, const SimulationOwner& simOwner);

    // debug
    static void test();

private:
    QUuid _id; // owner
    quint64 _expiry; // time when ownership can transition at equal priority
    quint64 _pendingBidTimestamp; // time when pending bid was set
    quint8 _priority; // priority of current owner
    quint8 _pendingBidPriority; // priority at which we'd like to own it
    quint8 _pendingState; // NOTHING, TAKE, or RELEASE
};



#endif // hifi_SimulationOwner_h
