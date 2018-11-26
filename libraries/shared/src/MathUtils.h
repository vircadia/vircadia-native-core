//
//  MathUtils.h
//  libraries/shared/src
//
//  Created by Olivier Prat on 9/21/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MathUtils_h
#define hifi_MathUtils_h

template <class T>
T divideRoundUp(const T& numerator, int divisor) {
    return (numerator + divisor - T(1)) / divisor;
}

#endif // hifi_MathUtils_h

