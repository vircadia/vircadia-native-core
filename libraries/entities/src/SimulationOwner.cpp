//
//  SimulationOwner.cpp
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2015.06.19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SimulationOwner.h"

#include <iostream> // included for tests
#include <assert.h>

#include <NumericalConstants.h>

const quint8 PENDING_STATE_NOTHING = 0;
const quint8 PENDING_STATE_TAKE = 1;
const quint8 PENDING_STATE_RELEASE = 2;

// static
const int SimulationOwner::NUM_BYTES_ENCODED = NUM_BYTES_RFC4122_UUID + 1;

SimulationOwner::SimulationOwner() :
        _id(),
        _expiry(0),
        _pendingBidTimestamp(0),
        _priority(0),
        _pendingBidPriority(0),
        _pendingState(PENDING_STATE_NOTHING)
{
}

SimulationOwner::SimulationOwner(const QUuid& id, quint8 priority) :
        _id(id),
        _expiry(0),
        _pendingBidTimestamp(0),
        _priority(priority),
        _pendingBidPriority(0)
{
}

QByteArray SimulationOwner::toByteArray() const {
    QByteArray data = _id.toRfc4122();
    data.append(_priority);
    return data;
}

bool SimulationOwner::fromByteArray(const QByteArray& data) {
    if (data.size() == NUM_BYTES_ENCODED) {
        QByteArray idBytes = data.left(NUM_BYTES_RFC4122_UUID);
        _id = QUuid::fromRfc4122(idBytes);
        _priority = data[NUM_BYTES_RFC4122_UUID];
        return true;
    }
    return false;
}

void SimulationOwner::clear() {
    _id = QUuid();
    _expiry = 0;
    _pendingBidTimestamp = 0;
    _priority = 0;
    _pendingBidPriority = 0;
    _pendingState = PENDING_STATE_NOTHING;
}

void SimulationOwner::setPriority(quint8 priority) {
    _priority = priority;
}

void SimulationOwner::promotePriority(quint8 priority) {
    if (priority > _priority) {
        _priority = priority;
    }
}

bool SimulationOwner::setID(const QUuid& id) {
    if (_id != id) {
        _id = id;
        updateExpiry();
        if (_id.isNull()) {
            _priority = 0;
        }
        return true;
    }
    return false;
}

bool SimulationOwner::set(const QUuid& id, quint8 priority) {
    uint8_t oldPriority = _priority;
    setPriority(priority);
    return setID(id) || oldPriority != _priority;
}

bool SimulationOwner::set(const SimulationOwner& owner) {
    uint8_t oldPriority = _priority;
    setPriority(owner._priority);
    return setID(owner._id) || oldPriority != _priority;
}

void SimulationOwner::setPendingPriority(quint8 priority, const quint64& timestamp) {
    _pendingBidPriority = priority;
    _pendingBidTimestamp = timestamp;
    _pendingState = (_pendingBidPriority == 0) ? PENDING_STATE_RELEASE : PENDING_STATE_TAKE;
}

void SimulationOwner::updateExpiry() {
    const quint64 OWNERSHIP_LOCKOUT_EXPIRY = USECS_PER_SECOND / 5;
    _expiry = usecTimestampNow() + OWNERSHIP_LOCKOUT_EXPIRY;
}

bool SimulationOwner::pendingRelease(const quint64& timestamp) {
    return _pendingBidPriority == 0 && _pendingState == PENDING_STATE_RELEASE && _pendingBidTimestamp >= timestamp;
}

bool SimulationOwner::pendingTake(const quint64& timestamp) {
    return _pendingBidPriority > 0 && _pendingState == PENDING_STATE_TAKE && _pendingBidTimestamp >= timestamp;
}

void SimulationOwner::clearCurrentOwner() {
    _id = QUuid();
    _expiry = 0;
    _priority = 0;
}

// NOTE: eventually this code will be moved into unit tests
// static debug
void SimulationOwner::test() {
    { // test default constructor
        SimulationOwner simOwner;
        if (!simOwner.isNull()) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR : SimulationOwner should be NULL" << std::endl;
        }

        if (simOwner.getPriority() != 0) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR : unexpeced SimulationOwner priority" << std::endl;
        }
    }

    { // test set constructor
        QUuid id = QUuid::createUuid();
        quint8 priority = 128;
        SimulationOwner simOwner(id, priority);
        if (simOwner.isNull()) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR : SimulationOwner should NOT be NULL" << std::endl;
        }

        if (simOwner.getID() != id) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR : SimulationOwner with unexpected id" << std::endl;
        }

        if (simOwner.getPriority() != priority) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR : unexpeced SimulationOwner priority" << std::endl;
        }

        QUuid otherID = QUuid::createUuid();
        if (simOwner.getID() == otherID) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR : SimulationOwner with unexpected id" << std::endl;
        }
    }

    { // test set()
        QUuid id = QUuid::createUuid();
        quint8 priority = 1;
        SimulationOwner simOwner;
        simOwner.set(id, priority);
        if (simOwner.isNull()) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR : SimulationOwner should NOT be NULL" << std::endl;
        }

        if (simOwner.getID() != id) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR : SimulationOwner with unexpected id" << std::endl;
        }

        if (simOwner.getPriority() != priority) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR : unexpeced SimulationOwner priority" << std::endl;
        }
    }

    { // test encode/decode
        SimulationOwner ownerA(QUuid::createUuid(), 1);
        SimulationOwner ownerB(QUuid::createUuid(), 2);

        QByteArray data = ownerA.toByteArray();
        ownerB.fromByteArray(data);

        if (ownerA.getID() != ownerB.getID()) {
            std::cout << __FILE__ << ":" << __LINE__ << " ERROR : ownerA._id should be equal to ownerB._id" << std::endl;
        }
    }
}

bool SimulationOwner::operator!=(const SimulationOwner& other) {
    return (_id != other._id || _priority != other._priority);
}

SimulationOwner& SimulationOwner::operator=(const SimulationOwner& other) {
    _priority = other._priority;
    if (_priority == 0) {
        _id = QUuid();
        _expiry = 0;
    } else {
        if (_id != other._id) {
            updateExpiry();
        }
        _id = other._id;
    }
    return *this;
}

QDebug& operator<<(QDebug& d, const SimulationOwner& simOwner) {
    d << "{ id : " << simOwner._id << ", priority : " << (int)simOwner._priority << " }";
    return d;
}

