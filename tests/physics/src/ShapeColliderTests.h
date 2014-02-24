//
//  ShapeColliderTests.h
//  physics-tests
//
//  Created by Andrew Meadows on 2014.02.21
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __tests__ShapeColliderTests__
#define __tests__ShapeColliderTests__

namespace ShapeColliderTests {

    void sphereMissesSphere();
    void sphereTouchesSphere();

    void sphereMissesCapsule();
    void sphereTouchesCapsule();

    void capsuleMissesCapsule();
    void capsuleTouchesCapsule();

    void runAllTests(); 
}

#endif // __tests__ShapeColliderTests__
