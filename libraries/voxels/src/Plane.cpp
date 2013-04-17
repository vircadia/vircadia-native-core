// Plane.cpp
//
//////////////////////////////////////////////////////////////////////

#include "Plane.h"
#include <stdio.h>

#include "voxels_Log.h"

using voxels::printLog;

// These are some useful utilities that vec3 is missing
float vec3_length(const glm::vec3& v) {
	return((float)sqrt(v.x*v.x + v.y*v.y + v.z*v.z));
}

void vec3_normalize(glm::vec3& v) {

	float len;

	len = vec3_length(v);
	if (len) {
		v.x /= len;;
		v.y /= len;
		v.z /= len;
	}
}

float vec3_innerProduct(const glm::vec3& v1,const glm::vec3& v2) {
    
	return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}



Plane::Plane(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3) {

	set3Points(v1,v2,v3);
}


Plane::Plane() {}

Plane::~Plane() {}


void Plane::set3Points(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3) {


	glm::vec3 aux1, aux2;

	aux1 = v1 - v2;
	aux2 = v3 - v2;

	normal = aux2 * aux1;

	vec3_normalize(normal);
	point = v2;
	d = -(vec3_innerProduct(normal,point));
}

void Plane::setNormalAndPoint(const glm::vec3 &normal, const glm::vec3 &point) {

	this->normal = normal;
	vec3_normalize(this->normal);
	d = -(vec3_innerProduct(this->normal,point));
}

void Plane::setCoefficients(float a, float b, float c, float d) {

	// set the normal vector
	normal = glm::vec3(a,b,c);
	//compute the lenght of the vector
	float l = normal.length();
	// normalize the vector
	normal = glm::vec3(a/l,b/l,c/l);
	// and divide d by th length as well
	this->d = d/l;
}

float Plane::distance(const glm::vec3 &p) {
    return (d + vec3_innerProduct(normal,p));
}

void Plane::print() {
	//printLog("Plane(");normal.print();printLog("# %f)",d);
}
