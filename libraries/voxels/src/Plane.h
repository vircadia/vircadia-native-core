//////////////////////////////////////////////////////////////////////
// Plane.h - inspired and modified from lighthouse3d.com
//


#ifndef _PLANE_
#define _PLANE_

#include <glm/glm.hpp>


class Plane  
{

public:

	glm::vec3 normal,point;
	float d;


	Plane(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3);
	Plane(void);
	~Plane();

	void set3Points(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3);
	void setNormalAndPoint(const glm::vec3 &normal, const glm::vec3 &point);
	void setCoefficients(float a, float b, float c, float d);
	float distance(const glm::vec3 &p);

	void print();

};


#endif