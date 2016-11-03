//
//  CubeProjectedPolygon.cpp
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 06/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm>

#include <QtCore/QDebug>

#include "GeometryUtil.h"
#include "SharedUtil.h"
#include "SharedLogging.h"
#include "CubeProjectedPolygon.h"


glm::vec2 BoundingRectangle::getVertex(int vertexNumber) const {
    switch (vertexNumber) {
        case BoundingRectangle::BOTTOM_LEFT:
            return corner;
        case BoundingRectangle::TOP_LEFT:
            return glm::vec2(corner.x, corner.y + size.y);
        case BoundingRectangle::BOTTOM_RIGHT:
            return glm::vec2(corner.x + size.x, corner.y);
        case BoundingRectangle::TOP_RIGHT:
            return corner + size;
    }
    assert(false); // not allowed
    return glm::vec2(0,0);
}

BoundingRectangle BoundingRectangle::topHalf() const {
    float halfY = size.y/2.0f;
    BoundingRectangle result(glm::vec2(corner.x,corner.y + halfY), glm::vec2(size.x, halfY));
    return result;
}

BoundingRectangle BoundingRectangle::bottomHalf() const {
    float halfY = size.y/2.0f;
    BoundingRectangle result(corner, glm::vec2(size.x, halfY));
    return result;
}

BoundingRectangle BoundingRectangle::leftHalf() const {
    float halfX = size.x/2.0f;
    BoundingRectangle result(corner, glm::vec2(halfX, size.y));
    return result;
}

BoundingRectangle BoundingRectangle::rightHalf() const {
    float halfX = size.x/2.0f;
    BoundingRectangle result(glm::vec2(corner.x + halfX , corner.y), glm::vec2(halfX, size.y));
    return result;
}

bool BoundingRectangle::contains(const BoundingRectangle& box) const {
    return ( _set &&
                (box.corner.x >= corner.x) &&
                (box.corner.y >= corner.y) &&
                (box.corner.x + box.size.x <= corner.x + size.x) &&
                (box.corner.y + box.size.y <= corner.y + size.y)
            );
}

bool BoundingRectangle::contains(const glm::vec2& point) const {
    return ( _set &&
                (point.x > corner.x) &&
                (point.y > corner.y) &&
                (point.x < corner.x + size.x) &&
                (point.y < corner.y + size.y)
            );
}

