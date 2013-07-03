//
//  VoxelProjectedPolygon.h - The projected shadow (on the 2D view plane) for a voxel
//  hifi
//
//  Added by Brad Hefta-Gaub on 06/11/13.
//

#ifndef _VOXEL_PROJECTED_SHADOW_
#define _VOXEL_PROJECTED_SHADOW_

#include <glm/glm.hpp>

// there's a max of 6 vertices of a project polygon, and a max of twice that when clipped to the screen
const int MAX_PROJECTED_POLYGON_VERTEX_COUNT = 6; 
const int MAX_CLIPPED_PROJECTED_POLYGON_VERTEX_COUNT = MAX_PROJECTED_POLYGON_VERTEX_COUNT * 2; 
typedef glm::vec2 ProjectedVertices[MAX_CLIPPED_PROJECTED_POLYGON_VERTEX_COUNT];

class BoundingBox {
public:
    enum { BOTTOM_LEFT, BOTTOM_RIGHT, TOP_RIGHT, TOP_LEFT, VERTEX_COUNT };

    BoundingBox(glm::vec2 corner, glm::vec2 size) : corner(corner), size(size), _set(true) {};
    BoundingBox() : _set(false) {};
    glm::vec2 corner;
    glm::vec2 size;
    bool contains(const BoundingBox& box) const;
    bool contains(const glm::vec2& point) const;
    bool pointInside(const glm::vec2& point) const { return contains(point); };
    
    void explandToInclude(const BoundingBox& box);

    float area() const { return size.x * size.y; };

    int getVertexCount() const { return VERTEX_COUNT; };
    glm::vec2 getVertex(int vertexNumber) const;

    BoundingBox topHalf() const;
    BoundingBox bottomHalf() const;
    BoundingBox leftHalf() const;
    BoundingBox rightHalf() const;

    float getMaxX() const { return corner.x + size.x; }
    float getMaxY() const { return corner.y + size.y; }
    float getMinX() const { return corner.x; }
    float getMinY() const { return corner.y; }
    
    void printDebugDetails(const char* label=NULL) const;
private:
    bool _set;
};

const int PROJECTION_RIGHT   = 1;
const int PROJECTION_LEFT    = 2;
const int PROJECTION_BOTTOM  = 4;
const int PROJECTION_TOP     = 8;
const int PROJECTION_NEAR    = 16;
const int PROJECTION_FAR     = 32;
const int PROJECTION_CLIPPED = 64;

class VoxelProjectedPolygon {

public:
    VoxelProjectedPolygon(const BoundingBox& box);

    VoxelProjectedPolygon(int vertexCount = 0) : 
        _vertexCount(vertexCount), 
        _maxX(-FLT_MAX), _maxY(-FLT_MAX), _minX(FLT_MAX), _minY(FLT_MAX),
        _distance(0)
        { };
        
    ~VoxelProjectedPolygon() { };
    const ProjectedVertices& getVertices() const { return _vertices; };
    const glm::vec2& getVertex(int i) const { return _vertices[i]; };
    void setVertex(int vertex, const glm::vec2& point);

    int getVertexCount() const                 { return _vertexCount; };
    void setVertexCount(int vertexCount)       { _vertexCount = vertexCount; };
    float getDistance() const                  { return _distance; }
    void  setDistance(float distance)          { _distance = distance; }
    bool getAnyInView() const                  { return _anyInView; };
    void setAnyInView(bool anyInView)          { _anyInView = anyInView; };
    bool getAllInView() const                  { return _allInView; };
    void setAllInView(bool allInView)          { _allInView = allInView; };
    void setProjectionType(unsigned char type) { _projectionType = type; };
    unsigned char getProjectionType() const    { return _projectionType; };


    bool pointInside(const glm::vec2& point, bool* matchesVertex = NULL) const;
    bool occludes(const VoxelProjectedPolygon& occludee, bool checkAllInView = false) const;
    bool occludes(const BoundingBox& occludee) const;
    bool intersects(const VoxelProjectedPolygon& testee) const;
    bool intersects(const BoundingBox& box) const;
    bool matches(const VoxelProjectedPolygon& testee) const;
    bool matches(const BoundingBox& testee) const;
    bool intersectsOnAxes(const VoxelProjectedPolygon& testee) const;

    bool canMerge(const VoxelProjectedPolygon& that) const;
    void merge(const VoxelProjectedPolygon& that); // replaces vertices of this with new merged version
    
    float getMaxX() const { return _maxX; }
    float getMaxY() const { return _maxY; }
    float getMinX() const { return _minX; }
    float getMinY() const { return _minY; }
    
    BoundingBox getBoundingBox() const { 
        return BoundingBox(glm::vec2(_minX,_minY), glm::vec2(_maxX - _minX, _maxY - _minY)); 
    };

    void printDebugDetails() const;
    
    static long pointInside_calls;
    static long occludes_calls;
    static long intersects_calls;

private:
    int _vertexCount;
    ProjectedVertices _vertices;
    float _maxX;
    float _maxY;
    float _minX;
    float _minY;
    float _distance;
    bool _anyInView; // if any points are in view
    bool _allInView; // if all points are in view
    unsigned char _projectionType;
};


#endif // _VOXEL_PROJECTED_SHADOW_
