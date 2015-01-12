//
//  CoverageMapV2.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 06/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm>
#include <cstring>

#include <QtCore/QDebug>

#include <SharedUtil.h>

#include "CoverageMapV2.h"

int CoverageMapV2::_mapCount = 0;
int CoverageMapV2::_checkMapRootCalls = 0;
int CoverageMapV2::_notAllInView = 0;
bool CoverageMapV2::wantDebugging = false;

const BoundingBox CoverageMapV2::ROOT_BOUNDING_BOX = BoundingBox(glm::vec2(-1.0f,-1.0f), glm::vec2(2.0f,2.0f));

// Coverage Map's polygon coordinates are from -1 to 1 in the following mapping to screen space.
//
//         (0,0)                   (windowWidth, 0)
//         -1,1                    1,1
//           +-----------------------+ 
//           |           |           |
//           |           |           |
//           | -1,0      |           |
//           |-----------+-----------|
//           |          0,0          |
//           |           |           |
//           |           |           |
//           |           |           |
//           +-----------------------+
//           -1,-1                  1,-1
// (0,windowHeight)                (windowWidth,windowHeight)
//

// Choosing a minimum sized polygon. Since we know a typical window is approximately 1500 pixels wide
// then a pixel on our screen will be ~ 2.0/1500 or 0.0013 "units" wide, similarly pixels are typically
// about that tall as well. If we say that polygons should be at least 10x10 pixels to be considered "big enough"
// then we can calculate a reasonable polygon area
const int TYPICAL_SCREEN_WIDTH_IN_PIXELS      = 1500;
const int MINIMUM_POLYGON_AREA_SIDE_IN_PIXELS = 10;
const float TYPICAL_SCREEN_PIXEL_WIDTH = (2.0f / TYPICAL_SCREEN_WIDTH_IN_PIXELS);
const float CoverageMapV2::MINIMUM_POLYGON_AREA_TO_STORE = (TYPICAL_SCREEN_PIXEL_WIDTH * MINIMUM_POLYGON_AREA_SIDE_IN_PIXELS) *
                                                         (TYPICAL_SCREEN_PIXEL_WIDTH * MINIMUM_POLYGON_AREA_SIDE_IN_PIXELS);
const float CoverageMapV2::NOT_COVERED = FLT_MAX;
const float CoverageMapV2::MINIMUM_OCCLUSION_CHECK_AREA = MINIMUM_POLYGON_AREA_TO_STORE/10.0f; // one quarter the size of poly


CoverageMapV2::CoverageMapV2(BoundingBox boundingBox, bool isRoot, bool isCovered, float coverageDistance) : 
    _isRoot(isRoot), 
    _myBoundingBox(boundingBox),
    _isCovered(isCovered),
    _coveredDistance(coverageDistance)
{ 
    _mapCount++;
    init(); 
};

CoverageMapV2::~CoverageMapV2() {
    erase();
};

void CoverageMapV2::erase() {

    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (_childMaps[i]) {
            delete _childMaps[i];
            _childMaps[i] = NULL;
        }
    }

    if (_isRoot && wantDebugging) {
        qDebug("CoverageMapV2 last to be deleted...");
        qDebug("MINIMUM_POLYGON_AREA_TO_STORE=%f",MINIMUM_POLYGON_AREA_TO_STORE);
        qDebug("_mapCount=%d",_mapCount);
        qDebug("_checkMapRootCalls=%d",_checkMapRootCalls);
        qDebug("_notAllInView=%d",_notAllInView);
        _mapCount = 0;
        _checkMapRootCalls = 0;
        _notAllInView = 0;
    }
}

void CoverageMapV2::init() {
    memset(_childMaps,0,sizeof(_childMaps));
}

// 0 = bottom, left
// 1 = bottom, right
// 2 = top, left
// 3 = top, right
BoundingBox CoverageMapV2::getChildBoundingBox(int childIndex) {
    const int RIGHT_BIT = 1;
    const int TOP_BIT  = 2;
    // initialize to our corner, and half our size
    BoundingBox result(_myBoundingBox.corner,_myBoundingBox.size/2.0f);
    // if our "right" bit is set, then add size.x to the corner
    if ((childIndex & RIGHT_BIT) == RIGHT_BIT) {
        result.corner.x += result.size.x;
    }
    // if our "top" bit is set, then add size.y to the corner
    if ((childIndex & TOP_BIT) == TOP_BIT) {
        result.corner.y += result.size.y;
    }
    return result;
}

// possible results = STORED/NOT_STORED, OCCLUDED, DOESNT_FIT
CoverageMapV2StorageResult CoverageMapV2::checkMap(const OctreeProjectedPolygon* polygon, bool storeIt) {
    assert(_isRoot); // you can only call this on the root map!!!
    _checkMapRootCalls++;

    // short circuit: if we're the root node (only case we're here), and we're covered, and this polygon is deeper than our 
    // covered depth, then this polygon is occluded!
    if (_isCovered && _coveredDistance < polygon->getDistance()) {
        return V2_OCCLUDED;
    }
    
    // short circuit: we don't handle polygons that aren't all in view, so, if the polygon in question is
    // not in view, then we just discard it with a DOESNT_FIT, this saves us time checking values later.
    if (!polygon->getAllInView()) {
        _notAllInView++;
        return V2_DOESNT_FIT;
    }
 
    // Here's where we recursively check the polygon against the coverage map. We need to maintain two pieces of state.
    // The first state is: have we seen at least one "fully occluded" map items. If we haven't then we don't track the covered
    // state of the polygon. 
    // The second piece of state is: Are all of our "fully occluded" map items "covered". If even one of these occluded map 
    // items is not covered, then our polygon is not covered.
    bool seenOccludedMapNodes = false;
    bool allOccludedMapNodesCovered = false;

    recurseMap(polygon, storeIt, seenOccludedMapNodes, allOccludedMapNodesCovered);
    
    // Ok, no matter how we were called, if all our occluded map nodes are covered, then we know this polygon
    // is occluded, otherwise, we will report back to the caller about whether or not we stored the polygon
    if (allOccludedMapNodesCovered) {
        return V2_OCCLUDED;
    } 
    if (storeIt) {
        return V2_STORED; // otherwise report that we STORED it
    }
    return V2_NOT_STORED; // unless we weren't asked to store it, then we didn't
}

