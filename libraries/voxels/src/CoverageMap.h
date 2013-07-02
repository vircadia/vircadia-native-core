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

typedef enum {STORED, OCCLUDED, DOESNT_FIT, NOT_STORED} CoverageMapStorageResult;
typedef enum {TOP_HALF, BOTTOM_HALF, LEFT_HALF, RIGHT_HALF, REMAINDER} RegionName;

class CoverageRegion {

public:
    
    CoverageRegion(BoundingBox boundingBox, bool isRoot, bool managePolygons = true, RegionName regionName = REMAINDER);
    ~CoverageRegion();

    CoverageMapStorageResult checkRegion(VoxelProjectedPolygon* polygon, const BoundingBox& polygonBox, bool storeIt);
    void storeInArray(VoxelProjectedPolygon* polygon);
    
    bool contains(const BoundingBox& box) const { return _myBoundingBox.contains(box); };
    void erase(); // erase the coverage region

    static int _maxPolygonsUsed;
    static int _totalPolygons;
    static int _occlusionTests;
    static int _regionSkips;
    static int _tooSmallSkips;
    static int _outOfOrderPolygon;
    static int _clippedPolygons;


    const char* getRegionName() const;
    
    int getPolygonCount() const { return _polygonCount; };
    VoxelProjectedPolygon* getPolygon(int index) const { return _polygons[index]; };

private:
    void init();

    bool                    _isRoot; // is this map the root, if so, it never returns DOESNT_FIT
    BoundingBox             _myBoundingBox;
    BoundingBox             _currentCoveredBounds; // area in this region currently covered by some polygon
    bool                    _managePolygons; // will the coverage map delete the polygons on destruct
    RegionName              _regionName;
    int                     _polygonCount; // how many polygons at this level
    int                     _polygonArraySize; // how much room is there to store polygons at this level
    VoxelProjectedPolygon**  _polygons;
    
    // we will use one or the other of these depending on settings in the code.
    float*                  _polygonDistances;
    float*                  _polygonSizes;
    void growPolygonArray();
    static const int DEFAULT_GROW_SIZE = 100;
    
    bool mergeItemsInArray(VoxelProjectedPolygon* seed, bool seedInArray);
    
};

class CoverageMap {

public:
    static const int NUMBER_OF_CHILDREN = 4;
    static const bool NOT_ROOT=false;
    static const bool IS_ROOT=true;
    static const BoundingBox ROOT_BOUNDING_BOX;
    static const float MINIMUM_POLYGON_AREA_TO_STORE;

    CoverageMap(BoundingBox boundingBox = ROOT_BOUNDING_BOX, bool isRoot = IS_ROOT, bool managePolygons = true);
    ~CoverageMap();
    
    CoverageMapStorageResult checkMap(VoxelProjectedPolygon* polygon, bool storeIt = true);
    
    BoundingBox getChildBoundingBox(int childIndex);
    
    void erase(); // erase the coverage map

    static bool wantDebugging;

    int getPolygonCount() const;
    VoxelProjectedPolygon* getPolygon(int index) const;
    CoverageMap* getChild(int childIndex) const { return _childMaps[childIndex]; };
    
private:
    void init();

    bool                    _isRoot; // is this map the root, if so, it never returns DOESNT_FIT
    BoundingBox             _myBoundingBox;
    CoverageMap*            _childMaps[NUMBER_OF_CHILDREN];
    bool                    _managePolygons; // will the coverage map delete the polygons on destruct
    
    // We divide the map into 5 regions representing each possible half of the map, and the whole map
    // this allows us to keep the list of polygons shorter
    CoverageRegion          _topHalf;
    CoverageRegion          _bottomHalf;
    CoverageRegion          _leftHalf;
    CoverageRegion          _rightHalf;
    CoverageRegion          _remainder;

    static int  _mapCount;
    static int  _checkMapRootCalls;
    static int  _notAllInView;
};


#endif // _COVERAGE_MAP_
