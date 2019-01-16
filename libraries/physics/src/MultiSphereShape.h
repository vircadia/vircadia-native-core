//
//  MultiSphereShape.h
//  libraries/physics/src
//
//  Created by Luis Cuenca 5/11/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MultiSphereShape_h
#define hifi_MultiSphereShape_h

#include <stdint.h>
#include <btBulletDynamicsCommon.h>
#include <GLMHelpers.h>
#include <AABox.h>
#include "BulletUtil.h"


enum CollisionShapeExtractionMode {
    None = 0,
    Automatic,
    Box,
    Sphere,
    SphereCollapse,
    SpheresX,
    SpheresY,
    SpheresZ,
    SpheresXY,
    SpheresYZ,
    SpheresXZ,
    SpheresXYZ
};

struct SphereShapeData {
    SphereShapeData() {}
    glm::vec3 _position;
    glm::vec3 _axis;
    float _radius;    
};

class SphereRegion {
public:
    SphereRegion() {}
    SphereRegion(const glm::vec3& direction) : _direction(direction) {}
    void extractSphereRegion(std::vector<std::pair<glm::vec3, glm::vec3>>& outLines);
    void extractEdges(bool reverseY = false);
    void translate(const glm::vec3& translation);
    void dump(std::vector<std::pair<glm::vec3, glm::vec3>>& outLines);
    const glm::vec3& getDirection() const { return _direction; }
    const std::vector<glm::vec3>& getEdgesX() const { return _edgesX; }
    const std::vector<glm::vec3>& getEdgesY() const { return _edgesY; }
    const std::vector<glm::vec3>& getEdgesZ() const { return _edgesZ; }
private:
    void insertUnique(const glm::vec3& point, std::vector<glm::vec3>& pointSet);
    
    std::vector<std::pair<glm::vec3, glm::vec3>> _lines;
    std::vector<glm::vec3> _edgesX;
    std::vector<glm::vec3> _edgesY;
    std::vector<glm::vec3> _edgesZ;
    glm::vec3 _direction;
};

const int DEFAULT_SPHERE_SUBDIVISIONS = 16;

const std::vector<glm::vec3> CORNER_SIGNS = {
    glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(-1.0f, 1.0f, 1.0f),
    glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 1.0f, -1.0f),
    glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(-1.0f, -1.0f, 1.0f),
    glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, -1.0f, -1.0f) };

class MultiSphereShape {
public:
    MultiSphereShape() {};
    bool computeMultiSphereShape(int jointIndex, const QString& name, const std::vector<btVector3>& points, float scale = 1.0f);
    void calculateDebugLines();
    const std::vector<SphereShapeData>& getSpheresData() const { return _spheres; }
    const std::vector<std::pair<glm::vec3, glm::vec3>>& getDebugLines() const { return _debugLines; }
    void setScale(float scale);
    AABox& updateBoundingBox(const glm::vec3& position, const glm::quat& rotation);
    const AABox& getBoundingBox() const { return _boundingBox; }
    int getJointIndex() const { return _jointIndex; }
    QString getJointName() const { return _name; }
    bool isValid() const { return _spheres.size() > 0; }

private:
    CollisionShapeExtractionMode getExtractionModeByName(const QString& name);
    void filterUniquePoints(const std::vector<btVector3>& kdop, std::vector<glm::vec3>& uniquePoints);
    void spheresFromAxes(const std::vector<glm::vec3>& points, const std::vector<glm::vec3>& axes, 
                              std::vector<SphereShapeData>& spheres);
    
    void calculateSphereLines(std::vector<std::pair<glm::vec3, glm::vec3>>& outLines, const glm::vec3& center, const float& radius,
                              const int& subdivisions = DEFAULT_SPHERE_SUBDIVISIONS, const glm::vec3& direction = Vectors::UNIT_Y, 
                              const float& percentage = 1.0f, std::vector<glm::vec3>* edge = nullptr);
    void calculateChamferBox(std::vector<std::pair<glm::vec3, glm::vec3>>& outLines, const float& radius, const std::vector<glm::vec3>& axes, const glm::vec3& translation);
    void connectEdges(std::vector<std::pair<glm::vec3, glm::vec3>>& outLines, const std::vector<glm::vec3>& edge1, 
                      const std::vector<glm::vec3>& edge2, bool reverse = false);
    void connectSpheres(int index1, int index2, bool onlyEdges = false);

    int _jointIndex { -1 };
    QString _name;
    std::vector<SphereShapeData> _spheres;
    std::vector<std::pair<glm::vec3, glm::vec3>> _debugLines;
    CollisionShapeExtractionMode _mode;
    glm::vec3 _midPoint;
    float _scale { 1.0f };
    AABox _boundingBox;
};

#endif // hifi_MultiSphereShape_h