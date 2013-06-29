//
//  CoverageMapV2.h - 2D CoverageMapV2 Quad tree for storage of VoxelProjectedPolygons
//  hifi
//
//  Added by Brad Hefta-Gaub on 06/11/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef _COVERAGE_MAP_V2_
#define _COVERAGE_MAP_V2_

#include <glm/glm.hpp>

#include "VoxelProjectedPolygon.h"

typedef enum {
    V2_DOESNT_FIT, V2_STORED, V2_NOT_STORED, 
    V2_INTERSECT, V2_NO_INTERSECT, 
    V2_OCCLUDED, V2_NOT_OCCLUDED
} CoverageMapV2StorageResult;

class CoverageMapV2 {

public:
    static const int NUMBER_OF_CHILDREN = 4;
    static const bool NOT_ROOT = false;
    static const bool IS_ROOT = true;
    static const BoundingBox ROOT_BOUNDING_BOX;
    static const float MINIMUM_POLYGON_AREA_TO_STORE;
    static const float NOT_COVERED;
    static const float MINIMUM_OCCLUSION_CHECK_AREA;
    static bool wantDebugging;
    
    CoverageMapV2(BoundingBox boundingBox = ROOT_BOUNDING_BOX, bool isRoot = IS_ROOT,
                  bool isCovered = false, float coverageDistance = NOT_COVERED);
    ~CoverageMapV2();
    
    CoverageMapV2StorageResult checkMap(const VoxelProjectedPolygon* polygon, bool storeIt = true);
    
    BoundingBox getChildBoundingBox(int childIndex);
    const BoundingBox& getBoundingBox() const { return _myBoundingBox; };
    CoverageMapV2* getChild(int childIndex) const { return _childMaps[childIndex]; };
    bool isCovered() const { return _isCovered; };
    
    void erase(); // erase the coverage map

    void render();
    

private:
    void recurseMap(const VoxelProjectedPolygon* polygon, bool storeIt, 
                    bool& seenOccludedMapNodes, bool& allOccludedMapNodesCovered);

    void init();

    bool                    _isRoot;
    BoundingBox             _myBoundingBox;
    CoverageMapV2*          _childMaps[NUMBER_OF_CHILDREN];

    bool                    _isCovered;
    float                   _coveredDistance;
    
    static int  _mapCount;
    static int  _checkMapRootCalls;
    static int  _notAllInView;
};


#endif // _COVERAGE_MAP_V2_
