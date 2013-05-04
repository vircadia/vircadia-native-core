//
//  AABox.h - Axis Aligned Boxes
//  hifi
//
//  Added by Brad Hefta-Gaub on 04/11/13.
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//
//  Simple axis aligned box class.
//

#include "SharedUtil.h"

#include "AABox.h"


void AABox::scale(float scale) {
    _corner = _corner*scale;
    _size = _size*scale;
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

// determines whether a value is within the extents
static bool isWithin(float value, float corner, float size) {
    return value >= corner && value <= corner + size;
}

bool AABox::contains(const glm::vec3& point) const {
    return isWithin(point.x, _corner.x, _size.x) &&
        isWithin(point.y, _corner.y, _size.y) &&
        isWithin(point.z, _corner.z, _size.z);
}

// finds the intersection between the closer plane in one direction
static bool findIntersection(float origin, float direction, float corner, float size, float* t) {
    if (direction > EPSILON) {
        *t = (corner - origin) / direction;
        return true;
        
    } else if (direction < -EPSILON) {
        *t = (corner + size - origin) / direction;
        return true;
    }
    return false;
}

bool AABox::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float* t) const {
    // handle the trivial case where the box contains the origin
    if (contains(origin)) {
        *t = 0;
        return true;
    }
    // check each direction
    float nt;
    if (findIntersection(origin.x, direction.x, _corner.x, _size.x, &nt) && nt >= 0 &&
            isWithin(origin.y + nt*direction.y, _corner.y, _size.y) &&
            isWithin(origin.z + nt*direction.z, _corner.z, _size.z) ||
        findIntersection(origin.y, direction.y, _corner.y, _size.y, &nt) && nt >= 0 &&
            isWithin(origin.x + nt*direction.x, _corner.x, _size.x) &&
            isWithin(origin.z + nt*direction.z, _corner.z, _size.z) ||
        findIntersection(origin.z, direction.z, _corner.z, _size.z, &nt) && nt >= 0 &&
            isWithin(origin.y + nt*direction.y, _corner.y, _size.y) &&
            isWithin(origin.x + nt*direction.x, _corner.x, _size.x)) {
        *t = nt;
        return true;
    }
    return false;
}
