//
//  Grab.cpp
//  libraries/avatars/src
//
//  Created by Seth Alves on 2018-9-1.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Grab.h"

QByteArray Grab::toByteArray() {
    QByteArray ba;
    QDataStream dataStream(&ba, QIODevice::WriteOnly);
    const int dataEncodingVersion = 1;
    dataStream << dataEncodingVersion << _ownerID << _targetID << _parentJointIndex
               << _hand << _positionalOffset << _rotationalOffset;
    return ba;
}

bool Grab::fromByteArray(const QByteArray& grabData) {
    QDataStream dataStream(grabData);

    int dataEncodingVersion;
    QUuid newOwnerID { QUuid() };
    QUuid newTargetID { QUuid() };
    int newParentJointIndex { -1 };
    QString newHand { "none" };
    glm::vec3 newPositionalOffset { glm::vec3(0.0f) };
    glm::quat newRotationalOffset { glm::quat() };

    dataStream >> dataEncodingVersion;
    assert(dataEncodingVersion == 1);
    dataStream >> newOwnerID;
    dataStream >> newTargetID;
    dataStream >> newParentJointIndex;
    dataStream >> newHand;
    dataStream >> newPositionalOffset;
    dataStream >> newRotationalOffset;

    bool somethingChanged { false };
    if (_ownerID != newOwnerID) {
        _ownerID = newOwnerID;
        somethingChanged = true;
    }
    if (_targetID != newTargetID) {
        _targetID = newTargetID;
        somethingChanged = true;
    }
    if (_parentJointIndex != newParentJointIndex) {
        _parentJointIndex = newParentJointIndex;
        somethingChanged = true;
    }
    if (_hand != newHand) {
        _hand = newHand;
        somethingChanged = true;
    }
    if (_positionalOffset != newPositionalOffset) {
        _positionalOffset = newPositionalOffset;
        somethingChanged = true;
    }
    if (_rotationalOffset != newRotationalOffset) {
        _rotationalOffset = newRotationalOffset;
        somethingChanged = true;
    }

    return somethingChanged;
}
