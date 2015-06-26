//
//  BulletTestUtils.h
//  hifi
//
//  Created by Seiji Emery on 6/22/15.
//
//

#ifndef hifi_BulletTestUtils_h
#define hifi_BulletTestUtils_h

#include <BulletUtil.h>

// Implements functionality in QTestExtensions.h for glm types
// There are 3 functions in here (which need to be defined for all types that use them):
//
// - getErrorDifference (const T &, const T &) -> V                   (used by QCOMPARE_WITH_ABS_ERROR)
// - operator <<  (QTextStream &, const T &) -> QTextStream &   (used by all (additional) test macros)
// - errorTest (const T &, const T &, V) -> std::function<bool()>
//      (used by QCOMPARE_WITH_RELATIVE_ERROR via QCOMPARE_WITH_LAMBDA)
//      (this is only used by btMatrix3x3 in MeshMassPropertiesTests.cpp, so it's only defined for the Mat3 type)

//
// fuzzy compare (this is a distance function, basically)
//

inline btScalar getErrorDifference(const btScalar & a, const btScalar & b) {
    return fabs(a - b);
}
inline btScalar getErrorDifference(const btVector3 & a, const btVector3 & b)
{
    return (a - b).length();
}
// Matrices are compared element-wise -- if the error value for any element > epsilon, then fail
inline btScalar getErrorDifference (const btMatrix3x3 & a, const btMatrix3x3 & b) {
    btScalar maxDiff   = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            btScalar diff = fabs(a[i][j] - b[i][j]);
            maxDiff    = qMax(diff, maxDiff);
        }
    }
    return maxDiff;
}

//
// Printing (operator <<)
//

// btMatrix3x3 stream printing (not advised to use this outside of the test macros, due to formatting)
inline QTextStream & operator << (QTextStream & stream, const btMatrix3x3 & matrix) {
    stream << "[\n\t\t";
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            stream << "    " << matrix[i][j];
        }
        stream << "\n\t\t";
    }
    stream << "]\n\t";   // hacky as hell, but this should work...
    return stream;
}
inline QTextStream & operator << (QTextStream & stream, const btVector3 & v) {
    return stream << "btVector3 { " << v.x() << ", " << v.y() << ", " << v.z() << " }";
}
// btScalar operator<< is already implemented (as it's just a typedef-ed float/double)

//
// errorTest (actual, expected, acceptableRelativeError) -> callback closure
//

// Produces a relative error test for btMatrix3x3 usable with QCOMPARE_WITH_LAMBDA
inline auto errorTest (const btMatrix3x3 & actual, const btMatrix3x3 & expected, const btScalar acceptableRelativeError)
-> std::function<bool ()>
{
    return [&actual, &expected, acceptableRelativeError] () {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (expected[i][j] != btScalar(0.0f)) {
                    auto err = (actual[i][j] - expected[i][j]) / expected[i][j];
                    if (fabsf(err) > acceptableRelativeError)
                        return false;
                } else {
                    // handle zero-case by also calling QCOMPARE_WITH_ABS_ERROR
                    // (this function implements QCOMPARE_WITH_RELATIVE_ERROR, so call both
                    //  to test matrices)
                }
            }
        }
        return true;
    };
}

#endif
