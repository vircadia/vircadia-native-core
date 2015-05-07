//
//  PhysicsTypedefs.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2015.04.29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PhysicsTypedefs_h
#define hifi_PhysicsTypedefs_h

#include <QSet>
#include <QVector>

class ObjectMotionState;

typedef QSet<ObjectMotionState*> SetOfMotionStates;
typedef QVector<ObjectMotionState*> VectorOfMotionStates;

#endif //hifi_PhysicsTypedefs_h