void BoundingRectangle::explandToInclude(const BoundingRectangle& box) {
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


void BoundingRectangle::printDebugDetails(const char* label) const {
    qCDebug(shared, "%s _set=%s\n    corner=%f,%f size=%f,%f\n    bounds=[(%f,%f) to (%f,%f)]",
            (label ? label : "BoundingRectangle"),
            debug::valueOf(_set), (double)corner.x, (double)corner.y, (double)size.x, (double)size.y,
            (double)corner.x, (double)corner.y, (double)(corner.x+size.x), (double)(corner.y+size.y));
}


long CubeProjectedPolygon::pointInside_calls = 0;
long CubeProjectedPolygon::occludes_calls = 0;
long CubeProjectedPolygon::intersects_calls = 0;


CubeProjectedPolygon::CubeProjectedPolygon(const BoundingRectangle& box) :
    _vertexCount(4),
    _maxX(-FLT_MAX), _maxY(-FLT_MAX), _minX(FLT_MAX), _minY(FLT_MAX),
    _distance(0)
{
    for (int i = 0; i < _vertexCount; i++) {
        setVertex(i, box.getVertex(i));
    }
}


void CubeProjectedPolygon::setVertex(int vertex, const glm::vec2& point) {
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

}

// can be optimized with new pointInside()
bool CubeProjectedPolygon::occludes(const CubeProjectedPolygon& occludee, bool checkAllInView) const {

    CubeProjectedPolygon::occludes_calls++;

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

    // we need to test for identity as well, because in the case of identity, none of the points
    // will be "inside" but we don't want to bail early on the first non-inside point
    bool potentialIdenity = false;
    if ((occludee.getVertexCount() == getVertexCount()) && (getBoundingBox().contains(occludee.getBoundingBox())) ) {
        potentialIdenity = true;
    }
    // if we got this far, then check each vertex of the occludee, if all those points
    // are inside our polygon, then the tested occludee is fully occluded
    int pointsInside = 0;
    for(int i = 0; i < occludee.getVertexCount(); i++) {
        bool vertexMatched = false;
        if (!pointInside(occludee.getVertex(i), &vertexMatched)) {

            // so the point we just tested isn't inside, but it might have matched a vertex
            // if it didn't match a vertext, then we bail because we can't be an identity
            // or if we're not expecting identity, then we also bail early, no matter what
            if (!potentialIdenity || !vertexMatched) {
                return false;
            }
        } else {
            pointsInside++;
        }
    }

    // we're only here if all points are inside matched and/or we had a potentialIdentity we need to check
    if (pointsInside == occludee.getVertexCount()) {
        return true;
    }

    // If we have the potential for identity, then test to see if we match, if we match, we occlude
    if (potentialIdenity) {
        return matches(occludee);
    }

    return false; // if we got this far, then we're not occluded
}

bool CubeProjectedPolygon::occludes(const BoundingRectangle& boxOccludee) const {
    CubeProjectedPolygon testee(boxOccludee);
    return occludes(testee);
}

bool CubeProjectedPolygon::matches(const CubeProjectedPolygon& testee) const {
    if (testee.getVertexCount() != getVertexCount()) {
        return false;
    }
    int vertextCount = getVertexCount();
    // find which testee vertex matches our first polygon vertex.
    glm::vec2 polygonVertex = getVertex(0);
    int originIndex = 0;
    for(int i = 0; i < vertextCount; i++) {
        glm::vec2 testeeVertex = testee.getVertex(i);

        // if they match, we found our origin.
        if (testeeVertex == polygonVertex) {
            originIndex = i;
            break;
        }
    }
    // Now, starting at the originIndex, walk the vertices of both the testee and ourselves

    for(int i = 0; i < vertextCount; i++) {
        glm::vec2 testeeVertex  = testee.getVertex((i + originIndex) % vertextCount);
        glm::vec2 polygonVertex = getVertex(i);
        if (testeeVertex != polygonVertex) {
            return false; // we don't match, therefore we're not the same
        }
    }
    return true; // all of our vertices match, therefore we're the same
}

bool CubeProjectedPolygon::matches(const BoundingRectangle& box) const {
    CubeProjectedPolygon testee(box);
    return matches(testee);
}

bool CubeProjectedPolygon::pointInside(const glm::vec2& point, bool* matchesVertex) const {

    CubeProjectedPolygon::pointInside_calls++;

    // first check the bounding boxes, the point must be fully within the boounding box of this polygon
    if ((point.x > getMaxX()) ||
        (point.y > getMaxY()) ||
        (point.x < getMinX()) ||
        (point.y < getMinY())) {
        return false;
    }

    // consider each edge of this polygon as a potential separating axis
    // check the point against each edge
    for (int i = 0; i < getVertexCount(); i++) {
        glm::vec2 start = getVertex(i);
        glm::vec2 end   = getVertex((i + 1) % getVertexCount());
        float a = start.y - end.y;
        float b = end.x - start.x;
        float c = a * start.x + b * start.y;
        if (a * point.x + b * point.y < c) {
            return false;
        }
    }

    return true;
}

void CubeProjectedPolygon::printDebugDetails() const {
    qCDebug(shared, "CubeProjectedPolygon..."
            "    minX=%f maxX=%f minY=%f maxY=%f", (double)getMinX(), (double)getMaxX(), (double)getMinY(), (double)getMaxY());
    qCDebug(shared, "    vertex count=%d distance=%f", getVertexCount(), (double)getDistance());
    for (int i = 0; i < getVertexCount(); i++) {
        glm::vec2 point = getVertex(i);
        qCDebug(shared, "    vertex[%d] = %f, %f ", i, (double)point.x, (double)point.y);
    }
}

bool CubeProjectedPolygon::intersects(const BoundingRectangle& box) const {
    CubeProjectedPolygon testee(box);
    return intersects(testee);
}

bool CubeProjectedPolygon::intersects(const CubeProjectedPolygon& testee) const {
    CubeProjectedPolygon::intersects_calls++;
    return intersectsOnAxes(testee) && testee.intersectsOnAxes(*this);
}

//
// Tests the edges of this polygon as potential separating axes for this polygon and the
// specified other.
//
// @return false if the polygons are disjoint on any of this polygon's axes, true if they
// intersect on all axes.
//
// Note: this only works on convex polygons
//
//
bool CubeProjectedPolygon::intersectsOnAxes(const CubeProjectedPolygon& testee) const {

    // consider each edge of this polygon as a potential separating axis
    for (int i = 0; i < getVertexCount(); i++) {
        glm::vec2 start = getVertex(i);
        glm::vec2 end   = getVertex((i + 1) % getVertexCount());
        float a = start.y - end.y;
        float b = end.x - start.x;
        float c = a * start.x + b * start.y;

        // if all vertices fall outside the edge, the polygons are disjoint
        // points that are ON the edge, are considered to be "outside"
        for (int j = 0; j < testee.getVertexCount(); j++) {
            glm::vec2 testeeVertex = testee.getVertex(j);

            // in comparison below:
            //      >= will cause points on edge to be considered inside
            //      >  will cause points on edge to be considered outside

            float c2 = a * testeeVertex.x + b * testeeVertex.y;
            if (c2 >= c) {
                goto CONTINUE_OUTER;
            }
        }
        return false;
        CONTINUE_OUTER: ;
    }
    return true;
}

bool CubeProjectedPolygon::canMerge(const CubeProjectedPolygon& that) const {

    // RIGHT/NEAR
    // LEFT/NEAR
    if (
        (getProjectionType() == that.getProjectionType()) &&
        (
             getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR) ||
             getProjectionType() == (PROJECTION_LEFT | PROJECTION_NEAR)
        )
       ) {
        if (getVertex(1) == that.getVertex(0) && getVertex(4) == that.getVertex(5)) {
            return true;
        }
        if (getVertex(0) == that.getVertex(1) && getVertex(5) == that.getVertex(4)) {
            return true;
        }
        if (getVertex(2) == that.getVertex(1) && getVertex(3) == that.getVertex(4)) {
            return true;
        }
        if (getVertex(1) == that.getVertex(2) && getVertex(4) == that.getVertex(3)) {
            return true;
        }
    }

    // NEAR/BOTTOM
    if (
        (getProjectionType() == that.getProjectionType()) &&
        (
             getProjectionType() == (PROJECTION_NEAR | PROJECTION_BOTTOM)
        )
       ) {
        if (getVertex(0) == that.getVertex(5) && getVertex(3) == that.getVertex(4)) {
            return true;
        }
        if (getVertex(5) == that.getVertex(0) && getVertex(4) == that.getVertex(3)) {
            return true;
        }
        if (getVertex(1) == that.getVertex(0) && getVertex(2) == that.getVertex(3)) {
            return true;
        }
        if (getVertex(0) == that.getVertex(1) && getVertex(3) == that.getVertex(2)) {
            return true;
        }
    }

    // NEAR/TOP
    if (
        (getProjectionType() == that.getProjectionType()) &&
        (
             getProjectionType() == (PROJECTION_NEAR | PROJECTION_TOP)
        )
       ) {
        if (getVertex(0) == that.getVertex(5) && getVertex(1) == that.getVertex(2)) {
            return true;
        }
        if (getVertex(5) == that.getVertex(0) && getVertex(2) == that.getVertex(1)) {
            return true;
        }
        if (getVertex(4) == that.getVertex(5) && getVertex(3) == that.getVertex(2)) {
            return true;
        }
        if (getVertex(5) == that.getVertex(4) && getVertex(2) == that.getVertex(3)) {
            return true;
        }
    }

    // RIGHT/NEAR & NEAR/RIGHT/TOP
    // LEFT/NEAR  & NEAR/LEFT/TOP
    if (
            ((getProjectionType()     == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_TOP)) &&
            (that.getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR)))
            ||
            ((getProjectionType()     == (PROJECTION_LEFT  | PROJECTION_NEAR | PROJECTION_TOP)) &&
            (that.getProjectionType() == (PROJECTION_LEFT  | PROJECTION_NEAR)))
        )
    {
        if (getVertex(5) == that.getVertex(0) && getVertex(3) == that.getVertex(2)) {
            return true;
        }
    }
    // RIGHT/NEAR & NEAR/RIGHT/TOP
    // LEFT/NEAR  & NEAR/LEFT/TOP
    if (
            ((that.getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_TOP)) &&
            (getProjectionType()       == (PROJECTION_RIGHT | PROJECTION_NEAR)))
            ||
            ((that.getProjectionType() == (PROJECTION_LEFT  | PROJECTION_NEAR | PROJECTION_TOP)) &&
            (getProjectionType()       == (PROJECTION_LEFT  | PROJECTION_NEAR)))

        )
    {
        if (getVertex(0) == that.getVertex(5) && getVertex(2) == that.getVertex(3)) {
            return true;
        }
    }

    // RIGHT/NEAR & NEAR/RIGHT/BOTTOM
    // NEAR/LEFT & NEAR/LEFT/BOTTOM
    if (
            ((that.getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_BOTTOM)) &&
            (getProjectionType()       == (PROJECTION_RIGHT | PROJECTION_NEAR)))
            ||
            ((that.getProjectionType() == (PROJECTION_LEFT | PROJECTION_NEAR | PROJECTION_BOTTOM)) &&
            (getProjectionType()       == (PROJECTION_LEFT | PROJECTION_NEAR)))

        )
    {
        if (getVertex(5) == that.getVertex(0) && getVertex(3) == that.getVertex(2)) {
            return true;
        }
    }
    // RIGHT/NEAR & NEAR/RIGHT/BOTTOM
    // NEAR/LEFT & NEAR/LEFT/BOTTOM
    if (
            ((getProjectionType()     == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_BOTTOM)) &&
            (that.getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR)))
            ||
            ((getProjectionType()     == (PROJECTION_LEFT | PROJECTION_NEAR | PROJECTION_BOTTOM)) &&
            (that.getProjectionType() == (PROJECTION_LEFT | PROJECTION_NEAR)))
        )
    {
        if (getVertex(0) == that.getVertex(5) && getVertex(2) == that.getVertex(3)) {
            return true;
        }
    }

    // NEAR/TOP & NEAR
    if (
            (getProjectionType()      == (PROJECTION_NEAR                   )) &&
            (that.getProjectionType() == (PROJECTION_NEAR  | PROJECTION_TOP ))
        )
    {
        if (getVertex(0) == that.getVertex(5) && getVertex(1) == that.getVertex(2)) {
            return true;
        }
    }

    // NEAR/TOP & NEAR
    if (
            (that.getProjectionType() == (PROJECTION_NEAR                   )) &&
            (getProjectionType()      == (PROJECTION_NEAR  | PROJECTION_TOP ))
        )
    {
        if (getVertex(5) == that.getVertex(0) && getVertex(2) == that.getVertex(1)) {
            return true;
        }
    }

    // NEAR/BOTTOM & NEAR
    if (
            (getProjectionType()      == (PROJECTION_NEAR                      )) &&
            (that.getProjectionType() == (PROJECTION_NEAR  | PROJECTION_BOTTOM ))
        )
    {
        if (getVertex(2) == that.getVertex(3) && getVertex(3) == that.getVertex(0)) {
            return true;
        }
    }

    // NEAR/BOTTOM & NEAR
    if (
            (that.getProjectionType() == (PROJECTION_NEAR                      )) &&
            (getProjectionType()      == (PROJECTION_NEAR  | PROJECTION_BOTTOM ))
        )
    {
        if (getVertex(3) == that.getVertex(2) && getVertex(0) == that.getVertex(3)) {
            return true;
        }
    }

    // NEAR/RIGHT & NEAR
    if (
            (getProjectionType()      == (PROJECTION_NEAR                      )) &&
            (that.getProjectionType() == (PROJECTION_NEAR  | PROJECTION_RIGHT ))
        )
    {
        if (getVertex(0) == that.getVertex(1) && getVertex(3) == that.getVertex(4)) {
            return true;
        }
    }

    // NEAR/RIGHT & NEAR
    if (
            (that.getProjectionType() == (PROJECTION_NEAR                      )) &&
            (getProjectionType()      == (PROJECTION_NEAR  | PROJECTION_RIGHT ))
        )
    {
        if (getVertex(1) == that.getVertex(0) && getVertex(4) == that.getVertex(3)) {
            return true;
        }
    }

    // NEAR/LEFT & NEAR
    if (
            (getProjectionType()      == (PROJECTION_NEAR                    )) &&
            (that.getProjectionType() == (PROJECTION_NEAR  | PROJECTION_LEFT ))
        )
    {
        if (getVertex(1) == that.getVertex(1) && getVertex(2) == that.getVertex(4)) {
            return true;
        }
    }

    // NEAR/LEFT & NEAR
    if (
            (that.getProjectionType() == (PROJECTION_NEAR                    )) &&
            (getProjectionType()      == (PROJECTION_NEAR  | PROJECTION_LEFT ))
        )
    {
        if (getVertex(1) == that.getVertex(0) && getVertex(4) == that.getVertex(3)) {
            return true;
        }
    }

    // NEAR/RIGHT/TOP & NEAR/TOP
    if (
            ((getProjectionType()     == (PROJECTION_TOP | PROJECTION_NEAR                     )) &&
            (that.getProjectionType() == (PROJECTION_TOP | PROJECTION_NEAR  | PROJECTION_RIGHT )))
        )
    {
        if (getVertex(0) == that.getVertex(1) && getVertex(4) == that.getVertex(3)) {
            return true;
        }
    }

    // NEAR/RIGHT/TOP & NEAR/TOP
    if (
            ((that.getProjectionType() == (PROJECTION_TOP | PROJECTION_NEAR                     )) &&
            (getProjectionType()      == (PROJECTION_TOP | PROJECTION_NEAR  | PROJECTION_RIGHT )))
        )
    {
        if (getVertex(1) == that.getVertex(0) && getVertex(3) == that.getVertex(4)) {
            return true;
        }
    }


    // NEAR/RIGHT/BOTTOM & NEAR/BOTTOM
    if (
            ((getProjectionType()     == (PROJECTION_BOTTOM | PROJECTION_NEAR                     )) &&
            (that.getProjectionType() == (PROJECTION_BOTTOM | PROJECTION_NEAR  | PROJECTION_RIGHT )))
        )
    {
        if (getVertex(1) == that.getVertex(2) && getVertex(5) == that.getVertex(4)) {
            return true;
        }
    }

    // NEAR/RIGHT/BOTTOM & NEAR/BOTTOM
    if (
            ((that.getProjectionType() == (PROJECTION_BOTTOM | PROJECTION_NEAR                     )) &&
            (getProjectionType()      == (PROJECTION_BOTTOM | PROJECTION_NEAR  | PROJECTION_RIGHT )))
        )
    {
        if (getVertex(2) == that.getVertex(1) && getVertex(4) == that.getVertex(5)) {
            return true;
        }
    }

    // NEAR/LEFT/BOTTOM & NEAR/BOTTOM
    if (
            ((getProjectionType()     == (PROJECTION_BOTTOM | PROJECTION_NEAR                     )) &&
            (that.getProjectionType() == (PROJECTION_BOTTOM | PROJECTION_NEAR  | PROJECTION_LEFT )))
        )
    {
        if (getVertex(2) == that.getVertex(0) && getVertex(4) == that.getVertex(4)) {
            return true;
        }
    }

    // NEAR/LEFT/BOTTOM & NEAR/BOTTOM
    if (
            ((that.getProjectionType() == (PROJECTION_BOTTOM | PROJECTION_NEAR                     )) &&
            (getProjectionType()       == (PROJECTION_BOTTOM | PROJECTION_NEAR  | PROJECTION_LEFT )))
        )
    {
        if (getVertex(0) == that.getVertex(2) && getVertex(4) == that.getVertex(4)) {
            return true;
        }
    }
    // RIGHT/NEAR/BOTTOM
    // RIGHT/NEAR/TOP
    // LEFT/NEAR/BOTTOM
    // LEFT/NEAR/TOP
    if (
            (getProjectionType() == that.getProjectionType()) &&
            (
                getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_BOTTOM ) ||
                getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_TOP    ) ||
                getProjectionType() == (PROJECTION_LEFT  | PROJECTION_NEAR | PROJECTION_BOTTOM ) ||
                getProjectionType() == (PROJECTION_LEFT  | PROJECTION_NEAR | PROJECTION_TOP    )
            )
       ) {
        if (getVertex(0) == that.getVertex(5) && getVertex(2) == that.getVertex(3)) {
            return true;
        }
        if (getVertex(5) == that.getVertex(0) && getVertex(3) == that.getVertex(2)) {
            return true;
        }
        if (getVertex(2) == that.getVertex(1) && getVertex(4) == that.getVertex(5)) {
            return true;
        }
        if (getVertex(1) == that.getVertex(2) && getVertex(5) == that.getVertex(4)) {
            return true;
        }
        if (getVertex(1) == that.getVertex(0) && getVertex(3) == that.getVertex(4)) {
            return true;
        }
        if (getVertex(0) == that.getVertex(1) && getVertex(4) == that.getVertex(3)) {
            return true;
        }
    }

    return false;
}


