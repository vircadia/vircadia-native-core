//
//  Constraint.h
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Constraint_h
#define hifi_Constraint_h

class Constraint {
public:
    Constraint() {}
    virtual ~Constraint() {}

    /// Enforce contraint by moving relevant points.
    /// \return max distance of point movement
    virtual float enforce() = 0;
};

#endif // hifi_Constraint_h
