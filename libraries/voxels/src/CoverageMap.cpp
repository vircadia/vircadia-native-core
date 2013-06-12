//
//  CoverageMap.cpp - 
//  hifi
//
//  Added by Brad Hefta-Gaub on 06/11/13.
//

#include "CoverageMap.h"
#include <string>

CoverageMap::~CoverageMap() {
    if (_polygons) {
        delete[] _polygons;
    }
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        if (_childMaps[i]) {
            delete _childMaps[i];
        }
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
    VoxelProjectedShadow** newPolygons = new VoxelProjectedShadow*[_polygonArraySize + DEFAULT_GROW_SIZE];

    if (_polygons) {
        memcpy(newPolygons, _polygons, sizeof(VoxelProjectedShadow*) * _polygonCount);
        delete[] _polygons;
    }
    _polygons = newPolygons;
    _polygonArraySize = _polygonArraySize + DEFAULT_GROW_SIZE;
}

// just handles storage in the array, doesn't test for occlusion or
// determining if this is the correct map to store in!
void CoverageMap::storeInArray(VoxelProjectedShadow* polygon) {
    if (_polygonArraySize < _polygonCount + 1) {
        growPolygonArray();
    }
    _polygons[_polygonCount] = polygon;
    _polygonCount++;
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
    if (_myBoundingBox.contains(polygon->getBoundingBox())) {
        // check to make sure this polygon isn't occluded by something at this level
        for (int i = 0; i < _polygonCount; i++) {
            VoxelProjectedShadow* polygonAtThisLevel = _polygons[i];
            if (polygonAtThisLevel->occludes(*polygon)) {
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
                    _childMaps[i] = new CoverageMap(childMapBoundingBox);
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

