//
//  CoverageMap.cpp - 
//  hifi
//
//  Added by Brad Hefta-Gaub on 06/11/13.
//

#include "CoverageMap.h"
#include <SharedUtil.h>
#include <string>
#include "Log.h"

int CoverageMap::_mapCount = 0;
const BoundingBox CoverageMap::ROOT_BOUNDING_BOX = BoundingBox(glm::vec2(-2.f,-2.f), glm::vec2(4.f,4.f));

CoverageMap::CoverageMap(BoundingBox boundingBox, bool isRoot, bool managePolygons) : 
    _isRoot(isRoot), _myBoundingBox(boundingBox), _managePolygons(managePolygons) { 
    _mapCount++;
    init(); 
    //printLog("CoverageMap created... _mapCount=%d\n",_mapCount);
};

CoverageMap::~CoverageMap() {
    erase();
};

void CoverageMap::erase() {
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
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (_childMaps[i]) {
            delete _childMaps[i];
            _childMaps[i] = NULL;
        }
    }

/**
    if (_isRoot) {
        printLog("CoverageMap last to be deleted...\n");
        printLog("_mapCount=%d\n",_mapCount);
        printLog("_maxPolygonsUsed=%d\n",_maxPolygonsUsed);
        printLog("_totalPolygons=%d\n",_totalPolygons);

        _maxPolygonsUsed = 0;
        _totalPolygons = 0;
        _mapCount = 0;
    }
**/

}

void CoverageMap::init() {
    _polygonCount = 0;
    _polygonArraySize = 0;
    _polygons = NULL;
    _polygonDistances = NULL;
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


void CoverageMap::growPolygonArray() {
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

int CoverageMap::_maxPolygonsUsed = 0;
int CoverageMap::_totalPolygons = 0;

// just handles storage in the array, doesn't test for occlusion or
// determining if this is the correct map to store in!
void CoverageMap::storeInArray(VoxelProjectedPolygon* polygon) {

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

    if (_polygonCount > _maxPolygonsUsed) {
        _maxPolygonsUsed = _polygonCount;
        //printLog("CoverageMap new _maxPolygonsUsed reached=%d\n",_maxPolygonsUsed);
        //_myBoundingBox.printDebugDetails("map._myBoundingBox");
    }
}


// possible results = STORED/NOT_STORED, OCCLUDED, DOESNT_FIT
CoverageMap::StorageResult CoverageMap::checkMap(VoxelProjectedPolygon* polygon, bool storeIt) {
    if (_isRoot || _myBoundingBox.contains(polygon->getBoundingBox())) {
        // check to make sure this polygon isn't occluded by something at this level
        for (int i = 0; i < _polygonCount; i++) {
            VoxelProjectedPolygon* polygonAtThisLevel = _polygons[i];
            // Check to make sure that the polygon in question is "behind" the polygon in the list
            // otherwise, we don't need to test it's occlusion (although, it means we've potentially
            // added an item previously that may be occluded??? Is that possible? Maybe not, because two
            // voxels can't have the exact same outline. So one occludes the other, they can't both occlude
            // each other.
            if (polygonAtThisLevel->occludes(*polygon)) {
                // if the polygonAtThisLevel is actually behind the one we're inserting, then we don't
                // want to report our inserted one as occluded, but we do want to add our inserted one.
                if (polygonAtThisLevel->getDistance() >= polygon->getDistance()) {
                    if (storeIt) {
                        storeInArray(polygon);
                        return STORED;
                    } else {
                        return NOT_STORED;
                    }
                }
                // this polygon is occluded by a closer polygon, so don't store it, and let the caller know
                return OCCLUDED;
            }
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
            storeInArray(polygon);
            return STORED;
        } else {
            return NOT_STORED;
        }
    }
    return DOESNT_FIT;
}
