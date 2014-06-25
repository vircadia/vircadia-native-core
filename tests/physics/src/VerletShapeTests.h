//
//  VerletShapeTests.h
//  tests/physics/src
//
//  Created by Andrew Meadows on 2014.06.18
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VerletShapeTests_h
#define hifi_VerletShapeTests_h

namespace VerletShapeTests {
    void setSpherePosition();

    void sphereMissesSphere();
    void sphereTouchesSphere();

    void sphereMissesCapsule();
    void sphereTouchesCapsule();

    void capsuleMissesCapsule();
    void capsuleTouchesCapsule();

    void runAllTests(); 
}

#endif // hifi_VerletShapeTests_h
