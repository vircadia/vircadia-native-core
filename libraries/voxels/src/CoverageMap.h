//
//  CoverageMap.h - 2D CoverageMap Quad tree for storage of VoxelProjectedShadows
//  hifi
//
//  Added by Brad Hefta-Gaub on 06/11/13.
//

#ifndef _COVERAGE_MAP_
#define _COVERAGE_MAP_

#include <glm/glm.hpp>
#include "VoxelProjectedShadow.h"


// 
// depth of polygon is distance from camera to closest point on 3d voxel
//    this works because voxels are axis aligned and therefore other voxels
//    can only be as close as that if they are on the same axis
//
// scanning octree in "closest" order...
// for each node
//      polygon = getShadow(node)
//      isOccluded = coverageMap->isOccluded(polygon)
//      if (!isOccluded)
//          coverageMap->add(polygon)
//          sendList->add(node)
//      else
//          don't need to send or remember
// end each

// adding to coverage map....
//
// at each level of quadtree
//      store a list of polygons whose bounding box fit in the bb of the quad but not a smaller quad
//      also store 4 smaller quad trees
//
// is it true, that only polygons from higher in the quad tree can occlude a polygon?
//      YES - I believe so, because polygon that occluded all the points of the potential occludee
//            would have a bounding box that couldn't fit in a lower level of the quad tree
//
//



class CoverageMap {

public:
    static const int NUMBER_OF_CHILDREN = 4;
    static const bool NOT_ROOT=false;
    static const bool IS_ROOT=true;

    CoverageMap(BoundingBox boundingBox, bool isRoot = IS_ROOT, bool managePolygons = false);
    ~CoverageMap();
    
    typedef enum {STORED, OCCLUDED, DOESNT_FIT, NOT_STORED} StorageResult;
    StorageResult checkMap(VoxelProjectedShadow* polygon, bool storeIt = true);
    
    BoundingBox getChildBoundingBox(int childIndex);

private:
    void init();
    void growPolygonArray();
    void storeInArray(VoxelProjectedShadow* polygon);

    bool                    _isRoot; // is this map the root, if so, it never returns DOESNT_FIT
    BoundingBox             _myBoundingBox;
    bool                    _managePolygons; // will the coverage map delete the polygons on destruct
    int                     _polygonCount; // how many polygons at this level
    int                     _polygonArraySize; // how much room is there to store polygons at this level
    VoxelProjectedShadow**  _polygons;
    float*                  _polygonDistances;
    CoverageMap*            _childMaps[NUMBER_OF_CHILDREN];

    static const int DEFAULT_GROW_SIZE = 100;
    static int _mapCount;
    static int _maxPolygonsUsed;
    static int _totalPolygons;

};


#endif // _COVERAGE_MAP_
