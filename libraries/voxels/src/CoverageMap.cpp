//
//  CoverageMap.cpp - 
//  hifi
//
//  Added by Brad Hefta-Gaub on 06/11/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "CoverageMap.h"
#include <SharedUtil.h>
#include <cstring>
#include "Log.h"

int CoverageMap::_mapCount = 0;
int CoverageMap::_checkMapRootCalls = 0;
bool CoverageMap::wantDebugging = false;

const BoundingBox CoverageMap::ROOT_BOUNDING_BOX = BoundingBox(glm::vec2(-2.f,-2.f), glm::vec2(4.f,4.f));

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
const float CoverageMap::MINIMUM_POLYGON_AREA_TO_STORE = (TYPICAL_SCREEN_PIXEL_WIDTH * MINIMUM_POLYGON_AREA_SIDE_IN_PIXELS) *
                                                         (TYPICAL_SCREEN_PIXEL_WIDTH * MINIMUM_POLYGON_AREA_SIDE_IN_PIXELS);

CoverageMap::CoverageMap(BoundingBox boundingBox, bool isRoot, bool managePolygons) : 
    _isRoot(isRoot), 
    _myBoundingBox(boundingBox), 
    _managePolygons(managePolygons),
    _topHalf    (boundingBox.topHalf()   , false,  managePolygons, TOP_HALF    ),
    _bottomHalf (boundingBox.bottomHalf(), false,  managePolygons, BOTTOM_HALF ),
    _leftHalf   (boundingBox.leftHalf()  , false,  managePolygons, LEFT_HALF   ),
    _rightHalf  (boundingBox.rightHalf() , false,  managePolygons, RIGHT_HALF  ),
    _remainder  (boundingBox,              isRoot, managePolygons, REMAINDER   )
{ 
    _mapCount++;
    init(); 
    //printLog("CoverageMap created... _mapCount=%d\n",_mapCount);
};

CoverageMap::~CoverageMap() {
    erase();
};

void CoverageMap::erase() {
    // tell our regions to erase()
    _topHalf.erase();
    _bottomHalf.erase();
    _leftHalf.erase();
    _rightHalf.erase();
    _remainder.erase();

    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (_childMaps[i]) {
            delete _childMaps[i];
            _childMaps[i] = NULL;
        }
    }

    if (_isRoot && wantDebugging) {
        printLog("CoverageMap last to be deleted...\n");
        printLog("MINIMUM_POLYGON_AREA_TO_STORE=%f\n",MINIMUM_POLYGON_AREA_TO_STORE);
        printLog("_mapCount=%d\n",_mapCount);
        printLog("_checkMapRootCalls=%d\n",_checkMapRootCalls);
        printLog("_maxPolygonsUsed=%d\n",CoverageRegion::_maxPolygonsUsed);
        printLog("_totalPolygons=%d\n",CoverageRegion::_totalPolygons);
        printLog("_occlusionTests=%d\n",CoverageRegion::_occlusionTests);
        printLog("_outOfOrderPolygon=%d\n",CoverageRegion::_outOfOrderPolygon);
        CoverageRegion::_maxPolygonsUsed = 0;
        CoverageRegion::_totalPolygons = 0;
        CoverageRegion::_occlusionTests = 0;
        CoverageRegion::_outOfOrderPolygon = 0;
        _mapCount = 0;
        _checkMapRootCalls = 0;
    }
}

void CoverageMap::init() {
    memset(_childMaps,0,sizeof(_childMaps));
}

// 0 = bottom, right
// 1 = bottom, left
// 2 = top, right
// 3 = top, left
BoundingBox CoverageMap::getChildBoundingBox(int childIndex) {
    const int LEFT_BIT = 1;
    const int TOP_BIT  = 2;
    // initialize to our corner, and half our size
    BoundingBox result(_myBoundingBox.corner,_myBoundingBox.size/2.0f);
    // if our "left" bit is set, then add size.x to the corner
    if ((childIndex & LEFT_BIT) == LEFT_BIT) {
        result.corner.x += result.size.x;
    }
    // if our "top" bit is set, then add size.y to the corner
    if ((childIndex & TOP_BIT) == TOP_BIT) {
        result.corner.y += result.size.y;
    }
    return result;
}

