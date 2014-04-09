//
//  CoverageMapV2.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 06/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CoverageMapV2_h
#define hifi_CoverageMapV2_h

#include <glm/glm.hpp>

#include "OctreeProjectedPolygon.h"

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
    
    CoverageMapV2StorageResult checkMap(const OctreeProjectedPolygon* polygon, bool storeIt = true);
    
    BoundingBox getChildBoundingBox(int childIndex);
    const BoundingBox& getBoundingBox() const { return _myBoundingBox; };
    CoverageMapV2* getChild(int childIndex) const { return _childMaps[childIndex]; };
    bool isCovered() const { return _isCovered; };
    
    void erase(); // erase the coverage map

    void render();
    

private:
    void recurseMap(const OctreeProjectedPolygon* polygon, bool storeIt, 
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


#endif // hifi_CoverageMapV2_h
