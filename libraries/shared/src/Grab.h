//
//  Grab.h
//  libraries/avatars/src
//
//  Created by Seth Alves on 2018-9-1.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Grab_h
#define hifi_Grab_h

#include <QUuid>
#include <QByteArray>
#include "GLMHelpers.h"
#include "StreamUtils.h"

class Grab;
using GrabPointer = std::shared_ptr<Grab>;
using GrabWeakPointer = std::weak_ptr<Grab>;

class GrabLocationAccumulator {
public:
    void accumulate(glm::vec3 position, glm::quat orientation) {
        _position += position;
        _orientation = orientation; // XXX
        _count++;
    }

    glm::vec3 finalizePosition() { return _count > 0 ? _position * (1.0f / _count) : glm::vec3(0.0f); }
    glm::quat finalizeOrientation() { return _orientation; } // XXX

protected:
    glm::vec3 _position;
    glm::quat _orientation;
    int _count { 0 };
};

class Grab {
public:
    Grab() {};
    Grab(const QUuid& newOwnerID, const QUuid& newTargetID, int newParentJointIndex, const QString& newHand,
         glm::vec3 newPositionalOffset, glm::quat newRotationalOffset) :
        _ownerID(newOwnerID),
        _targetID(newTargetID),
        _parentJointIndex(newParentJointIndex),
        _hand(newHand),
        _positionalOffset(newPositionalOffset),
        _rotationalOffset(newRotationalOffset),
        _released(false) {}

    QByteArray toByteArray();
    bool fromByteArray(const QByteArray& grabData);

    Grab& operator=(const GrabPointer& other) {
        _ownerID = other->_ownerID;
        _targetID = other->_targetID;
        _parentJointIndex = other->_parentJointIndex;
        _hand = other->_hand;
        _positionalOffset = other->_positionalOffset;
        _rotationalOffset = other->_rotationalOffset;
        _actionID = other->_actionID;
        _released = other->_released;
        return *this;
    }

    QUuid getActionID() const { return _actionID; }
    void setActionID(const QUuid& actionID) { _actionID = actionID; }

    QUuid getOwnerID() const { return _ownerID; }
    void setOwnerID(QUuid ownerID) { _ownerID = ownerID; }

    QUuid getTargetID() const { return _targetID; }
    void setTargetID(QUuid targetID) { _targetID = targetID; }

    int getParentJointIndex() const { return _parentJointIndex; }
    void setParentJointIndex(int parentJointIndex) { _parentJointIndex = parentJointIndex; }

    QString getHand() const { return _hand; }
    void setHand(QString hand) { _hand = hand; }

    glm::vec3 getPositionalOffset() const { return _positionalOffset; }
    void setPositionalOffset(glm::vec3 positionalOffset) { _positionalOffset = positionalOffset; }

    glm::quat getRotationalOffset() const { return _rotationalOffset; }
    void setRotationalOffset(glm::quat rotationalOffset) { _rotationalOffset = rotationalOffset; }

    bool getReleased() const { return _released; }
    void setReleased(bool value) { _released = value; }

protected:
    QUuid _actionID; // if an action is created in bullet for this grab, this is the ID
    QUuid _ownerID; // avatar ID of grabber
    QUuid _targetID; // SpatiallyNestable ID of grabbed
    int _parentJointIndex { -1 }; // which avatar joint is being used to grab
    QString _hand; // "left" or "right"
    glm::vec3 _positionalOffset; // relative to joint
    glm::quat _rotationalOffset; // relative to joint
    bool _released { false }; // released and scheduled for deletion
};


#endif // hifi_Grab_h
