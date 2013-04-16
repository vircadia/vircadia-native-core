/* ------------------------------------------------------

 Axis Aligned Boxes - Lighthouse3D

  -----------------------------------------------------*/

#include "AABox.h"


AABox::AABox(const glm::vec3& corner,  float x, float y, float z) {
	setBox(corner,x,y,z);
}

AABox::AABox(void) {
	corner.x = 0; corner.y = 0; corner.z = 0;
	x = 1.0f;
	y = 1.0f;
	z = 1.0f;
}


AABox::~AABox() {}

	

void AABox::setBox(const glm::vec3& corner,  float x, float y, float z) {
	this->corner = corner;
	if (x < 0.0) {
		x = -x;
		this->corner.x -= x;
	}
	if (y < 0.0) {
		y = -y;
		this->corner.y -= y;
	}
	if (z < 0.0) {
		z = -z;
		this->corner.z -= z;
	}
	this->x = x;
	this->y = y;
	this->z = z;
}



glm::vec3 AABox::getVertexP(const glm::vec3 &normal) {
	glm::vec3 res = corner;
	if (normal.x > 0)
		res.x += x;

	if (normal.y > 0)
		res.y += y;

	if (normal.z > 0)
		res.z += z;

	return(res);
}



glm::vec3 AABox::getVertexN(const glm::vec3 &normal) {
	glm::vec3 res = corner;

	if (normal.x < 0)
		res.x += x;

	if (normal.y < 0)
		res.y += y;

	if (normal.z < 0)
		res.z += z;

	return(res);
}
