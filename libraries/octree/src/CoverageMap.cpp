//
//  CoverageMap.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 06/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstring>

#include <QtCore/QDebug>

#include <SharedUtil.h>

#include "CoverageMap.h"

int CoverageMap::_mapCount = 0;
int CoverageMap::_checkMapRootCalls = 0;
int CoverageMap::_notAllInView = 0;
bool CoverageMap::wantDebugging = false;

const int MAX_POLYGONS_PER_REGION = 50;

const BoundingBox CoverageMap::ROOT_BOUNDING_BOX = BoundingBox(glm::vec2(-1.f,-1.f), glm::vec2(2.f,2.f));

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
};

CoverageMap::~CoverageMap() {
    erase();
};

void CoverageMap::printStats() {
    qDebug("CoverageMap::printStats()...");
    qDebug("MINIMUM_POLYGON_AREA_TO_STORE=%f",MINIMUM_POLYGON_AREA_TO_STORE);
    qDebug("_mapCount=%d",_mapCount);
    qDebug("_checkMapRootCalls=%d",_checkMapRootCalls);
    qDebug("_notAllInView=%d",_notAllInView);
    qDebug("_maxPolygonsUsed=%d",CoverageRegion::_maxPolygonsUsed);
    qDebug("_totalPolygons=%d",CoverageRegion::_totalPolygons);
    qDebug("_occlusionTests=%d",CoverageRegion::_occlusionTests);
    qDebug("_regionSkips=%d",CoverageRegion::_regionSkips);
    qDebug("_tooSmallSkips=%d",CoverageRegion::_tooSmallSkips);
    qDebug("_regionFullSkips=%d",CoverageRegion::_regionFullSkips);
    qDebug("_outOfOrderPolygon=%d",CoverageRegion::_outOfOrderPolygon);
    qDebug("_clippedPolygons=%d",CoverageRegion::_clippedPolygons);
}

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
        qDebug("CoverageMap last to be deleted...");
        printStats();
        
        CoverageRegion::_maxPolygonsUsed = 0;
        CoverageRegion::_totalPolygons = 0;
        CoverageRegion::_occlusionTests = 0;
        CoverageRegion::_regionSkips = 0;
        CoverageRegion::_tooSmallSkips = 0;
        CoverageRegion::_regionFullSkips = 0;
        CoverageRegion::_outOfOrderPolygon = 0;
        CoverageRegion::_clippedPolygons = 0;
        _mapCount = 0;
        _checkMapRootCalls = 0;
        _notAllInView = 0;
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

int CoverageMap::getPolygonCount() const {
    return (_topHalf.getPolygonCount() +
        _bottomHalf.getPolygonCount() +
        _leftHalf.getPolygonCount() +
        _rightHalf.getPolygonCount() +
        _remainder.getPolygonCount());
}

OctreeProjectedPolygon* CoverageMap::getPolygon(int index) const {
    int base = 0;
    if ((index - base) < _topHalf.getPolygonCount()) {
        return _topHalf.getPolygon((index - base));
    }
    base += _topHalf.getPolygonCount();

    if ((index - base) < _bottomHalf.getPolygonCount()) {
        return _bottomHalf.getPolygon((index - base));
    }
    base += _bottomHalf.getPolygonCount();

    if ((index - base) < _leftHalf.getPolygonCount()) {
        return _leftHalf.getPolygon((index - base));
    }
    base += _leftHalf.getPolygonCount();

    if ((index - base) < _rightHalf.getPolygonCount()) {
        return _rightHalf.getPolygon((index - base));
    }
    base += _rightHalf.getPolygonCount();

    if ((index - base) < _remainder.getPolygonCount()) {
        return _remainder.getPolygon((index - base));
    }
    return NULL;
}



