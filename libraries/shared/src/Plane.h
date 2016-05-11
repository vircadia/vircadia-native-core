//
//  Plane.h
//  libraries/shared/src/
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//  Simple plane class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Plane_h
#define hifi_Plane_h

#include <glm/glm.hpp>

class Plane {
public:
    Plane(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3) { set3Points(v1,v2,v3); }
    Plane() : _normal(0.0f), _point(0.0f), _dCoefficient(0.0f) {};
    ~Plane() {} ;

    // methods for defining the plane
    void set3Points(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3);
    void setNormalAndPoint(const glm::vec3 &normal, const glm::vec3 &point);
    void setCoefficients(float a, float b, float c, float d);

    // getters
    const glm::vec3& getNormal() const { return _normal; };
    const glm::vec3& getPoint() const { return _point; };
    float getDCoefficient() const { return _dCoefficient; };

    // utilities
    void invalidate() { _normal = glm::vec3(0.0f), _dCoefficient = 1.0e6f; } // distance() never less than 10^6
    float distance(const glm::vec3 &point) const;
    void print() const;

private:
    glm::vec3 _normal;
    glm::vec3 _point;
    float _dCoefficient;
};


#endif // hifi_Plane_h
