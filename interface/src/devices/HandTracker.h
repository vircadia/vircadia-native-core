//
//  HandTracker.h
//  interface/src/devices
//
//  Created by Sam Cake on 6/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HandTracker_h
#define hifi_HandTracker_h

#include <QObject>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

/// Base class for face trackers (Faceshift, Visage, Faceplus).
class HandTracker {

public:

    struct PalmData
    {
        glm::vec3   _translation;
        glm::quat   _rotation;
        int         _lastUpdate;

        PalmData() :
            _translation(),
            _rotation(),
            _lastUpdate(0)
            {}
    };

    enum Side
    {
        SIDE_LEFT =0,
        SIDE_RIGHT,
        SIDE_NUMSIDES, // Not a valid Side, the number of sides
    };

    HandTracker();

    const glm::vec3& getPalmTranslation(Side side) const    { return _palms[side]._translation; }
    const glm::quat& getPalmRotation(Side side) const       { return _palms[side]._rotation; }
    bool isPalmActive(Side side) const                      { return _palms[side]._lastUpdate == 0; }

protected:

    PalmData _palms[SIDE_NUMSIDES];

};

#endif // hifi_HandTracker_h