// possible results = STORED/NOT_STORED, OCCLUDED, DOESNT_FIT
CoverageMapStorageResult CoverageMap::checkMap(OctreeProjectedPolygon* polygon, bool storeIt) {

    if (_isRoot) {
        _checkMapRootCalls++;
    }

    // short circuit: we don't handle polygons that aren't all in view, so, if the polygon in question is
    // not in view, then we just discard it with a DOESNT_FIT, this saves us time checking values later.
    if (!polygon->getAllInView()) {
        _notAllInView++;
        return DOESNT_FIT;
    }

    BoundingBox polygonBox(polygon->getBoundingBox()); 
    if (_isRoot || _myBoundingBox.contains(polygonBox)) {
    
        CoverageMapStorageResult result = NOT_STORED;
        CoverageRegion* storeIn = &_remainder;
        
        // Check each half of the box independently
        const bool useRegions = true; // for now we will continue to use regions
        if (useRegions) {
            if (_topHalf.contains(polygonBox)) {
                result = _topHalf.checkRegion(polygon, polygonBox, storeIt);
                storeIn = &_topHalf;
            } else if (_bottomHalf.contains(polygonBox)) {
                result  = _bottomHalf.checkRegion(polygon, polygonBox, storeIt);
                storeIn = &_bottomHalf;
            } else if (_leftHalf.contains(polygonBox)) {
                result  = _leftHalf.checkRegion(polygon, polygonBox, storeIt);
                storeIn = &_leftHalf;
            } else if (_rightHalf.contains(polygonBox)) {
                result  = _rightHalf.checkRegion(polygon, polygonBox, storeIt);
                storeIn = &_rightHalf;
            }
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
        
            /*
            if (result == STORED)
                qDebug("CoverageMap2::checkMap()... STORED\n");
            else
                qDebug("CoverageMap2::checkMap()... OCCLUDED\n");
            */
            
            return result;
        }
        
        // if we made it here, then it means the polygon being stored is not occluded
        // at this level of the quad tree, so we can continue to insert it into the map. 
        // First we check to see if it fits in any of our sub maps
        const bool useChildMaps = true; // for now we will continue to use child maps
        if (useChildMaps) {        
            for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
                BoundingBox childMapBoundingBox = getChildBoundingBox(i);
                if (childMapBoundingBox.contains(polygon->getBoundingBox())) {
                    // if no child map exists yet, then create it
                    if (!_childMaps[i]) {
                        _childMaps[i] = new CoverageMap(childMapBoundingBox, NOT_ROOT, _managePolygons);
                    }
                    result = _childMaps[i]->checkMap(polygon, storeIt);

                    /*
                    switch (result) {
                        case STORED:
                            qDebug("checkMap() = STORED\n");
                            break;
                        case NOT_STORED:
                            qDebug("checkMap() = NOT_STORED\n");
                            break;
                        case OCCLUDED:
                            qDebug("checkMap() = OCCLUDED\n");
                            break;
                        default:
                            qDebug("checkMap() = ????? \n");
                            break;
                    }
                    */
                
                    return result;
                }
            }
        }
        // if we got this far, then the polygon is in our bounding box, but doesn't fit in
        // any of our child bounding boxes, so we should add it here.
        if (storeIt) {
            if (polygon->getBoundingBox().area() > CoverageMap::MINIMUM_POLYGON_AREA_TO_STORE) {
                if (storeIn->getPolygonCount() < MAX_POLYGONS_PER_REGION) {
                    storeIn->storeInArray(polygon);
                    return STORED;
                } else {
                    CoverageRegion::_regionFullSkips++;
                    return NOT_STORED;
                }
            } else {
                CoverageRegion::_tooSmallSkips++;
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
    _polygonSizes = NULL;
}


void CoverageRegion::erase() {

/**
    if (_polygonCount) {
        qDebug("CoverageRegion::erase()...\n");
        qDebug("_polygonCount=%d\n",_polygonCount);
        _myBoundingBox.printDebugDetails(getRegionName());
        //for (int i = 0; i < _polygonCount; i++) {
        //    qDebug("_polygons[%d]=",i);
        //    _polygons[i]->getBoundingBox().printDebugDetails();
        //}
    }
**/
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
    if (_polygonSizes) {
        delete[] _polygonSizes;
        _polygonSizes = NULL;
    }
}

void CoverageRegion::growPolygonArray() {
    OctreeProjectedPolygon** newPolygons  = new OctreeProjectedPolygon*[_polygonArraySize + DEFAULT_GROW_SIZE];
    float*                  newDistances = new float[_polygonArraySize + DEFAULT_GROW_SIZE];
    float*                  newSizes     = new float[_polygonArraySize + DEFAULT_GROW_SIZE];


    if (_polygons) {
        memcpy(newPolygons, _polygons, sizeof(OctreeProjectedPolygon*) * _polygonCount);
        delete[] _polygons;
        memcpy(newDistances, _polygonDistances, sizeof(float) * _polygonCount);
        delete[] _polygonDistances;
        memcpy(newSizes, _polygonSizes, sizeof(float) * _polygonCount);
        delete[] _polygonSizes;
    }
    _polygons         = newPolygons;
    _polygonDistances = newDistances;
    _polygonSizes     = newSizes;
    _polygonArraySize = _polygonArraySize + DEFAULT_GROW_SIZE;
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
int CoverageRegion::_regionSkips = 0;
int CoverageRegion::_tooSmallSkips = 0;
int CoverageRegion::_regionFullSkips = 0;
int CoverageRegion::_outOfOrderPolygon = 0;
int CoverageRegion::_clippedPolygons = 0;


bool CoverageRegion::mergeItemsInArray(OctreeProjectedPolygon* seed, bool seedInArray) {
    for (int i = 0; i < _polygonCount; i++) {
        OctreeProjectedPolygon* otherPolygon = _polygons[i];
        if (otherPolygon->canMerge(*seed)) {
            otherPolygon->merge(*seed);

            if (seedInArray) {
                int* IGNORED_ADDRESS = NULL;
                // remove this otherOtherPolygon for our polygon array
                _polygonCount = removeFromSortedArrays((void*)seed,
                                                       (void**)_polygons, _polygonDistances, IGNORED_ADDRESS,
                                                       _polygonCount, _polygonArraySize);
                _totalPolygons--;
            }
        
            // clean up
            if (_managePolygons) {
                delete seed;
            }
            
            // Now run again using our newly merged polygon as the seed
            mergeItemsInArray(otherPolygon, true);
            
            return true;
        }
    }
    return false;
}

// just handles storage in the array, doesn't test for occlusion or
// determining if this is the correct map to store in!
void CoverageRegion::storeInArray(OctreeProjectedPolygon* polygon) {

    _currentCoveredBounds.explandToInclude(polygon->getBoundingBox());
    
    
    // Before we actually store this polygon in the array, check to see if this polygon can be merged to any of the existing
    // polygons already in our array.
    if (mergeItemsInArray(polygon, false)) {
        return; // exit early
    }
    
    // only after we attempt to merge!
    _totalPolygons++;

    if (_polygonArraySize < _polygonCount + 1) {
        growPolygonArray();
    }

    // As an experiment we're going to see if we get an improvement by storing the polygons in coverage area sorted order
    // this means the bigger polygons are earlier in the array. We should have a higher probability of being occluded earlier
    // in the list. We still check to see if the polygon is "in front" of the target polygon before we test occlusion. Since
    // sometimes things come out of order.
    const bool SORT_BY_SIZE = false;
    const int  IGNORED = 0;
    int* IGNORED_ADDRESS = NULL;
    if (SORT_BY_SIZE) {
        // This old code assumes that polygons will always be added in z-buffer order, but that doesn't seem to
        // be a good assumption. So instead, we will need to sort this by distance. Use a binary search to find the
        // insertion point in this array, and shift the array accordingly
        float area = polygon->getBoundingBox().area();
        float reverseArea = 4.0f - area;
        _polygonCount = insertIntoSortedArrays((void*)polygon, reverseArea, IGNORED,
                                               (void**)_polygons, _polygonSizes, IGNORED_ADDRESS,
                                               _polygonCount, _polygonArraySize);
    } else {
        _polygonCount = insertIntoSortedArrays((void*)polygon, polygon->getDistance(), IGNORED,
                                               (void**)_polygons, _polygonDistances, IGNORED_ADDRESS,
                                               _polygonCount, _polygonArraySize);
    }
                                           
    // Debugging and Optimization Tuning code.
    if (_polygonCount > _maxPolygonsUsed) {
        _maxPolygonsUsed = _polygonCount;
    }
}



CoverageMapStorageResult CoverageRegion::checkRegion(OctreeProjectedPolygon* polygon, const BoundingBox& polygonBox, bool storeIt) {

    CoverageMapStorageResult result = DOESNT_FIT;

    if (_isRoot || _myBoundingBox.contains(polygonBox)) {
        result = NOT_STORED; // if we got here, then we DO fit...
        
        // only actually check the polygons if this polygon is in the covered bounds for this region
        if (!_currentCoveredBounds.contains(polygonBox)) {
            _regionSkips += _polygonCount;
        } else {
            // check to make sure this polygon isn't occluded by something at this level
            for (int i = 0; i < _polygonCount; i++) {
                OctreeProjectedPolygon* polygonAtThisLevel = _polygons[i];

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
                                if (getPolygonCount() < MAX_POLYGONS_PER_REGION) {
                                    storeInArray(polygon);
                                    return STORED;
                                } else {
                                    CoverageRegion::_regionFullSkips++;
                                    return NOT_STORED;
                                }
                            } else {
                                _tooSmallSkips++;
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
    }
    return result;
}