//
//  Extents.h
//  libraries/shared/src
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Moved to shared by Brad Hefta-Gaub on 9/11/14
//  Copyright 2013-2104 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Extents_h
#define hifi_Extents_h

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>

#include <QDebug>
#include "StreamUtils.h"
#include "GLMHelpers.h"

class AABox;
class Transform;

class Extents {
public:
    Extents() { }
    Extents(const glm::vec3& minimum, const glm::vec3& maximum) : minimum(minimum), maximum(maximum) {}
    Extents(const AABox& box) { reset(); add(box); }

    /// set minimum and maximum to FLT_MAX and -FLT_MAX respectively
    void reset();

    /// \param extents another intance of extents
    /// expand current limits to contain other extents
    void addExtents(const Extents& extents);

    /// \param aabox another intance of extents
    /// expand current limits to contain other aabox
    void add(const AABox& box);

    /// \param point new point to compare against existing limits
    /// compare point to current limits and expand them if necessary to contain point
    void addPoint(const glm::vec3& point);

    /// \param point
    /// \return true if point is within current limits
    bool containsPoint(const glm::vec3& point) const;

    /// \return whether or not the extents are empty
    bool isEmpty() const { return minimum == maximum; }
    bool isValid() const { return !((minimum == Vectors::MAX) && (maximum == Vectors::MIN)); }

    /// \param vec3 for delta amount to shift the extents by
    /// \return true if point is within current limits
    void shiftBy(const glm::vec3& delta) { minimum += delta; maximum += delta; }
    
    /// rotate the extents around orign by rotation
    void rotate(const glm::quat& rotation);
    
    /// scale the extents around orign by scale
    void scale(float scale) { minimum *= scale; maximum *= scale; }
    void scale(const glm::vec3& scale) { minimum *= scale; maximum *= scale; }
    
    // Transform the extents with transform
    void transform(const Transform& transform);

    glm::vec3 size() const { return maximum - minimum; }
    float largestDimension() const { return glm::compMax(size()); }

    /// \return new Extents which is original rotated around origin by rotation
    Extents getRotated(const glm::quat& rotation) const {
        Extents temp(minimum, maximum);
        temp.rotate(rotation);
        return temp;
    }

    glm::vec3 minimum{ Vectors::MAX };
    glm::vec3 maximum{ Vectors::MIN };
};

inline QDebug operator<<(QDebug debug, const Extents& extents) {
    debug << "Extents[ (" 
            << extents.minimum << " ) to ("
            << extents.maximum << ") size: ("
            << (extents.maximum - extents.minimum) << ")"
            << " ]";

    return debug;
}


#endif // hifi_Extents_h
