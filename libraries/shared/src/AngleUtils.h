//
// AngleUtils.h
// hifi
//
// Created by Tobias Schwinger on 3/23/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__AngleUtils__
#define __hifi__AngleUtils__

#include <math.h>

struct Degrees {

    static float pi() { return 180.0f; }
    static float twicePi() { return 360.0f; }
    static float halfPi()  { return 90.0f; }
};

struct Radians {

    static float pi() { return 3.141592653589793f; }
    static float twicePi() { return 6.283185307179586f; }
    static float halfPi()  { return 1.5707963267948966; }
};

struct Rotations {

    static float pi() { return 0.5f; }
    static float twicePi() { return 1.0f; }
    static float halfPi()  { return 0.25f; }
};

/**
 * Converts an angle from one unit to another.
 */
template< class UnitFrom, class UnitTo >
float angleConvert(float a) {

    return a * (UnitTo::halfPi() / UnitFrom::halfPi());
}


/**
 * Clamps an angle to the range of [-180; 180) degrees.
 */
template< class Unit >
float angleSignedNormal(float a) {

    float result = remainder(a, Unit::twicePi());
    if (result == Unit::pi())
        result = -Unit::pi();
    return result;
}

/**
 * Clamps an angle to the range of [0; 360) degrees.
 */
template< class Unit >
float angleUnsignedNormal(float a) {

    return angleSignedNormal<Unit>(a - Unit::pi()) + Unit::pi();
}


/** 
 * Clamps a polar direction so that azimuth is in the range of [0; 360)
 * degrees and altitude is in the range of [-90; 90] degrees.
 *
 * The so normalized angle still contains ambiguity due to gimbal lock:
 * Both poles can be reached from any azimuthal direction.
 */
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

#endif

