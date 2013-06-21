//
//  VoxelProjectedPolygon.cpp - The projected shadow (on the 2D view plane) for a voxel
//  hifi
//
//  Added by Brad Hefta-Gaub on 06/11/13.
//

#include <algorithm>
#include "VoxelProjectedPolygon.h"
#include "GeometryUtil.h"
#include "Log.h"
#include "SharedUtil.h"


BoundingBox BoundingBox::topHalf() const {
    float halfY = size.y/2.0f;
    BoundingBox result(glm::vec2(corner.x,corner.y + halfY), glm::vec2(size.x, halfY));
    return result;
}

BoundingBox BoundingBox::bottomHalf() const {
    float halfY = size.y/2.0f;
    BoundingBox result(corner, glm::vec2(size.x, halfY));
    return result;
}

BoundingBox BoundingBox::leftHalf() const {
    float halfX = size.x/2.0f;
    BoundingBox result(corner, glm::vec2(halfX, size.y));
    return result;
}

BoundingBox BoundingBox::rightHalf() const {
    float halfX = size.x/2.0f;
    BoundingBox result(glm::vec2(corner.x + halfX , corner.y), glm::vec2(halfX, size.y));
    return result;
}

bool BoundingBox::contains(const BoundingBox& box) const {
    return ( _set &&
                (box.corner.x >= corner.x) &&
                (box.corner.y >= corner.y) &&
                (box.corner.x + box.size.x <= corner.x + size.x) &&
                (box.corner.y + box.size.y <= corner.y + size.y)
            );
};

void BoundingBox::explandToInclude(const BoundingBox& box) {
    if (!_set) {
        corner = box.corner;
        size = box.size;
        _set = true;
    } else {
        float minX = std::min(box.corner.x, corner.x);
        float minY = std::min(box.corner.y, corner.y);
        float maxX = std::max(box.corner.x + box.size.x, corner.x + size.x);
        float maxY = std::max(box.corner.y + box.size.y, corner.y + size.y);
        corner.x = minX;
        corner.y = minY;
        size.x = maxX - minX;
        size.y = maxY - minY;
    }
}


void BoundingBox::printDebugDetails(const char* label) const {
    if (label) {
        printLog(label);
    } else {
        printLog("BoundingBox");
    }
    printLog("\n    _set=%s\n    corner=%f,%f size=%f,%f\n    bounds=[(%f,%f) to (%f,%f)]\n", 
        debug::valueOf(_set), corner.x, corner.y, size.x, size.y, corner.x, corner.y, corner.x+size.x, corner.y+size.y);
}


void VoxelProjectedPolygon::setVertex(int vertex, const glm::vec2& point) { 
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

bool VoxelProjectedPolygon::occludes(const VoxelProjectedPolygon& occludee, bool checkAllInView) const {
    
    // if we are completely out of view, then we definitely don't occlude!
    // if the occludee is completely out of view, then we also don't occlude it
    //
    // this is true, but unfortunately, we're not quite handling projects in the
    // case when SOME points are in view and others are not. So, we will not consider
    // occlusion for any shadows that are partially in view.
    if (checkAllInView && (!getAllInView() || !occludee.getAllInView())) {
        return false;
    }

    // first check the bounding boxes, the occludee must be fully within the boounding box of this shadow
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

bool VoxelProjectedPolygon::pointInside(const glm::vec2& point) const {
    // first check the bounding boxes, the point must be fully within the boounding box of this shadow
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

void VoxelProjectedPolygon::printDebugDetails() const {
    printf("VoxelProjectedPolygon...");
    printf("    minX=%f maxX=%f minY=%f maxY=%f\n", getMinX(), getMaxX(), getMinY(), getMaxY());
    printf("    vertex count=%d distance=%f\n", getVertexCount(), getDistance());
    for (int i = 0; i < getVertexCount(); i++) {
        glm::vec2 point = getVertex(i);
        printf("    vertex[%d] = %f, %f \n", i, point.x, point.y);
    }
}


