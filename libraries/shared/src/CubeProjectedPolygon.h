//
//  CubeProjectedPolygon.h
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 06/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  The projected shadow (on the 2D view plane) for a cube
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CubeProjectedPolygon_h
#define hifi_CubeProjectedPolygon_h

#include <glm/glm.hpp>

// there's a max of 6 vertices of a project polygon, and a max of twice that when clipped to the screen
const int MAX_PROJECTED_POLYGON_VERTEX_COUNT = 6;
const int MAX_CLIPPED_PROJECTED_POLYGON_VERTEX_COUNT = MAX_PROJECTED_POLYGON_VERTEX_COUNT * 2;
typedef glm::vec2 ProjectedVertices[MAX_CLIPPED_PROJECTED_POLYGON_VERTEX_COUNT];

class BoundingRectangle {
public:
    enum { BOTTOM_LEFT, BOTTOM_RIGHT, TOP_RIGHT, TOP_LEFT, VERTEX_COUNT };

    BoundingRectangle(const glm::vec2 corner, const glm::vec2 size) : corner(corner), size(size), _set(true) {}
    BoundingRectangle() : _set(false) {}
    glm::vec2 corner;
    glm::vec2 size;
    bool contains(const BoundingRectangle& box) const;
    bool contains(const glm::vec2& point) const;
    bool pointInside(const glm::vec2& point) const { return contains(point); }

    void explandToInclude(const BoundingRectangle& box);

    float area() const { return size.x * size.y; }

    int getVertexCount() const { return VERTEX_COUNT; }
    glm::vec2 getVertex(int vertexNumber) const;

    BoundingRectangle topHalf() const;
    BoundingRectangle bottomHalf() const;
    BoundingRectangle leftHalf() const;
    BoundingRectangle rightHalf() const;

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

class CubeProjectedPolygon {

public:
    CubeProjectedPolygon(const BoundingRectangle& box);

    CubeProjectedPolygon(int vertexCount = 0) :
        _vertexCount(vertexCount),
        _maxX(-FLT_MAX), _maxY(-FLT_MAX), _minX(FLT_MAX), _minY(FLT_MAX),
        _distance(0)
        { }

    ~CubeProjectedPolygon() { }
    const ProjectedVertices& getVertices() const { return _vertices; }
    const glm::vec2& getVertex(int i) const { return _vertices[i]; }
    void setVertex(int vertex, const glm::vec2& point);

    int getVertexCount() const { return _vertexCount; }
    float getDistance() const { return _distance; }
    bool getAnyInView() const { return _anyInView; }
    bool getAllInView() const { return _allInView; }
    unsigned char getProjectionType() const { return _projectionType; }
    void setVertexCount(int vertexCount) { _vertexCount = vertexCount; }
    void setDistance(float distance) { _distance = distance; }
    void setAnyInView(bool anyInView) { _anyInView = anyInView; }
    void setAllInView(bool allInView) { _allInView = allInView; }
    void setProjectionType(unsigned char type) { _projectionType = type; }


    bool pointInside(const glm::vec2& point, bool* matchesVertex = NULL) const;
    bool occludes(const CubeProjectedPolygon& occludee, bool checkAllInView = false) const;
    bool occludes(const BoundingRectangle& occludee) const;
    bool intersects(const CubeProjectedPolygon& testee) const;
    bool intersects(const BoundingRectangle& box) const;
    bool matches(const CubeProjectedPolygon& testee) const;
    bool matches(const BoundingRectangle& testee) const;
    bool intersectsOnAxes(const CubeProjectedPolygon& testee) const;

    bool canMerge(const CubeProjectedPolygon& that) const;
    void merge(const CubeProjectedPolygon& that); // replaces vertices of this with new merged version

    float getMaxX() const { return _maxX; }
    float getMaxY() const { return _maxY; }
    float getMinX() const { return _minX; }
    float getMinY() const { return _minY; }

    BoundingRectangle getBoundingBox() const {
        return BoundingRectangle(glm::vec2(_minX,_minY), glm::vec2(_maxX - _minX, _maxY - _minY));
    }

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


#endif // hifi_CubeProjectedPolygon_h