// possible results = STORED/NOT_STORED, OCCLUDED, DOESNT_FIT
CoverageMapStorageResult CoverageMap::checkMap(VoxelProjectedPolygon* polygon, bool storeIt) {

    if (_isRoot) {
        _checkMapRootCalls++;
    }

    // short circuit: we don't handle polygons that aren't all in view, so, if the polygon in question is
    // not in view, then we just discard it with a DOESNT_FIT, this saves us time checking values later.
    if (!polygon->getAllInView()) {
        return DOESNT_FIT;
    }

    BoundingBox polygonBox(polygon->getBoundingBox()); 
    if (_isRoot || _myBoundingBox.contains(polygonBox)) {
    
        CoverageMapStorageResult result = NOT_STORED;
        CoverageRegion* storeIn = &_remainder;
        bool fitsInAHalf = false;
        
        // Check each half of the box independently
        if (_topHalf.contains(polygonBox)) {
            result = _topHalf.checkRegion(polygon, polygonBox, storeIt);
            storeIn = &_topHalf;
            fitsInAHalf = true;
        } else if (_bottomHalf.contains(polygonBox)) {
            result  = _bottomHalf.checkRegion(polygon, polygonBox, storeIt);
            storeIn = &_bottomHalf;
            fitsInAHalf = true;
        } else if (_leftHalf.contains(polygonBox)) {
            result  = _leftHalf.checkRegion(polygon, polygonBox, storeIt);
            storeIn = &_leftHalf;
            fitsInAHalf = true;
        } else if (_rightHalf.contains(polygonBox)) {
            result  = _rightHalf.checkRegion(polygon, polygonBox, storeIt);
            storeIn = &_rightHalf;
            fitsInAHalf = true;
        }
        
        // if we got this far, there are one of two possibilities, either a polygon doesn't fit
        // in one of the halves, or it did fit, but it wasn't occluded by anything only in that
        // half. In either of these cases, we want to check our remainder region to see if its
        // occluded by anything there
        if (!(result == STORED || result == OCCLUDED)) {
            result  = _remainder.checkRegion(polygon, polygonBox, storeIt);
        }

        // It's possible that this first set of checks might have resulted in an out of order polygon
        // in which case we just return..
        if (result == STORED || result == OCCLUDED) {
            return result;
        }
        
        // if we made it here, then it means the polygon being stored is not occluded
        // at this level of the quad tree, so we can continue to insert it into the map. 
        // First we check to see if it fits in any of our sub maps
        for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
            BoundingBox childMapBoundingBox = getChildBoundingBox(i);
            if (childMapBoundingBox.contains(polygon->getBoundingBox())) {
                // if no child map exists yet, then create it
                if (!_childMaps[i]) {
                    _childMaps[i] = new CoverageMap(childMapBoundingBox, NOT_ROOT, _managePolygons);
                }
                return _childMaps[i]->checkMap(polygon, storeIt);
            }
        }
        // if we got this far, then the polygon is in our bounding box, but doesn't fit in
        // any of our child bounding boxes, so we should add it here.
        if (storeIt) {
            if (polygon->getBoundingBox().area() > CoverageMap::MINIMUM_POLYGON_AREA_TO_STORE) {
                //printLog("storing polygon of area: %f\n",polygon->getBoundingBox().area());                    
                storeIn->storeInArray(polygon);
                return STORED;
            } else {
                return NOT_STORED;
            }
        } else {
            return NOT_STORED;
        }
    }
    return DOESNT_FIT;
}


CoverageRegion::CoverageRegion(BoundingBox boundingBox, bool isRoot, bool managePolygons, RegionName regionName) :
    _isRoot(isRoot),
    _myBoundingBox(boundingBox), 
    _managePolygons(managePolygons),
    _regionName(regionName)
{ 
    init(); 
};

CoverageRegion::~CoverageRegion() {
    erase();
};

void CoverageRegion::init() {
    _polygonCount = 0;
    _polygonArraySize = 0;
    _polygons = NULL;
    _polygonDistances = NULL;
}


void CoverageRegion::erase() {

/*
    if (_polygonCount) {
        printLog("CoverageRegion::erase()...\n");
        printLog("_polygonCount=%d\n",_polygonCount);
        _myBoundingBox.printDebugDetails(getRegionName());
        //for (int i = 0; i < _polygonCount; i++) {
        //    printLog("_polygons[%d]=",i);
        //    _polygons[i]->getBoundingBox().printDebugDetails();
        //}
    }
*/
    // If we're in charge of managing the polygons, then clean them up first
    if (_managePolygons) {
        for (int i = 0; i < _polygonCount; i++) {
            delete _polygons[i];
            _polygons[i] = NULL; // do we need to do this?
        }
    }
    
    // Now, clean up our local storage
    _polygonCount = 0;
    _polygonArraySize = 0;
    if (_polygons) {
        delete[] _polygons;
        _polygons = NULL;
    }
    if (_polygonDistances) {
        delete[] _polygonDistances;
        _polygonDistances = NULL;
    }
}

