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

bool AABox::expandedIntersectsSegment(const glm::vec3& start, const glm::vec3& end, float expansion) const {
    // handle the trivial cases where the expanded box contains the start or end
    if (expandedContains(start, expansion) || expandedContains(end, expansion)) {
        return true;
    }
    // check each axis
    glm::vec3 expandedCorner = _corner - glm::vec3(expansion, expansion, expansion);
    glm::vec3 expandedSize = _size + glm::vec3(expansion, expansion, expansion) * 2.0f;
    glm::vec3 direction = end - start;
    float axisDistance;
    return (findIntersection(start.x, direction.x, expandedCorner.x, expandedSize.x, axisDistance) &&
                axisDistance >= 0.0f && axisDistance <= 1.0f &&
                isWithin(start.y + axisDistance*direction.y, expandedCorner.y, expandedSize.y) &&
                isWithin(start.z + axisDistance*direction.z, expandedCorner.z, expandedSize.z)) ||
           (findIntersection(start.y, direction.y, expandedCorner.y, expandedSize.y, axisDistance) &&
                axisDistance >= 0.0f && axisDistance <= 1.0f &&
                isWithin(start.x + axisDistance*direction.x, expandedCorner.x, expandedSize.x) &&
                isWithin(start.z + axisDistance*direction.z, expandedCorner.z, expandedSize.z)) ||
           (findIntersection(start.z, direction.z, expandedCorner.z, expandedSize.z, axisDistance) &&
                axisDistance >= 0.0f && axisDistance <= 1.0f &&
                isWithin(start.y + axisDistance*direction.y, expandedCorner.y, expandedSize.y) &&
                isWithin(start.x + axisDistance*direction.x, expandedCorner.x, expandedSize.x));
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
        glm::vec4 facePlane = getPlane((BoxFace)i);
        glm::vec3 vector = getClosestPointOnFace(center, (BoxFace)i) - center;
        if (glm::dot(center4, getPlane((BoxFace)i)) >= 0.0f) {
            // outside this face, so use vector to closest point to determine penetration
            return ::findSpherePenetration(vector, glm::vec3(-facePlane), radius, penetration);
        }
        float vectorLength = glm::length(vector);
        if (vectorLength < minPenetrationLength) {
            // remember the smallest penetration vector; if we're inside all faces, we'll use that
            penetration = (vectorLength < EPSILON) ? glm::vec3(-facePlane) * radius :
                vector * ((vectorLength + radius) / -vectorLength);
            minPenetrationLength = vectorLength;
        }
    }
    
    return true;
}

bool AABox::findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration) const {
    glm::vec4 start4 = glm::vec4(start, 1.0f);
    glm::vec4 end4 = glm::vec4(end, 1.0f);
    glm::vec4 startToEnd = glm::vec4(end - start, 0.0f);
    
    float minPenetrationLength = FLT_MAX;
    for (int i = 0; i < FACE_COUNT; i++) {
        // find the vector from the segment to the closest point on the face (starting from deeper end)
        glm::vec4 facePlane = getPlane((BoxFace)i);
        glm::vec3 closest = (glm::dot(start4, facePlane) <= glm::dot(end4, facePlane)) ?
            getClosestPointOnFace(start4, startToEnd, (BoxFace)i) : getClosestPointOnFace(end4, -startToEnd, (BoxFace)i);
        glm::vec3 vector = -computeVectorFromPointToSegment(closest, start, end);
        if (glm::dot(vector, glm::vec3(facePlane)) < 0.0f) {
            // outside this face, so use vector to closest point to determine penetration
            return ::findSpherePenetration(vector, glm::vec3(-facePlane), radius, penetration);
        }
        float vectorLength = glm::length(vector);
        if (vectorLength < minPenetrationLength) {
            // remember the smallest penetration vector; if we're inside all faces, we'll use that
            penetration = (vectorLength < EPSILON) ? glm::vec3(-facePlane) * radius :
                vector * ((vectorLength + radius) / -vectorLength);
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

glm::vec3 AABox::getClosestPointOnFace(const glm::vec4& origin, const glm::vec4& direction, BoxFace face) const {
    // check against the four planes that border the face
    BoxFace oppositeFace = getOppositeFace(face);
    bool anyOutside = false;
    for (int i = 0; i < FACE_COUNT; i++) {
        if (i == face || i == oppositeFace) {
            continue;
        }
        glm::vec4 iPlane = getPlane((BoxFace)i);
        float originDistance = glm::dot(origin, iPlane);
        if (originDistance < 0.0f) {
            continue; // inside the border
        }
        anyOutside = true;
        float divisor = glm::dot(direction, iPlane);
        if (fabs(divisor) < EPSILON) {
            continue; // segment is parallel to plane
        }
        // find intersection and see if it lies within face bounds
        float directionalDistance = -originDistance / divisor;
        glm::vec4 intersection = origin + direction * directionalDistance;
        BoxFace iOppositeFace = getOppositeFace((BoxFace)i);
        for (int j = 0; j < FACE_COUNT; j++) {
            if (j == face || j == oppositeFace || j == i || j == iOppositeFace) {
                continue;
            }
            if (glm::dot(intersection, getPlane((BoxFace)j)) > 0.0f) {
                goto outerContinue; // intersection is out of bounds
            }
        }
        return getClosestPointOnFace(glm::vec3(intersection), face);
        
        outerContinue: ;
    }
    
    // if we were outside any of the sides, we must check against the diagonals
    if (anyOutside) {
        int faceAxis = face / 2;
        int secondAxis = (faceAxis + 1) % 3;
        int thirdAxis = (faceAxis + 2) % 3;
        
        glm::vec4 secondAxisMinPlane = getPlane((BoxFace)(secondAxis * 2));
        glm::vec4 secondAxisMaxPlane = getPlane((BoxFace)(secondAxis * 2 + 1));
        glm::vec4 thirdAxisMaxPlane = getPlane((BoxFace)(thirdAxis * 2 + 1));
        
        glm::vec4 offset = glm::vec4(0.0f, 0.0f, 0.0f,
            glm::dot(glm::vec3(secondAxisMaxPlane + thirdAxisMaxPlane), _size) * 0.5f);
        glm::vec4 diagonals[] = { secondAxisMinPlane + thirdAxisMaxPlane + offset,
            secondAxisMaxPlane + thirdAxisMaxPlane + offset };
        
        for (int i = 0; i < sizeof(diagonals) / sizeof(diagonals[0]); i++) {
            float divisor = glm::dot(direction, diagonals[i]);
            if (fabs(divisor) < EPSILON) {
                continue; // segment is parallel to diagonal plane
            }
            float directionalDistance = -glm::dot(origin, diagonals[i]) / divisor;
            return getClosestPointOnFace(glm::vec3(origin + direction * directionalDistance), face);
        }
    }
    
    // last resort or all inside: clamp origin to face
    return getClosestPointOnFace(glm::vec3(origin), face);
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

BoxFace AABox::getOppositeFace(BoxFace face) {
    switch (face) {
        case MIN_X_FACE: return MAX_X_FACE;
        case MAX_X_FACE: return MIN_X_FACE;
        case MIN_Y_FACE: return MAX_Y_FACE;
        case MAX_Y_FACE: return MIN_Y_FACE;
        case MIN_Z_FACE: return MAX_Z_FACE;
        case MAX_Z_FACE: return MIN_Z_FACE;
    }
}
