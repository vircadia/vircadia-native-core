//
//  VoxelProjectedShadow.cpp - The projected shadow (on the 2D view plane) for a voxel
//  hifi
//
//  Added by Brad Hefta-Gaub on 06/11/13.
//

#include "VoxelProjectedShadow.h"
#include "GeometryUtil.h"


bool BoundingBox::contains(const BoundingBox& box) const {
    return (
                (box.corner.x >= corner.x) &&
                (box.corner.y >= corner.y) &&
                (box.corner.x + box.size.x <= corner.x + size.x) &&
                (box.corner.y + box.size.y <= corner.y + size.y)
            );
};

void VoxelProjectedShadow::setVertex(int vertex, const glm::vec2& point) { 
    _vertices[vertex] = point;
    
    // keep track of our bounding box
    if (point.x > _maxX) {
        _maxX = point.x;
    }
    if (point.y > _maxY) {
        _maxY = point.y;
    }
    if (point.x < _minX) {
        _minX = point.x;
    }
    if (point.y < _minY) {
        _minY = point.y;
    }
    
};

bool VoxelProjectedShadow::occludes(const VoxelProjectedShadow& occludee) const {
    
    // first check the bounding boxes, the occludee mush be fully within the boounding box of this shadow
    if ((occludee.getMaxX() > getMaxX()) ||
        (occludee.getMaxY() > getMaxY()) ||
        (occludee.getMinX() < getMinX()) ||
        (occludee.getMinY() < getMinY())) {
        return false;
    }
    
    // if we got this far, then check each vertex of the occludee, if all those points
    // are inside our polygon, then the tested occludee is fully occluded
    for(int i = 0; i < occludee.getVertexCount(); i++) {
        if (!pointInside(occludee.getVertex(i))) {
            return false;
        }
    }
    
    // if we got this far, then indeed the occludee is fully occluded by us
    return true;
}

bool VoxelProjectedShadow::pointInside(const glm::vec2& point) const {
    // first check the bounding boxes, the point mush be fully within the boounding box of this shadow
    if ((point.x > getMaxX()) ||
        (point.y > getMaxY()) ||
        (point.x < getMinX()) ||
        (point.y < getMinY())) {
        return false;
    }

    float e = (getMaxX() - getMinX()) / 100.0f; // some epsilon
    
    // We need to have one ray that goes from a known outside position to the point in question. We'll pick a
    // start point just outside of our min X
    glm::vec2 r1p1(getMinX() - e, point.y);
    glm::vec2 r1p2(point);

    glm::vec2 r2p1(getVertex(getVertexCount()-1)); // start with last vertex to first vertex
    glm::vec2 r2p2;
    
    // Test the ray against all sides
    int intersections = 0;
    for (int i = 0; i < getVertexCount(); i++) {
        r2p2 = getVertex(i);
        if (doLineSegmentsIntersect(r1p1, r1p2, r2p1, r2p2)) {
            intersections++;
        }
        r2p1 = r2p2; // set up for next side
    }

    // If odd number of intersections, we're inside    
    return ((intersections & 1) == 1);
}


