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
#include "GeometryUtil.h"


void AABox::scale(float scale) {
    _corner = _corner * scale;
    _size = _size * scale;
    _center = _center * scale;
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

// determines whether a value is within the extents
static bool isWithin(float value, float corner, float size) {
    return value >= corner && value <= corner + size;
}

bool AABox::contains(const glm::vec3& point) const {
    return isWithin(point.x, _corner.x, _size.x) &&
        isWithin(point.y, _corner.y, _size.y) &&
        isWithin(point.z, _corner.z, _size.z);
}

// determines whether a value is within the expanded extents
static bool isWithinExpanded(float value, float corner, float size, float expansion) {
    return value >= corner - expansion && value <= corner + size + expansion;
}

bool AABox::expandedContains(const glm::vec3& point, float expansion) const {
    return isWithinExpanded(point.x, _corner.x, _size.x, expansion) &&
        isWithinExpanded(point.y, _corner.y, _size.y, expansion) &&
        isWithinExpanded(point.z, _corner.z, _size.z, expansion);
}

// finds the intersection between a ray and the facing plane on one axis
static bool findIntersection(float origin, float direction, float corner, float size, float& distance) {
    if (direction > EPSILON) {
        distance = (corner - origin) / direction;
        return true;
        
    } else if (direction < -EPSILON) {
        distance = (corner + size - origin) / direction;
        return true;
    }
    return false;
}

bool AABox::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, BoxFace& face) const {
    // handle the trivial case where the box contains the origin
    if (contains(origin)) {
        distance = 0;
        return true;
    }
    // check each axis
    float axisDistance;
    if ((findIntersection(origin.x, direction.x, _corner.x, _size.x, axisDistance) && axisDistance >= 0 &&
            isWithin(origin.y + axisDistance*direction.y, _corner.y, _size.y) &&
            isWithin(origin.z + axisDistance*direction.z, _corner.z, _size.z))) {
        distance = axisDistance;
        face = direction.x > 0 ? MIN_X_FACE : MAX_X_FACE;
        return true;
    }
    if ((findIntersection(origin.y, direction.y, _corner.y, _size.y, axisDistance) && axisDistance >= 0 &&
            isWithin(origin.x + axisDistance*direction.x, _corner.x, _size.x) &&
            isWithin(origin.z + axisDistance*direction.z, _corner.z, _size.z))) {
        distance = axisDistance;
        face = direction.y > 0 ? MIN_Y_FACE : MAX_Y_FACE;
        return true;
    }
    if ((findIntersection(origin.z, direction.z, _corner.z, _size.z, axisDistance) && axisDistance >= 0 &&
            isWithin(origin.y + axisDistance*direction.y, _corner.y, _size.y) &&
            isWithin(origin.x + axisDistance*direction.x, _corner.x, _size.x))) {
        distance = axisDistance;
        face = direction.z > 0 ? MIN_Z_FACE : MAX_Z_FACE; 
        return true;
    }
    return false;
}

bool AABox::findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration) const {
    glm::vec4 center4 = glm::vec4(center, 1.0f);
    
    float minPenetrationLength = FLT_MAX;
    for (int i = 0; i < FACE_COUNT; i++) {
        glm::vec3 vector = getClosestPointOnFace(center, (BoxFace)i) - center;
        if (glm::dot(center4, getPlane((BoxFace)i)) >= 0.0f) {
            return ::findSpherePenetration(vector, radius, penetration);
        }
        float vectorLength = glm::length(vector);
        if (vectorLength < minPenetrationLength) {
            penetration = vector * ((vectorLength + radius) / -vectorLength);
            minPenetrationLength = vectorLength;
        }
    }
    
    return true;
}

glm::vec3 AABox::getClosestPointOnFace(const glm::vec3& point, BoxFace face) const {
    switch (face) {
        case MIN_X_FACE:
            return glm::clamp(point, glm::vec3(_corner.x, _corner.y, _corner.z),
                glm::vec3(_corner.x, _corner.y + _size.y, _corner.z + _size.z));
        
        case MAX_X_FACE:
            return glm::clamp(point, glm::vec3(_corner.x + _size.x, _corner.y, _corner.z),
                glm::vec3(_corner.x + _size.x, _corner.y + _size.y, _corner.z + _size.z));
        
        case MIN_Y_FACE:
            return glm::clamp(point, glm::vec3(_corner.x, _corner.y, _corner.z),
                glm::vec3(_corner.x + _size.x, _corner.y, _corner.z + _size.z));
    
        case MAX_Y_FACE:
            return glm::clamp(point, glm::vec3(_corner.x, _corner.y + _size.y, _corner.z),
                glm::vec3(_corner.x + _size.x, _corner.y + _size.y, _corner.z + _size.z));
    
        case MIN_Z_FACE:
            return glm::clamp(point, glm::vec3(_corner.x, _corner.y, _corner.z),
                glm::vec3(_corner.x + _size.x, _corner.y + _size.y, _corner.z));
    
        case MAX_Z_FACE:
            return glm::clamp(point, glm::vec3(_corner.x, _corner.y, _corner.z + _size.z),
                glm::vec3(_corner.x + _size.x, _corner.y + _size.y, _corner.z + _size.z));
    }
}

glm::vec4 AABox::getPlane(BoxFace face) const {
    switch (face) {
        case MIN_X_FACE: return glm::vec4(-1.0f, 0.0f, 0.0f, _corner.x);
        case MAX_X_FACE: return glm::vec4(1.0f, 0.0f, 0.0f, -_corner.x - _size.x);
        case MIN_Y_FACE: return glm::vec4(0.0f, -1.0f, 0.0f, _corner.y);
        case MAX_Y_FACE: return glm::vec4(0.0f, 1.0f, 0.0f, -_corner.y - _size.y);
        case MIN_Z_FACE: return glm::vec4(0.0f, 0.0f, -1.0f, _corner.z);
        case MAX_Z_FACE: return glm::vec4(0.0f, 0.0f, 1.0f, -_corner.z - _size.z);
    }
}
