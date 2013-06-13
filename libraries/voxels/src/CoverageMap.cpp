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

CoverageMap::CoverageMap(BoundingBox boundingBox, bool isRoot, bool managePolygons) : 
    _isRoot(isRoot), _myBoundingBox(boundingBox), _managePolygons(managePolygons) { 
    _mapCount++;
    init(); 
    printLog("CoverageMap created... _mapCount=%d\n",_mapCount);
};

CoverageMap::~CoverageMap() {
    if (_managePolygons) {
        for (int i = 0; i < _polygonCount; i++) {
            delete _polygons[i];
        }
    }
    if (_polygons) {
        delete[] _polygons;
    }
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (_childMaps[i]) {
            delete _childMaps[i];
        }
    }
    
    if (_isRoot) {
        printLog("CoverageMap last to be deleted...\n");
         printLog("_mapCount=%d\n",_mapCount);
         printLog("_maxPolygonsUsed=%d\n",_maxPolygonsUsed);
         printLog("_totalPolygons=%d\n",_totalPolygons);

        _maxPolygonsUsed = 0;
        _totalPolygons = 0;
        _mapCount = 0;
    }
};


void CoverageMap::init() {
    _polygonCount = 0;
    _polygonArraySize = 0;
    _polygons = NULL;
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
    VoxelProjectedShadow** newPolygons  = new VoxelProjectedShadow*[_polygonArraySize + DEFAULT_GROW_SIZE];
    float*                 newDistances = new float[_polygonArraySize + DEFAULT_GROW_SIZE];


    if (_polygons) {
        memcpy(newPolygons, _polygons, sizeof(VoxelProjectedShadow*) * _polygonCount);
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
void CoverageMap::storeInArray(VoxelProjectedShadow* polygon) {

    _totalPolygons++;

//printLog("CoverageMap::storeInArray()...");
//polygon->printDebugDetails();

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
        printLog("CoverageMap new _maxPolygonsUsed reached=%d\n",_maxPolygonsUsed);
        _myBoundingBox.printDebugDetails("map._myBoundingBox");
    }
}


// possible results = STORED, OCCLUDED, DOESNT_FIT
// storeInMap(poly)
//      if (poly->BB inside myBB)
//          -- at this point, we can check polygons at this level
//          -- to see if they occlude the polygon being inserted
//          -- since the polygons at this level are big relative to
//          -- lower levels, there's a higher chance for them to occlude
//          -- Note: list should be "depth" sorted
//          for each polygon at this level
//              if (levelpolygon.occludes(poly))
//                  return OCCLUDED
//          end
//          for each child
//              childResult = child->storeInMap(poly)
//              if (childResult == STORED || childResult == OCCLUDED)
//                  return childResult
//              end
//          end
//          -- if we got here, then the polygon didn't fit in any of the children and
//          -- wasn't already occluded, so store it here
//          insert into local list (poly, poly.depth)
//          return STORED
//      end
//      return DOESNT_FIT
CoverageMap::StorageResult CoverageMap::storeInMap(VoxelProjectedShadow* polygon) {

//printLog("CoverageMap::storeInMap()...");
//polygon->printDebugDetails();

    if (_isRoot || _myBoundingBox.contains(polygon->getBoundingBox())) {

/**
if (_isRoot) {
    printLog("CoverageMap::storeInMap()... this map _isRoot, so all polygons are contained....\n");
} else {
    printLog("CoverageMap::storeInMap()... _myBoundingBox.contains(polygon)....\n");
    _myBoundingBox.printDebugDetails("_myBoundingBox");
    polygon->getBoundingBox().printDebugDetails("polygon->getBoundingBox()");
}
**/

        // check to make sure this polygon isn't occluded by something at this level
        for (int i = 0; i < _polygonCount; i++) {
            VoxelProjectedShadow* polygonAtThisLevel = _polygons[i];

//printLog("CoverageMap::storeInMap()... polygonAtThisLevel = _polygons[%d] =",i);
//polygonAtThisLevel->printDebugDetails();
            
            // Check to make sure that the polygon in question is "behind" the polygon in the list
            // otherwise, we don't need to test it's occlusion (although, it means we've potentially
            // added an item previously that may be occluded??? Is that possible? Maybe not, because two
            // voxels can't have the exact same outline. So one occludes the other, they can't both occlude
            // each other.
            if (polygonAtThisLevel->occludes(*polygon)) {

//printLog("CoverageMap::storeInMap()... polygonAtThisLevel->occludes(*polygon)...\n",i);

                // if the polygonAtThisLevel is actually behind the one we're inserting, then we don't
                // want to report our inserted one as occluded, but we do want to add our inserted one.
                if (polygonAtThisLevel->getDistance() >= polygon->getDistance()) {
                    storeInArray(polygon);
                    return STORED;
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
                return _childMaps[i]->storeInMap(polygon);
            }
        }
        // if we got this far, then the polygon is in our bounding box, but doesn't fit in
        // any of our child bounding boxes, so we should add it here.
        storeInArray(polygon);
        return STORED;
    }
    return DOESNT_FIT;
}