void CoverageMapV2::recurseMap(const OctreeProjectedPolygon* polygon, bool storeIt, 
                bool& seenOccludedMapNodes, bool& allOccludedMapNodesCovered) {

    // if we are really small, then we act like we don't intersect, this allows us to stop
    // recusing as we get to the smalles edge of the polygon
    if (_myBoundingBox.area() < MINIMUM_OCCLUSION_CHECK_AREA) {
        return; // stop recursion, we're done!
    }

    // Determine if this map node intersects the polygon and/or is fully covered by the polygon
    // There are a couple special cases: If we're the root, we are assumed to intersect with all
    // polygons. Also, any map node that is fully occluded also intersects.
    bool nodeIsCoveredByPolygon = polygon->occludes(_myBoundingBox);
    bool nodeIsIntersectedByPolygon = nodeIsCoveredByPolygon || _isRoot || polygon->intersects(_myBoundingBox);

    // If we don't intersect, then we can just return, we're done recursing
    if (!nodeIsIntersectedByPolygon) {
        return; // stop recursion, we're done!
    }

    // At this point, we know our node intersects with the polygon. If this node is covered, then we want to treat it
    // as if the node was fully covered, because this allows us to short circuit further recursion...
    if (_isCovered && _coveredDistance < polygon->getDistance()) {
        nodeIsCoveredByPolygon = true; // fake it till you make it
    }
    
    // If this node in the map is fully covered by our polygon, then we don't need to recurse any further, but
    // we do need to do some bookkeeping.
    if (nodeIsCoveredByPolygon) {
        // If this is the very first fully covered node we've seen, then we're initialize our allOccludedMapNodesCovered
        // to be our current covered state. This has the following effect: if this node isn't already covered, then by
        // definition, we know that at least one node for this polygon isn't covered, and therefore we aren't fully covered.
        if (!seenOccludedMapNodes) {
            allOccludedMapNodesCovered = (_isCovered && _coveredDistance < polygon->getDistance());
            // We need to mark that we've seen at least one node of our polygon! ;)
            seenOccludedMapNodes = true;
        } else {
            // If this is our second or later node of our polygon, then we need to track our allOccludedMapNodesCovered state
            allOccludedMapNodesCovered = allOccludedMapNodesCovered && 
                                        (_isCovered && _coveredDistance < polygon->getDistance());
        }
        
        // if we're in store mode then we want to record that this node is covered. 
        if (storeIt) {
            _isCovered = true;
            // store the minimum distance of our previous known distance, or our current polygon's distance. This is because
            // we know that we're at least covered at this distance, but if we had previously identified that we're covered
            // at a shallower distance, then we want to maintain that distance
            _coveredDistance = std::min(polygon->getDistance(), _coveredDistance);
            
            // Note: this might be a good chance to delete child maps, but we're not going to do that at this point because
            // we're trying to maintain the known distances in the lower portion of the tree.
        }
        
        // and since this node of the quad map is covered, we can safely stop recursion. because we know all smaller map
        // nodes will also be covered.
        return;
    }
    
    // If we got here, then it means we know that this node is not fully covered by the polygon, but it does intersect 
    // with the polygon.
    
    // Another case is that we aren't yet marked as covered, and so we should recurse and process smaller quad tree nodes.
    // Note: we use this to determine if we can collapse the child quad trees and mark this node as covered
    bool allChildrenOccluded = true; 
    float maxChildCoveredDepth = NOT_COVERED;
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        BoundingBox childMapBoundingBox = getChildBoundingBox(i);
        // if no child map exists yet, then create it
        if (!_childMaps[i]) {
            // children get created with the coverage state of their parent.
            _childMaps[i] = new CoverageMapV2(childMapBoundingBox, NOT_ROOT, _isCovered, _coveredDistance);
        }

        _childMaps[i]->recurseMap(polygon, storeIt, seenOccludedMapNodes, allOccludedMapNodesCovered);
        
        // if so far, all of our children are covered, then record our furthest coverage distance
        if (allChildrenOccluded && _childMaps[i]->_isCovered) {
            maxChildCoveredDepth = std::max(maxChildCoveredDepth, _childMaps[i]->_coveredDistance);
        } else {
            // otherwise, at least one of our children is not covered, so not all are covered
            allChildrenOccluded = false;
        }
    }
    // if all the children are covered, this makes our quad tree "shallower" because it records that
    // entire quad is covered, it uses the "furthest" z-order so that if a shalower polygon comes through
    // we won't assume its occluded
    if (allChildrenOccluded && storeIt) {
        _isCovered = true;
        _coveredDistance = maxChildCoveredDepth;
    }

    // normal exit case... return...
}