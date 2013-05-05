//
//  AABox.h - Axis Aligned Boxes
//  hifi
//
//  Added by Brad Hefta-Gaub on 04/11/13.
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//
//  Simple axis aligned box class.
//

#include "AABox.h"


void AABox::scale(float scale) {
    _corner = _corner*scale;
    _size = _size*scale;
    _center = _center*scale;
}
	

void AABox::setBox(const glm::vec3& corner,  const glm::vec3& size) {
	_corner = corner;
	_size = size;
	
	// In the event that the caller gave us negative sizes, fix things up to be reasonable
	if (_size.x < 0.0) {
		_size.x = -size.x;
		_corner.x -= _size.x;
	}
	if (_size.y < 0.0) {
		_size.y = -size.y;
		_corner.y -= _size.y;
	}
	if (_size.z < 0.0) {
		_size.z = -size.z;
		_corner.z -= _size.z;
	}
	_center = _corner + (_size * 0.5f);
}

glm::vec3 AABox::getVertexP(const glm::vec3 &normal) const {
	glm::vec3 res = _corner;
	if (normal.x > 0)
		res.x += _size.x;

	if (normal.y > 0)
		res.y += _size.y;

	if (normal.z > 0)
		res.z += _size.z;

	return(res);
}



glm::vec3 AABox::getVertexN(const glm::vec3 &normal) const {
	glm::vec3 res = _corner;

	if (normal.x < 0)
		res.x += _size.x;

	if (normal.y < 0)
		res.y += _size.y;

	if (normal.z < 0)
		res.z += _size.z;

	return(res);
}
