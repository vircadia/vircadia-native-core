//
//  Plane.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//
//  Simple plane class.
//

#include "Plane.h"
#include <stdio.h>

#include "voxels_Log.h"

using voxels_lib::printLog;

// These are some useful utilities that vec3 is missing
float length(const glm::vec3& v) {
    return((float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z));
}

void normalize(glm::vec3& v) {
    float len;
    len = length(v);
    if (len) {
        v.x /= len;;
        v.y /= len;
        v.z /= len;
    }
}

// cross product
glm::vec3 crossProduct(const glm::vec3& lhs, const glm::vec3& rhs) {
    glm::vec3 result;
    result.x = lhs.y * rhs.z - lhs.z * rhs.y;
    result.y = lhs.z * rhs.x - lhs.x * rhs.z;
    result.z = lhs.x * rhs.y - lhs.y * rhs.x;
    return result;
}


float innerProduct(const glm::vec3& v1,const glm::vec3& v2) {
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

void printVec3(const char* name, const glm::vec3& v) {
    printf("%s x=%f y=%f z=%f\n", name, v.x, v.y, v.z);
}

void Plane::set3Points(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3) {
    glm::vec3 linev1v2, linev1v3;

    linev1v2 = v2 - v1;
    linev1v3 = v3 - v1;

    // this will be perpendicular to both lines
    _normal = crossProduct(linev1v2,linev1v3);
    normalize(_normal);

    // this is a point on the plane
    _point = v2;

    // the D coefficient from the form Ax+By+Cz=D
    _dCoefficient = -(innerProduct(_normal,_point));
}

void Plane::setNormalAndPoint(const glm::vec3 &normal, const glm::vec3 &point) {
    _point = point;
    _normal = normal;
    normalize(_normal);

    // the D coefficient from the form Ax+By+Cz=D
    _dCoefficient = -(innerProduct(_normal,_point));
}

void Plane::setCoefficients(float a, float b, float c, float d) {
    // set the normal vector
    _normal = glm::vec3(a,b,c);

    //compute the lenght of the vector
    float l = length(_normal);

    // normalize the vector
    _normal = glm::vec3(a/l,b/l,c/l);

    // and divide d by th length as well
    _dCoefficient = d/l;
}

float Plane::distance(const glm::vec3 &point) const {
    return (_dCoefficient + innerProduct(_normal,point));
}

void Plane::print() const {
    printf("Plane - point (x=%f y=%f z=%f) normal (x=%f y=%f z=%f) d=%f\n",
        _point.x, _point.y, _point.z, _normal.x, _normal.y, _normal.z, _dCoefficient);
}
