//
//  NumericalConstants.h 
//  libraries/shared/src
//
//  Created by Stephen Birarda on 05/01/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NumericalConstants_h
#define hifi_NumericalConstants_h

#pragma once

#include <cmath>

#include <QtGlobal>

const float PI = 3.14159265358979f;
const float TWO_PI = 2.0f * PI;
const float PI_OVER_TWO = 0.5f * PI;
const float RADIANS_PER_DEGREE = PI / 180.0f;
const float DEGREES_PER_RADIAN = 180.0f / PI;
const float ARCMINUTES_PER_DEGREE = 60.0f;
const float ARCSECONDS_PER_ARCMINUTE = 60.0f;
const float ARCSECONDS_PER_DEGREE = ARCMINUTES_PER_DEGREE * ARCSECONDS_PER_ARCMINUTE;

const float EPSILON = 0.000001f;    //smallish positive number - used as margin of error for some computations
const float SQUARE_ROOT_OF_2 = (float)sqrt(2.0f);
const float SQUARE_ROOT_OF_3 = (float)sqrt(3.0f);
const float METERS_PER_DECIMETER  = 0.1f;
const float METERS_PER_CENTIMETER = 0.01f;
const float METERS_PER_MILLIMETER = 0.001f;
const float MILLIMETERS_PER_METER = 1000.0f;
const quint64 NSECS_PER_USEC = 1000;
const quint64 USECS_PER_MSEC = 1000;
const quint64 MSECS_PER_SECOND = 1000;
const quint64 USECS_PER_SECOND = USECS_PER_MSEC * MSECS_PER_SECOND;

const int BITS_IN_BYTE = 8;
const int BYTES_PER_KILOBYTE = 1000;
const int BYTES_PER_KILOBIT = BYTES_PER_KILOBYTE / BITS_IN_BYTE;
const int KILO_PER_MEGA = 1000;

#endif // hifi_NumericalConstants_h
