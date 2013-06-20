//
//  VoxelProjectedPolygon.h - The projected shadow (on the 2D view plane) for a voxel
//  hifi
//
//  Added by Brad Hefta-Gaub on 06/11/13.
//

#ifndef _VOXEL_PROJECTED_SHADOW_
#define _VOXEL_PROJECTED_SHADOW_

#include <glm/glm.hpp>

const int MAX_SHADOW_VERTEX_COUNT = 6;

typedef glm::vec2 ShadowVertices[MAX_SHADOW_VERTEX_COUNT];

class BoundingBox {
public:
    BoundingBox(glm::vec2 corner, glm::vec2 size) : corner(corner), size(size), _set(true) {};
    BoundingBox() : _set(false) {};
    glm::vec2 corner;
    glm::vec2 size;
    bool contains(const BoundingBox& box) const;
    void explandToInclude(const BoundingBox& box);

    float area() const { return size.x * size.y; };

    BoundingBox topHalf() const;
    BoundingBox bottomHalf() const;
    BoundingBox leftHalf() const;
    BoundingBox rightHalf() const;
    
    void printDebugDetails(const char* label=NULL) const;
private:
    bool _set;
};

class VoxelProjectedPolygon {

public:
    VoxelProjectedPolygon(int vertexCount = 0) : 
        _vertexCount(vertexCount), 
        _maxX(-FLT_MAX), _maxY(-FLT_MAX), _minX(FLT_MAX), _minY(FLT_MAX),
        _distance(0)
        { };
        
    ~VoxelProjectedPolygon() { };
    const ShadowVertices& getVerices() const { return _vertices; };
    const glm::vec2& getVertex(int i) const { return _vertices[i]; };
    void setVertex(int vertex, const glm::vec2& point);
    int getVertexCount() const { return _vertexCount; };
    void setVertexCount(int vertexCount) { _vertexCount = vertexCount; };

    float getDistance() const { return _distance; }
    void  setDistance(float distance) { _distance = distance; }

    bool getAnyInView() const { return _anyInView; };
    void setAnyInView(bool anyInView) { _anyInView = anyInView; };
    bool getAllInView() const { return _allInView; };
    void setAllInView(bool allInView) { _allInView = allInView; };

    bool occludes(const VoxelProjectedPolygon& occludee, bool checkAllInView = false) const;
    bool pointInside(const glm::vec2& point) const;
    
    float getMaxX() const { return _maxX; }
    float getMaxY() const { return _maxY; }
    float getMinX() const { return _minX; }
    float getMinY() const { return _minY; }
    
    BoundingBox getBoundingBox() const { 
        return BoundingBox(glm::vec2(_minX,_minY), glm::vec2(_maxX - _minX, _maxY - _minY)); 
    };

    void printDebugDetails() const;

private:
    int _vertexCount;
    ShadowVertices _vertices;
    float _maxX;
    float _maxY;
    float _minX;
    float _minY;
    float _distance;
    bool _anyInView; // if any points are in view
    bool _allInView; // if all points are in view
};


#endif // _VOXEL_PROJECTED_SHADOW_
