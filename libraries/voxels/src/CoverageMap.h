//
//  CoverageMap.h - 2D CoverageMap Quad tree for storage of VoxelProjectedPolygons
//  hifi
//
//  Added by Brad Hefta-Gaub on 06/11/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef _COVERAGE_MAP_
#define _COVERAGE_MAP_

#include <glm/glm.hpp>
#include "VoxelProjectedPolygon.h"

class CoverageMap {

public:
    static const int NUMBER_OF_CHILDREN = 4;
    static const bool NOT_ROOT=false;
    static const bool IS_ROOT=true;
    static const BoundingBox ROOT_BOUNDING_BOX;
    static const float MINIMUM_POLYGON_AREA_TO_STORE;

    CoverageMap(BoundingBox boundingBox = ROOT_BOUNDING_BOX, bool isRoot = IS_ROOT, bool managePolygons = true);
    ~CoverageMap();
    
    typedef enum {STORED, OCCLUDED, DOESNT_FIT, NOT_STORED} StorageResult;
    StorageResult checkMap(VoxelProjectedPolygon* polygon, bool storeIt = true);
    
    BoundingBox getChildBoundingBox(int childIndex);
    
    void erase(); // erase the coverage map

private:
    void init();
    void growPolygonArray();
    void storeInArray(VoxelProjectedPolygon* polygon);

    bool                    _isRoot; // is this map the root, if so, it never returns DOESNT_FIT
    BoundingBox             _myBoundingBox;
    bool                    _managePolygons; // will the coverage map delete the polygons on destruct
    int                     _polygonCount; // how many polygons at this level
    int                     _polygonArraySize; // how much room is there to store polygons at this level
    VoxelProjectedPolygon**  _polygons;
    float*                  _polygonDistances;
    CoverageMap*            _childMaps[NUMBER_OF_CHILDREN];

    static const int DEFAULT_GROW_SIZE = 100;
    static int _mapCount;
    static int _maxPolygonsUsed;
    static int _totalPolygons;

};


#endif // _COVERAGE_MAP_
