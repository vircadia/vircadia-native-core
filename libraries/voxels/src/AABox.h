/* ------------------------------------------------------

 Axis Aligned Boxes - Lighthouse3D

  -----------------------------------------------------*/


#ifndef _AABOX_
#define _AABOX_

#include <glm/glm.hpp>

class AABox 
{

public:

	glm::vec3 corner;
	float x,y,z;

	AABox(const glm::vec3 &corner, float x, float y, float z);
	AABox(void);
	~AABox();

	void setBox(const glm::vec3& corner, float x, float y, float z);

	// for use in frustum computations
	glm::vec3 getVertexP(const glm::vec3& normal);
	glm::vec3 getVertexN(const glm::vec3& normal);
};


#endif