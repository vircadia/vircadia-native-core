//
//  CoverageMapV2.cpp - 
//  hifi
//
//  Added by Brad Hefta-Gaub on 06/11/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <algorithm>

#include "CoverageMapV2.h"
#include <SharedUtil.h>
#include <cstring>
#include "Log.h"

int CoverageMapV2::_mapCount = 0;
int CoverageMapV2::_checkMapRootCalls = 0;
int CoverageMapV2::_notAllInView = 0;
bool CoverageMapV2::wantDebugging = false;

const BoundingBox CoverageMapV2::ROOT_BOUNDING_BOX = BoundingBox(glm::vec2(-1.f,-1.f), glm::vec2(2.f,2.f));

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
    //printLog("CoverageMapV2 created... _mapCount=%d\n",_mapCount);
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
        printLog("CoverageMapV2 last to be deleted...\n");
        printLog("MINIMUM_POLYGON_AREA_TO_STORE=%f\n",MINIMUM_POLYGON_AREA_TO_STORE);
        printLog("_mapCount=%d\n",_mapCount);
        printLog("_checkMapRootCalls=%d\n",_checkMapRootCalls);
        printLog("_notAllInView=%d\n",_notAllInView);
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

// possible results: 
//     OCCLUDED - node is occluded by the polygon
//     INTERSECT - node and polygon intersect (note: polygon may be occluded by node)
//     NO_INTERSECT - node and polygon intersect
CoverageMapV2StorageResult CoverageMapV2::checkNode(const VoxelProjectedPolygon* polygon) {

    float x1 = _myBoundingBox.corner.x;
    float y1 = _myBoundingBox.corner.y;
    float sx = _myBoundingBox.size.x;
    float sy = _myBoundingBox.size.y;
    //assert(_myBoundingBox.area() >= MINIMUM_OCCLUSION_CHECK_AREA/10.0f);

    // if we are really small, then we report that we don't intersect, this allows us to stop
    // recusing as we get to the smalles edge of the polygon
    if (_myBoundingBox.area() < MINIMUM_OCCLUSION_CHECK_AREA) {
        //printLog("CoverageMapV2::checkNode() -- (_myBoundingBox.area() < MINIMUM_OCCLUSION_CHECK_AREA) -- returning V2_NO_INTERSECT\n");
        return V2_NO_INTERSECT;
    }

    if (polygon->occludes(_myBoundingBox)) {
        //printLog("CoverageMapV2::checkNode() returning OCCLUDED\n");
        //polygon->printDebugDetails();
        //_myBoundingBox.printDebugDetails();
        return V2_OCCLUDED;
    } else if (polygon->intersects(_myBoundingBox)) {
        //printLog("CoverageMapV2::checkNode() -- polygon->intersects(_myBoundingBox) -- returning V2_INTERSECT\n");
        return V2_INTERSECT;
    }
    
    // if we got here and we're the root, then we know we interesect
    if (_isRoot) {
        //printLog("CoverageMapV2::checkNode() -- _isRoot -- returning V2_INTERSECT\n");
        return V2_INTERSECT;
    }

    //printLog("CoverageMapV2::checkNode() -- bottom -- returning V2_NO_INTERSECT\n");
    return V2_NO_INTERSECT;
}

// possible results = STORED/NOT_STORED, OCCLUDED, DOESNT_FIT
CoverageMapV2StorageResult CoverageMapV2::checkMap(const VoxelProjectedPolygon* polygon, bool storeIt) {
    if (_isRoot) {
        _checkMapRootCalls++;
        
        //printLog("CoverageMap2::checkMap()... storeIt=%s\n", debug::valueOf(storeIt));
        //polygon->printDebugDetails();
    } else {
        assert(false); // we should only call checkMap() on the root map
    }
    
    // short circuit: if we're the root node (only case we're here), and we're covered, and this polygon is deeper than our 
    // covered depth, then this polygon is occluded!
    if (_isCovered && _coveredDistance < polygon->getDistance()) {
        if (!storeIt) {
            printLog("CoverageMap2::checkMap()... V2_OCCLUDED  storeIt=FALSE ------- (_isCovered && _coveredDistance < polygon->getDistance())\n");
        }
        return V2_OCCLUDED;
    }

    // short circuit: we don't handle polygons that aren't all in view, so, if the polygon in question is
    // not in view, then we just discard it with a DOESNT_FIT, this saves us time checking values later.
    if (!polygon->getAllInView()) {
        _notAllInView++;
        //printLog("CoverageMap2::checkMap()... V2_DOESNT_FIT\n");
        return V2_DOESNT_FIT;
    }
    
    bool polygonIsCompletelyCovered = true; // assume the best.
    
    // this will recursively set the quad tree map as covered at a certain depth. It will also determine if the polygon
    // is occluded by existing coverage set in the map
    CoverageMapV2StorageResult result = coverNode(polygon, polygonIsCompletelyCovered, storeIt);

    // If we determined that the polygon was indeed covered already, then report back
    if (polygonIsCompletelyCovered) {
        //printLog("CoverageMap2::checkMap()... V2_OCCLUDED ------- (polygonIsCompletelyCovered)\n");
        if (!storeIt) {
            printLog("CoverageMap2::checkMap()... V2_OCCLUDED  storeIt=FALSE ------- (polygonIsCompletelyCovered)\n");
        }
        return V2_OCCLUDED;
    }
    if (storeIt) {
        //printLog("CoverageMap2::checkMap()... V2_STORED\n");
        return V2_STORED; // otherwise report that we STORED it
    }
    //printLog("CoverageMap2::checkMap()... V2_NOT_STORED\n");
    return V2_NOT_STORED; // unless we weren't asked to store it, then we didn't
}