void CoverageRegion::growPolygonArray() {
    VoxelProjectedPolygon** newPolygons  = new VoxelProjectedPolygon*[_polygonArraySize + DEFAULT_GROW_SIZE];
    float*                 newDistances = new float[_polygonArraySize + DEFAULT_GROW_SIZE];


    if (_polygons) {
        memcpy(newPolygons, _polygons, sizeof(VoxelProjectedPolygon*) * _polygonCount);
        delete[] _polygons;
        memcpy(newDistances, _polygonDistances, sizeof(float) * _polygonCount);
        delete[] _polygonDistances;
    }
    _polygons = newPolygons;
    _polygonDistances = newDistances;
    _polygonArraySize = _polygonArraySize + DEFAULT_GROW_SIZE;
    //printLog("CoverageMap::growPolygonArray() _polygonArraySize=%d...\n",_polygonArraySize);
}

const char* CoverageRegion::getRegionName() const {
    switch (_regionName) {
        case TOP_HALF: 
            return "TOP_HALF";
        case BOTTOM_HALF: 
            return "BOTTOM_HALF";
        case LEFT_HALF: 
            return "LEFT_HALF";
        case RIGHT_HALF: 
            return "RIGHT_HALF";
        default:
        case REMAINDER: 
            return "REMAINDER";
    }
    return "REMAINDER";
}

int CoverageRegion::_maxPolygonsUsed = 0;
int CoverageRegion::_totalPolygons = 0;
int CoverageRegion::_occlusionTests = 0;
int CoverageRegion::_outOfOrderPolygon = 0;

// just handles storage in the array, doesn't test for occlusion or
// determining if this is the correct map to store in!
void CoverageRegion::storeInArray(VoxelProjectedPolygon* polygon) {

    _totalPolygons++;

    if (_polygonArraySize < _polygonCount + 1) {
        growPolygonArray();
    }

    // This old code assumes that polygons will always be added in z-buffer order, but that doesn't seem to
    // be a good assumption. So instead, we will need to sort this by distance. Use a binary search to find the
    // insertion point in this array, and shift the array accordingly
    const int IGNORED = NULL;
    _polygonCount = insertIntoSortedArrays((void*)polygon, polygon->getDistance(), IGNORED,
                                           (void**)_polygons, _polygonDistances, IGNORED,
                                           _polygonCount, _polygonArraySize);

    // Debugging and Optimization Tuning code.
    if (_polygonCount > _maxPolygonsUsed) {
        _maxPolygonsUsed = _polygonCount;
        //printLog("CoverageRegion new _maxPolygonsUsed reached=%d region=%s\n",_maxPolygonsUsed, getRegionName());
        //_myBoundingBox.printDebugDetails("map._myBoundingBox");
    } else {
        //printLog("CoverageRegion::storeInArray() _polygonCount=%d region=%s\n",_polygonCount, getRegionName());
    }
}



CoverageMapStorageResult CoverageRegion::checkRegion(VoxelProjectedPolygon* polygon, const BoundingBox& polygonBox, bool storeIt) {

    CoverageMapStorageResult result = DOESNT_FIT;

    if (_isRoot || _myBoundingBox.contains(polygonBox)) {
        result = NOT_STORED; // if we got here, then we DO fit...

        // check to make sure this polygon isn't occluded by something at this level
        for (int i = 0; i < _polygonCount; i++) {
            VoxelProjectedPolygon* polygonAtThisLevel = _polygons[i];
            // Check to make sure that the polygon in question is "behind" the polygon in the list
            // otherwise, we don't need to test it's occlusion (although, it means we've potentially
            // added an item previously that may be occluded??? Is that possible? Maybe not, because two
            // voxels can't have the exact same outline. So one occludes the other, they can't both occlude
            // each other.
        
        
            _occlusionTests++;
            if (polygonAtThisLevel->occludes(*polygon)) {
                // if the polygonAtThisLevel is actually behind the one we're inserting, then we don't
                // want to report our inserted one as occluded, but we do want to add our inserted one.
                if (polygonAtThisLevel->getDistance() >= polygon->getDistance()) {
                    _outOfOrderPolygon++;
                    if (storeIt) {
                        if (polygon->getBoundingBox().area() > CoverageMap::MINIMUM_POLYGON_AREA_TO_STORE) {
                            //printLog("storing polygon of area: %f\n",polygon->getBoundingBox().area());                    
                            storeInArray(polygon);
                            return STORED;
                        } else {
                            return NOT_STORED;
                        }
                    } else {
                        return NOT_STORED;
                    }
                }
                // this polygon is occluded by a closer polygon, so don't store it, and let the caller know
                return OCCLUDED;
            }
        }
    }
    return result;
}