void CubeProjectedPolygon::merge(const CubeProjectedPolygon& that) {

    // RIGHT/NEAR
    // LEFT/NEAR
    if (
        (getProjectionType() == that.getProjectionType()) &&
        (
             getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR) ||
             getProjectionType() == (PROJECTION_LEFT  | PROJECTION_NEAR)
        )
       ) {
        if (getVertex(1) == that.getVertex(0) && getVertex(4) == that.getVertex(5)) {
            //setVertex(0, this.getVertex(0)); // no change
            setVertex(1, that.getVertex(1));
            setVertex(2, that.getVertex(2));
            setVertex(3, that.getVertex(3));
            setVertex(4, that.getVertex(4));
            //setVertex(5, this.getVertex(5)); // no change
            return; // done
        }
        if (getVertex(0) == that.getVertex(1) && getVertex(5) == that.getVertex(4)) {
            setVertex(0, that.getVertex(0));
            //setVertex(1, this.getVertex(1)); // no change
            //setVertex(2, this.getVertex(2)); // no change
            //setVertex(3, this.getVertex(3)); // no change
            //setVertex(4, that.getVertex(4)); // no change
            setVertex(5, that.getVertex(5));
            return; // done
        }
        if (getVertex(2) == that.getVertex(1) && getVertex(3) == that.getVertex(4)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, this.getVertex(1)); // no change
            setVertex(2, that.getVertex(2));
            setVertex(3, that.getVertex(3));
            //setVertex(4, this.getVertex(4)); // no change
            //setVertex(5, that.getVertex(5)); // no change
            return; // done
        }
        if (getVertex(1) == that.getVertex(2) && getVertex(4) == that.getVertex(3)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            //setVertex(2, this.getVertex(2)); // no change
            //setVertex(3, that.getVertex(3)); // no change
            setVertex(4, that.getVertex(4));
            setVertex(5, that.getVertex(5));
            return; // done
        }
    }

    // NEAR/BOTTOM
    if (
        (getProjectionType() == that.getProjectionType()) &&
        (
             getProjectionType() == (PROJECTION_NEAR | PROJECTION_BOTTOM)
        )
       ) {
        if (getVertex(0) == that.getVertex(5) && getVertex(3) == that.getVertex(4)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            setVertex(2, that.getVertex(2));
            setVertex(3, that.getVertex(3));
            //setVertex(4, this.getVertex(4)); // no change
            //setVertex(5, that.getVertex(5)); // no change
            return; // done
        }
        if (getVertex(5) == that.getVertex(0) && getVertex(4) == that.getVertex(3)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, that.getVertex(1)); // no change
            //setVertex(2, this.getVertex(2)); // no change
            //setVertex(3, that.getVertex(3)); // no change
            setVertex(4, that.getVertex(4));
            setVertex(5, that.getVertex(5));
            return; // done
        }
        if (getVertex(1) == that.getVertex(0) && getVertex(2) == that.getVertex(3)) {
            //setVertex(0, this.getVertex(0)); // no change
            setVertex(1, that.getVertex(1));
            setVertex(2, that.getVertex(2));
            //setVertex(3, that.getVertex(3)); // no change
            //setVertex(4, this.getVertex(4)); // no change
            //setVertex(5, that.getVertex(5)); // no change
            return; // done
        }
        if (getVertex(0) == that.getVertex(1) && getVertex(3) == that.getVertex(2)) {
            setVertex(0, that.getVertex(0));
            //setVertex(1, this.getVertex(1)); // no change
            //setVertex(2, that.getVertex(2)); // no change
            setVertex(3, that.getVertex(3));
            setVertex(4, that.getVertex(4));
            setVertex(5, that.getVertex(5));
            return; // done
        }
    }

    // NEAR/TOP
    if (
        (getProjectionType() == that.getProjectionType()) &&
        (
             getProjectionType() == (PROJECTION_NEAR | PROJECTION_TOP)
        )
       ) {
        if (getVertex(0) == that.getVertex(5) && getVertex(1) == that.getVertex(2)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            //setVertex(2, this.getVertex(2)); // no change
            //setVertex(3, that.getVertex(3)); // no change
            //setVertex(4, this.getVertex(4)); // no change
            //setVertex(5, that.getVertex(5)); // no change
            return; // done
        }
        if (getVertex(5) == that.getVertex(0) && getVertex(2) == that.getVertex(1)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, that.getVertex(1)); // no change
            setVertex(2, that.getVertex(2));
            setVertex(3, that.getVertex(3));
            setVertex(4, that.getVertex(4));
            setVertex(5, that.getVertex(5));
            return; // done
        }
        if (getVertex(4) == that.getVertex(5) && getVertex(3) == that.getVertex(2)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, that.getVertex(1)); // no change
            //setVertex(2, that.getVertex(2)); // no change
            setVertex(3, that.getVertex(3));
            setVertex(4, that.getVertex(4));
            //setVertex(5, that.getVertex(5)); // no change
            return; // done
        }
        if (getVertex(5) == that.getVertex(4) && getVertex(2) == that.getVertex(3)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            setVertex(2, that.getVertex(2));
            //setVertex(3, this.getVertex(3)); // no change
            //setVertex(4, that.getVertex(3)); // no change
            setVertex(5, that.getVertex(5));
            return; // done
        }
    }


    // RIGHT/NEAR & NEAR/RIGHT/TOP
    // LEFT/NEAR  & NEAR/LEFT/TOP
    if (
            ((getProjectionType()     == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_TOP)) &&
            (that.getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR)))
            ||
            ((getProjectionType()     == (PROJECTION_LEFT  | PROJECTION_NEAR | PROJECTION_TOP)) &&
            (that.getProjectionType() == (PROJECTION_LEFT  | PROJECTION_NEAR)))
        )
    {
        if (getVertex(5) == that.getVertex(0) && getVertex(3) == that.getVertex(2)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, this.getVertex(1)); // no change
            //setVertex(2, this.getVertex(2)); // no change
            setVertex(3, that.getVertex(3));
            setVertex(4, that.getVertex(4));
            setVertex(5, that.getVertex(5));
            setProjectionType((PROJECTION_RIGHT | PROJECTION_NEAR));
            return; // done
        }
    }

    // RIGHT/NEAR & NEAR/RIGHT/TOP
    // LEFT/NEAR  & NEAR/LEFT/TOP
    if (
            ((that.getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_TOP)) &&
            (getProjectionType()       == (PROJECTION_RIGHT | PROJECTION_NEAR)))
            ||
            ((that.getProjectionType() == (PROJECTION_LEFT  | PROJECTION_NEAR | PROJECTION_TOP)) &&
            (getProjectionType()       == (PROJECTION_LEFT  | PROJECTION_NEAR)))

        )
    {
        if (getVertex(0) == that.getVertex(5) && getVertex(2) == that.getVertex(3)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            setVertex(2, that.getVertex(2));
            //setVertex(3, this.getVertex(3)); // no change
            //setVertex(4, this.getVertex(4)); // no change
            //setVertex(5, this.getVertex(5)); // no change
            //setProjectionType((PROJECTION_RIGHT | PROJECTION_NEAR)); // no change
            return; // done
        }
    }

    // RIGHT/NEAR & NEAR/RIGHT/BOTTOM
    // NEAR/LEFT & NEAR/LEFT/BOTTOM
    if (
            ((that.getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_BOTTOM)) &&
            (getProjectionType()       == (PROJECTION_RIGHT | PROJECTION_NEAR)))
            ||
            ((that.getProjectionType() == (PROJECTION_LEFT | PROJECTION_NEAR | PROJECTION_BOTTOM)) &&
            (getProjectionType()       == (PROJECTION_LEFT | PROJECTION_NEAR)))

        )
    {
        if (getVertex(5) == that.getVertex(0) && getVertex(3) == that.getVertex(2)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, this.getVertex(1)); // no change
            //setVertex(2, this.getVertex(2)); // no change
            setVertex(3, that.getVertex(3));
            setVertex(4, that.getVertex(4));
            setVertex(5, that.getVertex(5));
            //setProjectionType((PROJECTION_RIGHT | PROJECTION_NEAR)); // no change
            return; // done
        }
    }
    // RIGHT/NEAR & NEAR/RIGHT/BOTTOM
    // NEAR/LEFT & NEAR/LEFT/BOTTOM
    if (
            ((getProjectionType()     == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_BOTTOM)) &&
            (that.getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR)))
            ||
            ((getProjectionType()     == (PROJECTION_LEFT | PROJECTION_NEAR | PROJECTION_BOTTOM)) &&
            (that.getProjectionType() == (PROJECTION_LEFT | PROJECTION_NEAR)))
        )
    {
        if (getVertex(0) == that.getVertex(5) && getVertex(2) == that.getVertex(3)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            setVertex(2, that.getVertex(2));
            //setVertex(3, this.getVertex(3)); // no change
            //setVertex(4, this.getVertex(4)); // no change
            //setVertex(5, this.getVertex(5)); // no change
            setProjectionType((PROJECTION_RIGHT | PROJECTION_NEAR));
            return; // done
        }
    }


    // NEAR/TOP & NEAR
    if (
            (getProjectionType()      == (PROJECTION_NEAR                   )) &&
            (that.getProjectionType() == (PROJECTION_NEAR  | PROJECTION_TOP ))
        )
    {
        if (getVertex(0) == that.getVertex(5) && getVertex(1) == that.getVertex(2)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            //setVertex(2, this.getVertex(2)); // no change
            //setVertex(3, this.getVertex(3)); // no change
            //setVertexCount(4); // no change
            //setProjectionType((PROJECTION_NEAR));  // no change
            return; // done
        }
    }

    // NEAR/TOP & NEAR
    if (
            (that.getProjectionType() == (PROJECTION_NEAR                   )) &&
            (getProjectionType()      == (PROJECTION_NEAR  | PROJECTION_TOP ))
        )
    {
        if (getVertex(5) == that.getVertex(0) && getVertex(2) == that.getVertex(1)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, this.getVertex(1)); // no change
            setVertex(2, that.getVertex(2));
            setVertex(3, that.getVertex(3));
            setVertexCount(4);
            setProjectionType((PROJECTION_NEAR));
            return; // done
        }
    }

    // NEAR/BOTTOM & NEAR
    if (
            (getProjectionType()      == (PROJECTION_NEAR                      )) &&
            (that.getProjectionType() == (PROJECTION_NEAR  | PROJECTION_BOTTOM ))
        )
    {
        if (getVertex(2) == that.getVertex(3) && getVertex(3) == that.getVertex(0)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, this.getVertex(1)); // no change
            setVertex(2, that.getVertex(4));
            setVertex(3, that.getVertex(5));
            //setVertexCount(4); // no change
            //setProjectionType((PROJECTION_NEAR));  // no change
        }
    }

    // NEAR/BOTTOM & NEAR
    if (
            (that.getProjectionType() == (PROJECTION_NEAR                      )) &&
            (getProjectionType()      == (PROJECTION_NEAR  | PROJECTION_BOTTOM ))
        )
    {
        if (getVertex(3) == that.getVertex(2) && getVertex(0) == that.getVertex(3)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            setVertex(2, getVertex(4));
            setVertex(3, getVertex(5));
            setVertexCount(4);
            setProjectionType((PROJECTION_NEAR));
            return; // done
        }
    }

    // NEAR/RIGHT & NEAR
    if (
            (getProjectionType()      == (PROJECTION_NEAR                      )) &&
            (that.getProjectionType() == (PROJECTION_NEAR  | PROJECTION_RIGHT ))
        )
    {
        if (getVertex(0) == that.getVertex(1) && getVertex(3) == that.getVertex(4)) {
            setVertex(0, that.getVertex(0));
            //setVertex(1, this.getVertex(1)); // no change
            //setVertex(2, this.getVertex(2)); // no change
            setVertex(3, that.getVertex(5));
            //setVertexCount(4); // no change
            //setProjectionType((PROJECTION_NEAR));  // no change
        }
    }

    // NEAR/RIGHT & NEAR
    if (
            (that.getProjectionType() == (PROJECTION_NEAR                      )) &&
            (getProjectionType()      == (PROJECTION_NEAR  | PROJECTION_RIGHT ))
        )
    {
        if (getVertex(1) == that.getVertex(0) && getVertex(4) == that.getVertex(3)) {
            //setVertex(0, this.getVertex(0)); // no change
            setVertex(1, that.getVertex(1));
            setVertex(2, that.getVertex(2));
            setVertex(3, getVertex(5));
            setVertexCount(4);
            setProjectionType((PROJECTION_NEAR));
            return; // done
        }
    }

    // NEAR/LEFT & NEAR
    if (
            (getProjectionType()      == (PROJECTION_NEAR                    )) &&
            (that.getProjectionType() == (PROJECTION_NEAR  | PROJECTION_LEFT ))
        )
    {
        if (getVertex(1) == that.getVertex(1) && getVertex(2) == that.getVertex(4)) {
            //setVertex(0, this.getVertex()); // no change
            setVertex(1, that.getVertex(2));
            setVertex(2, that.getVertex(3));
            //setVertex(3, this.getVertex(3)); // no change
            //setVertexCount(4); // no change
            //setProjectionType((PROJECTION_NEAR));  // no change
            return; // done
        }
    }

    // NEAR/LEFT & NEAR
    if (
            (that.getProjectionType() == (PROJECTION_NEAR                    )) &&
            (getProjectionType()      == (PROJECTION_NEAR  | PROJECTION_LEFT ))
        )
    {
        if (getVertex(1) == that.getVertex(0) && getVertex(4) == that.getVertex(3)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, getVertex(2));
            setVertex(2, getVertex(3));
            setVertex(3, that.getVertex(3));
            setVertexCount(4);
            setProjectionType((PROJECTION_NEAR));
            return; // done
        }
    }

    // NEAR/RIGHT/TOP & NEAR/TOP
    if (
            ((getProjectionType()     == (PROJECTION_TOP | PROJECTION_NEAR                     )) &&
            (that.getProjectionType() == (PROJECTION_TOP | PROJECTION_NEAR  | PROJECTION_RIGHT )))
        )
    {
        if (getVertex(0) == that.getVertex(1) && getVertex(4) == that.getVertex(3)) {
            setVertex(0, that.getVertex(0));
            //setVertex(1, this.getVertex(1)); // no change
            //setVertex(2, this.getVertex(2)); // no change
            //setVertex(3, this.getVertex(3)); // no change
            setVertex(4, that.getVertex(4));
            setVertex(5, that.getVertex(5));
            return; // done
        }
    }

    // NEAR/RIGHT/TOP & NEAR/TOP
    if (
            ((that.getProjectionType() == (PROJECTION_TOP | PROJECTION_NEAR                     )) &&
            (getProjectionType()      == (PROJECTION_TOP | PROJECTION_NEAR  | PROJECTION_RIGHT )))
        )
    {
        if (getVertex(1) == that.getVertex(0) && getVertex(3) == that.getVertex(4)) {
            //setVertex(0, this.getVertex(0)); // no change
            setVertex(1, that.getVertex(1));
            setVertex(2, that.getVertex(2));
            setVertex(3, that.getVertex(3));
            //setVertex(4, this.getVertex(4)); // no change
            //setVertex(5, this.getVertex(5)); // no change
            setProjectionType((PROJECTION_TOP | PROJECTION_NEAR));
            return; // done
        }
    }


    // NEAR/RIGHT/BOTTOM & NEAR/BOTTOM
    if (
            ((getProjectionType()     == (PROJECTION_BOTTOM | PROJECTION_NEAR                     )) &&
            (that.getProjectionType() == (PROJECTION_BOTTOM | PROJECTION_NEAR  | PROJECTION_RIGHT )))
        )
    {
        if (getVertex(1) == that.getVertex(2) && getVertex(5) == that.getVertex(4)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            //setVertex(2, this.getVertex(2)); // no change
            //setVertex(3, this.getVertex(3)); // no change
            //setVertex(4, this.getVertex(4)); // no change
            setVertex(5, that.getVertex(5));
            return; // done
        }
    }

    // NEAR/RIGHT/BOTTOM & NEAR/BOTTOM
    if (
            ((that.getProjectionType() == (PROJECTION_BOTTOM | PROJECTION_NEAR                     )) &&
            (getProjectionType()       == (PROJECTION_BOTTOM | PROJECTION_NEAR  | PROJECTION_RIGHT )))
        )
    {
        if (getVertex(2) == that.getVertex(1) && getVertex(4) == that.getVertex(5)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, this.getVertex(1)); // no change
            setVertex(2, that.getVertex(2));
            setVertex(3, that.getVertex(3));
            setVertex(4, that.getVertex(4));
            //setVertex(5, this.getVertex(5)); // no change
            setProjectionType((PROJECTION_BOTTOM | PROJECTION_NEAR));
            return; // done
        }
    }

    // NEAR/LEFT/BOTTOM & NEAR/BOTTOM
    if (
            ((getProjectionType()     == (PROJECTION_BOTTOM | PROJECTION_NEAR                     )) &&
            (that.getProjectionType() == (PROJECTION_BOTTOM | PROJECTION_NEAR  | PROJECTION_LEFT )))
        )
    {
        if (getVertex(2) == that.getVertex(0) && getVertex(4) == that.getVertex(4)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, this.getVertex(1)); // no change
            setVertex(2, that.getVertex(1));
            setVertex(3, that.getVertex(2));
            setVertex(4, that.getVertex(3));
            //setVertex(5, this.getVertex(5)); // no change
            return; // done
        }
    }

    // NEAR/LEFT/BOTTOM & NEAR/BOTTOM
    if (
            ((that.getProjectionType() == (PROJECTION_BOTTOM | PROJECTION_NEAR                     )) &&
            (getProjectionType()       == (PROJECTION_BOTTOM | PROJECTION_NEAR  | PROJECTION_LEFT )))
        )
    {
        if (getVertex(0) == that.getVertex(2) && getVertex(4) == that.getVertex(4)) {
            // we need to do this in an unusual order, because otherwise we'd overwrite our own values
            setVertex(4, getVertex(3));
            setVertex(3, getVertex(2));
            setVertex(2, getVertex(1));
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            setVertex(5, that.getVertex(5));
            setProjectionType((PROJECTION_BOTTOM | PROJECTION_NEAR));
            return; // done
        }
    }


    // RIGHT/NEAR/BOTTOM
    // RIGHT/NEAR/TOP
    // LEFT/NEAR/BOTTOM
    // LEFT/NEAR/TOP
    if (
            (getProjectionType() == that.getProjectionType()) &&
            (
                getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_BOTTOM ) ||
                getProjectionType() == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_TOP    ) ||
                getProjectionType() == (PROJECTION_LEFT  | PROJECTION_NEAR | PROJECTION_BOTTOM ) ||
                getProjectionType() == (PROJECTION_LEFT  | PROJECTION_NEAR | PROJECTION_TOP    )
            )
       ) {
        if (getVertex(0) == that.getVertex(5) && getVertex(2) == that.getVertex(3)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            setVertex(2, that.getVertex(2));
            //setVertex(3, this.getVertex(3)); // no change
            //setVertex(4, this.getVertex(4)); // no change
            //setVertex(5, this.getVertex(5)); // no change
            return; // done
        }
        if (getVertex(5) == that.getVertex(0) && getVertex(3) == that.getVertex(2)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, this.getVertex(1)); // no change
            //setVertex(2, this.getVertex(2)); // no change
            setVertex(3, that.getVertex(3));
            setVertex(4, that.getVertex(4));
            setVertex(5, that.getVertex(5));
            return; // done
        }
        if (getVertex(2) == that.getVertex(1) && getVertex(4) == that.getVertex(5)) {
            //setVertex(0, this.getVertex(0)); // no change
            //setVertex(1, this.getVertex(1)); // no change
            setVertex(2, that.getVertex(2));
            setVertex(3, that.getVertex(3));
            setVertex(4, that.getVertex(4));
            //setVertex(5, this.getVertex(5)); // no change
            return; // done
        }
        if (getVertex(1) == that.getVertex(2) && getVertex(5) == that.getVertex(4)) {
            setVertex(0, that.getVertex(0));
            setVertex(1, that.getVertex(1));
            //setVertex(2, this.getVertex(2)); // no change
            //setVertex(3, this.getVertex(3)); // no change
            //setVertex(4, this.getVertex(4)); // no change
            setVertex(5, that.getVertex(5));
            return; // done
        }
    //   if this.([1],[3]) == that.([0],[4]) then create polygon: this.[0], that.[1], that.[2], that.[3], this.[4], this.[5]
        if (getVertex(1) == that.getVertex(0) && getVertex(3) == that.getVertex(4)) {
            //setVertex(0, this.getVertex(0)); // no change
            setVertex(1, that.getVertex(1));
            setVertex(2, that.getVertex(2));
            setVertex(3, that.getVertex(3));
            //setVertex(4, this.getVertex(4)); // no change
            //setVertex(5, this.getVertex(5)); // no change
            return; // done
        }
    //   if this.([0],[4]) == that.([1],[3]) then create polygon: that.[0], this.[1], this.[2], this.[3], that.[4], that.[5]
        if (getVertex(0) == that.getVertex(1) && getVertex(4) == that.getVertex(3)) {
            setVertex(0, that.getVertex(0));
            //setVertex(1, this.getVertex(1)); // no change
            //setVertex(2, this.getVertex(2)); // no change
            //setVertex(3, this.getVertex(3)); // no change
            setVertex(4, that.getVertex(4));
            setVertex(5, that.getVertex(5));
            return; // done
        }
    }

}