// recurse down the quad tree, marking nodes that are fully covered by the polygon, that would give us essentially
// a bitmap of the polygon, these nodes are by definition covered for the next polygon we check.
// as we went down this tree, the nodes are either already marked as covered, or not, if any of them were not already marked 
// as covered then the polygon isn't occluded. We use the polygonIsCompletelyCovered reference, which is assumed to be
// optimistically set to true by the highest caller. We will logically AND this with all of the "isCovered" states for all 
// the nodes that are covered by the polygon. Then end result will tell us if the polygon was occluded
CoverageMapV2StorageResult CoverageMapV2::coverNode(const VoxelProjectedPolygon* polygon, 
                              bool& polygonIsCompletelyCovered, bool storeIt) {
    //printLog("CoverageMapV2::coverNode()... BEFORE checkNode() \n");
    //_myBoundingBox.printDebugDetails("coverNode()");

    CoverageMapV2StorageResult coverageState = checkNode(polygon);

    //printLog("CoverageMapV2::coverNode()... AFTER checkNode() \n");

    //
    //   if node is super small, then we need to stop recursion, we probably should do this by saying
    //      at a certain small size of node, an intersect is as good as an occlude. this is handled by checkNode()
    //
    if (coverageState == V2_NO_INTERSECT) {
        //printLog("CoverageMapV2::coverNode()... V2_NO_INTERSECT returning false \n");
        if (!storeIt) {
            //printLog("CoverageMap2::checkMap()... coverageState == V2_NO_INTERSECT  storeIt=FALSE polygonIsCompletelyCovered=%s\n",debug::valueOf(polygonIsCompletelyCovered));
        }
        return V2_NO_INTERSECT; // don't recurse further, this node isn't covered
    }
    if (coverageState == V2_OCCLUDED) { // this node's rect is completely covered by the polygon
        // if this node is already covered, and it's coverage depth is in front of our polygon depth
        // then we logically AND true into the completelyCovered state, if this node is not covered, or it's
        // depth is greater than the polygon, then this polygon is not covered at this part of it
        // and we should set the isCompletelyCovered to false, which this accomplishes
        polygonIsCompletelyCovered = polygonIsCompletelyCovered && (_isCovered && _coveredDistance < polygon->getDistance());
        if (!storeIt && !_isCovered) {
            printLog("CoverageMap2::checkMap()... coverageState == V2_OCCLUDED  storeIt=FALSE _isCovered=FALSE polygonIsCompletelyCovered=%s\n",debug::valueOf(polygonIsCompletelyCovered));
        }
        
        if (storeIt) {
            _coveredDistance = _isCovered ? std::min(_coveredDistance, polygon->getDistance()) : polygon->getDistance(); 
            _isCovered = true; // we are definitely covered
        }
        //printLog("CoverageMapV2::coverNode()... V2_OCCLUDED returning _isCovered=%s \n", debug::valueOf(_isCovered));

        if (!storeIt) {
            printLog("CoverageMap2::checkMap()... coverageState == V2_OCCLUDED  storeIt=FALSE polygonIsCompletelyCovered=%s _isCovered=%s _coveredDistance=%f  polygon->getDistance()=%f\n",debug::valueOf(polygonIsCompletelyCovered), debug::valueOf(_isCovered),_coveredDistance,polygon->getDistance());
        }

        // If this node is covered by the polygon, then we want to report either OCCLUDED or NOT_OCCLUDED
        return ((_isCovered && _coveredDistance < polygon->getDistance()) ? V2_OCCLUDED : V2_NOT_OCCLUDED); // don't recurse further
    }
    if (coverageState == V2_INTERSECT) {
        //printLog("CoverageMapV2::coverNode()... V2_INTERSECT case... check children \n");
        
        // NOTE: if we're covered, then we don't really need to recurse any further down!
        if (_isCovered && _coveredDistance < polygon->getDistance()) {
            //printLog("CoverageMapV2::coverNode()... V2_INTERSECT AND _isCovered... can we pop here??? \n");
            polygonIsCompletelyCovered = polygonIsCompletelyCovered && (_isCovered && _coveredDistance < polygon->getDistance());

            if (!storeIt) {
                printLog("CoverageMap2::checkMap()... coverageState == V2_INTERSECT  storeIt=FALSE polygonIsCompletelyCovered=%s\n",debug::valueOf(polygonIsCompletelyCovered));
            }

            // If this node intersects the polygon, then we want to report either OCCLUDED or V2_INTERSECT
            return ((_isCovered && _coveredDistance < polygon->getDistance()) ? V2_OCCLUDED : V2_INTERSECT); // don't recurse further
        }
        
        //recurse deeper, and perform same operation on child quads nodes
        bool allChildrenOccluded = true; // assume the best
        float maxChildCoveredDepth = NOT_COVERED;
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            BoundingBox childMapBoundingBox = getChildBoundingBox(i);
            // if no child map exists yet, then create it
            if (!_childMaps[i]) {
                // children get created with the coverage state of their parent.
                _childMaps[i] = new CoverageMapV2(childMapBoundingBox, NOT_ROOT, _isCovered, _coveredDistance);
            }

            //printLog("CoverageMapV2::coverNode()... calling coverNode() for child[%d] \n", i);
            CoverageMapV2StorageResult childResult = _childMaps[i]->coverNode(polygon, polygonIsCompletelyCovered, storeIt);
            //printLog("CoverageMapV2::coverNode()... DONE calling coverNode() for child[%d] childCovered=%s, _childMaps[i]->_isCovered=%s \n", i,
            //                debug::valueOf(childCovered), debug::valueOf(_childMaps[i]->_isCovered));
            
            
            if (allChildrenOccluded && _childMaps[i]->_isCovered) {
                maxChildCoveredDepth = std::max(maxChildCoveredDepth, _childMaps[i]->_coveredDistance);
            } else {
                allChildrenOccluded = false;
            }
        }
        // if all the children are covered, this makes our quad tree "shallower" because it records that
        // entire quad is covered, it uses the "furthest" z-order so that if a shalower polygon comes through
        // we won't assume its occluded
        if (allChildrenOccluded && storeIt) {
            _isCovered = true;
            _coveredDistance = maxChildCoveredDepth;
            //printLog("CoverageMapV2::coverNode()... V2_INTERSECT (allChildrenCovered && storeIt) returning _isCovered=%s \n", debug::valueOf(_isCovered));

            // If this node intersects the polygon, then we want to report either OCCLUDED or V2_INTERSECT
            return ((_isCovered && _coveredDistance < polygon->getDistance()) ? V2_OCCLUDED : V2_INTERSECT); // don't recurse further
        }
        //printLog("CoverageMapV2::coverNode()... V2_INTERSECT returning false \n");

        if (!storeIt) {
            printLog("CoverageMap2::checkMap()... coverageState == V2_INTERSECT  LINE 296 storeIt=FALSE polygonIsCompletelyCovered=%s allChildrenOccluded=%s _isCovered=%s\n",
                    debug::valueOf(polygonIsCompletelyCovered),
                    debug::valueOf(allChildrenOccluded),
                    debug::valueOf(_isCovered));
            _myBoundingBox.printDebugDetails();
        }

        // If we got here, we intersect, but we don't occlude
        return V2_INTERSECT; 
    }

    if (!storeIt) {
        printLog("CoverageMap2::checkMap()... coverageState == V2_INTERSECT  LINE 304 storeIt=FALSE polygonIsCompletelyCovered=%s\n",debug::valueOf(polygonIsCompletelyCovered));
    }


    //printLog("CoverageMapV2::coverNode()... bottom of function   returning false \n");
    
    // not sure we should get here! maybe assert!
    assert(false);
    return coverageState;
}

