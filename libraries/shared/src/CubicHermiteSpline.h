//
//  CubicHermiteSpline.h
//
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CubicHermiteSpline_h
#define hifi_CubicHermiteSpline_h

#include "GLMHelpers.h"

class CubicHermiteSplineFunctor {
public:
    CubicHermiteSplineFunctor() : _p0(), _m0(), _p1(), _m1() {}
    CubicHermiteSplineFunctor(const glm::vec3& p0, const glm::vec3& m0, const glm::vec3& p1, const glm::vec3& m1) : _p0(p0), _m0(m0), _p1(p1), _m1(m1) {}

    CubicHermiteSplineFunctor(const CubicHermiteSplineFunctor& orig) : _p0(orig._p0), _m0(orig._m0), _p1(orig._p1), _m1(orig._m1) {}

    virtual ~CubicHermiteSplineFunctor() {}

    // evalute the hermite curve at parameter t (0..1)
    glm::vec3 operator()(float t) const {
        float t2 = t * t;
        float t3 = t2 * t;
        float w0 = 2.0f * t3 - 3.0f * t2 + 1.0f;
        float w1 = t3 - 2.0f * t2 + t;
        float w2 = -2.0f * t3 + 3.0f * t2;
        float w3 = t3 - t2;
        return w0 * _p0 + w1 * _m0 + w2 * _p1 + w3 * _m1;
    }

    // evaulate the first derivative of the hermite curve at parameter t (0..1)
    glm::vec3 d(float t) const {
        float t2 = t * t;
        float w0 = -6.0f * t + 6.0f * t2;
        float w1 = 1.0f - 4.0f * t + 3.0f * t2;
        float w2 = 6.0f * t - 6.0f * t2;
        float w3 = -2.0f * t + 3.0f * t2;
        return w0 * _p0 + w1 * _m0 + w2 * _p1 + w3 * _m1;
    }

    // evaulate the second derivative of the hermite curve at paramter t (0..1)
    glm::vec3 d2(float t) const {
        float w0 = -6.0f + 12.0f * t;
        float w1 = -4.0f + 6.0f * t;
        float w2 = 6.0f - 12.0f * t;
        float w3 = -2.0f + 6.0f * t;
        return w0 + _p0 + w1 * _m0 + w2 * _p1 + w3 * _m1;
    }

protected:
    glm::vec3 _p0;
    glm::vec3 _m0;
    glm::vec3 _p1;
    glm::vec3 _m1;
};

class CubicHermiteSplineFunctorWithArcLength : public CubicHermiteSplineFunctor {
public:
    enum Constants { NUM_SUBDIVISIONS = 30 };

    CubicHermiteSplineFunctorWithArcLength() : CubicHermiteSplineFunctor() {
        memset(_values, 0, sizeof(float) * (NUM_SUBDIVISIONS + 1));
    }
    CubicHermiteSplineFunctorWithArcLength(const glm::vec3& p0, const glm::vec3& m0, const glm::vec3& p1, const glm::vec3& m1) : CubicHermiteSplineFunctor(p0, m0, p1, m1) {
        // initialize _values with the accumulated arcLength along the spline.
        const float DELTA = 1.0f / NUM_SUBDIVISIONS;
        float alpha = 0.0f;
        float accum = 0.0f;
        _values[0] = 0.0f;
        for (int i = 1; i < NUM_SUBDIVISIONS + 1; i++) {
            accum += glm::distance(this->operator()(alpha),
                                   this->operator()(alpha + DELTA));
            alpha += DELTA;
            _values[i] = accum;
        }
    }

    CubicHermiteSplineFunctorWithArcLength(const CubicHermiteSplineFunctorWithArcLength& orig) : CubicHermiteSplineFunctor(orig) {
        memcpy(_values, orig._values, sizeof(float) * (NUM_SUBDIVISIONS + 1));
    }

    // given the spline parameter (0..1) output the arcLength of the spline up to that point.
    float arcLength(float t) const {
        float index = t * NUM_SUBDIVISIONS;
        int prevIndex = std::min(std::max(0, (int)glm::floor(index)), (int)NUM_SUBDIVISIONS);
        int nextIndex = std::min(std::max(0, (int)glm::ceil(index)), (int)NUM_SUBDIVISIONS);
        float alpha = glm::fract(index);
        return lerp(_values[prevIndex], _values[nextIndex], alpha);
    }

    // given an arcLength compute the spline parameter (0..1) that cooresponds to that arcLength.
    float arcLengthInverse(float s) const {
        // find first item in _values that is > s.
        int nextIndex;
        for (nextIndex = 0; nextIndex < NUM_SUBDIVISIONS; nextIndex++) {
            if (_values[nextIndex] > s) {
                break;
            }
        }
        int prevIndex = std::min(std::max(0, nextIndex - 1), (int)NUM_SUBDIVISIONS);
        float alpha = glm::clamp((s - _values[prevIndex]) / (_values[nextIndex] - _values[prevIndex]), 0.0f, 1.0f);
        const float DELTA = 1.0f / NUM_SUBDIVISIONS;
        return lerp(prevIndex * DELTA, nextIndex * DELTA, alpha);
    }
protected:
    float _values[NUM_SUBDIVISIONS + 1];
};

#endif // hifi_CubicHermiteSpline_h
