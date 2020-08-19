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
const float SQUARE_ROOT_OF_2 = 1.414214f;
const float SQUARE_ROOT_OF_3 = 1.732051f;
const float METERS_PER_DECIMETER  = 0.1f;
const float METERS_PER_CENTIMETER = 0.01f;
const float METERS_PER_MILLIMETER = 0.001f;
const float MILLIMETERS_PER_METER = 1000.0f;
const quint64 NSECS_PER_USEC = 1000;
const quint64 NSECS_PER_MSEC = 1000000;
const quint64 NSECS_PER_SECOND = 1e9;
const quint64 USECS_PER_MSEC = 1000;
const quint64 MSECS_PER_SECOND = 1000;
const quint64 USECS_PER_SECOND = USECS_PER_MSEC * MSECS_PER_SECOND;
const quint64 SECS_PER_MINUTE = 60;
const quint64 MINS_PER_HOUR = 60;
const quint64 SECS_PER_HOUR = SECS_PER_MINUTE * MINS_PER_HOUR;

const int BITS_IN_BYTE = 8;
const int BYTES_PER_KILOBYTE = 1000;
const int BYTES_PER_KILOBIT = BYTES_PER_KILOBYTE / BITS_IN_BYTE;
const int KILO_PER_MEGA = 1000;

const float SQRT_THREE = 1.73205080f;

#define KB_TO_BYTES_SHIFT 10
#define MB_TO_BYTES_SHIFT 20
#define GB_TO_BYTES_SHIFT 30

#define GB_TO_BYTES(X) ((size_t)(X) << GB_TO_BYTES_SHIFT)
#define MB_TO_BYTES(X) ((size_t)(X) << MB_TO_BYTES_SHIFT)
#define KB_TO_BYTES(X) ((size_t)(X) << KB_TO_BYTES_SHIFT)

#define BYTES_TO_GB(X) (X >> GB_TO_BYTES_SHIFT)
#define BYTES_TO_MB(X) (X >> MB_TO_BYTES_SHIFT)
#define BYTES_TO_KB(X) (X >> KB_TO_BYTES_SHIFT)

#endif // hifi_NumericalConstants_h
