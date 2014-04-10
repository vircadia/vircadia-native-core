//
//  AngleUtil.h
//  libraries/shared/src
//
//  Created by Tobias Schwinger on 3/23/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AngleUtil_h
#define hifi_AngleUtil_h

#include <math.h>

struct Degrees {

    static float pi() { return 180.0f; }
    static float twicePi() { return 360.0f; }
    static float halfPi()  { return 90.0f; }
};

struct Radians {

    static float pi() { return 3.141592653589793f; }
    static float twicePi() { return 6.283185307179586f; }
    static float halfPi()  { return 1.5707963267948966f; }
};

struct Rotations {

    static float pi() { return 0.5f; }
    static float twicePi() { return 1.0f; }
    static float halfPi()  { return 0.25f; }
};

// 
// Converts an angle from one unit to another.
// 
template< class UnitFrom, class UnitTo >
float angleConvert(float a) {

    return a * (UnitTo::halfPi() / UnitFrom::halfPi());
}


// 
// Clamps an angle to the range of [-180; 180) degrees.
// 
template< class Unit >
float angleSignedNormal(float a) {

    // result is remainder(a, Unit::twicePi());
    float result = fmod(a, Unit::twicePi());
    if (result >= Unit::pi()) {

        result -= Unit::twicePi();

    } else if (result < -Unit::pi()) {

        result += Unit::twicePi();
    } 
    return result;
}

// 
// Clamps an angle to the range of [0; 360) degrees.
// 
template< class Unit >
float angleUnsignedNormal(float a) {

    return angleSignedNormal<Unit>(a - Unit::pi()) + Unit::pi();
}


//  
// Clamps a polar direction so that azimuth is in the range of [0; 360)
// degrees and altitude is in the range of [-90; 90] degrees.
//
// The so normalized angle still contains ambiguity due to gimbal lock:
// Both poles can be reached from any azimuthal direction.
// 
template< class Unit >
void angleHorizontalPolar(float& azimuth, float& altitude) {

    altitude = angleSignedNormal<Unit>(altitude);
    if (altitude > Unit::halfPi()) {

        altitude = Unit::pi() - altitude;
        azimuth += Unit::pi();

    } else if (altitude < -Unit::halfPi()) {

        altitude = -Unit::pi() - altitude;
        azimuth += Unit::pi();
    }
    azimuth = angleUnsignedNormal<Unit>(azimuth);
}

#endif // hifi_AngleUtil_h
