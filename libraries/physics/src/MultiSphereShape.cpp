//
//  MultiSphereShape.cpp
//  libraries/physics/src
//
//  Created by Luis Cuenca 5/11/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MultiSphereShape.h"

void SphereRegion::translate(const glm::vec3& translation) {
    for (auto &line : _lines) {
        line.first += translation;
        line.second += translation;
    }
}
void SphereRegion::dump(std::vector<std::pair<glm::vec3, glm::vec3>>& outLines) {
    for (auto &line : _lines) {
        outLines.push_back(line);
    }
}

void SphereRegion::insertUnique(const glm::vec3& point, std::vector<glm::vec3>& pointSet) {
    auto hit = std::find_if(pointSet.begin(), pointSet.end(), [point](const glm::vec3& pointFromSet) -> bool {
        return (glm::length(pointFromSet-point) < FLT_EPSILON);
    });
    if (hit == pointSet.end()) {
        pointSet.push_back(point);
    }
}

void SphereRegion::extractEdges(bool reverseY) {
    if (_lines.size() == 0) {
        return;
    }
    float yVal = _lines[0].first.y;
    for (size_t i = 0; i < _lines.size(); i++) {
        yVal = reverseY ? glm::max(yVal, _lines[i].first.y) : glm::min(yVal, _lines[i].first.y);
    }
    for (size_t i = 0; i < _lines.size(); i++) {
        auto line = _lines[i];
        auto p1 = line.first;
        auto p2 = line.second;
        auto vec = p1 - p2;
        if (vec.z == 0.0f) {
            insertUnique(p1, _edgesX);
            insertUnique(p2, _edgesX);
        } else if (vec.y == 0.0f && p1.y == yVal && p2.y == yVal) {
            insertUnique(p1, _edgesY);
            insertUnique(p2, _edgesY);
        } else if (vec.x == 0.0f) {
            insertUnique(p1, _edgesZ);
            insertUnique(p2, _edgesZ);
        }
    }
}

void SphereRegion::extractSphereRegion(std::vector<std::pair<glm::vec3, glm::vec3>>& outLines) {
    for (size_t i = 0; i < outLines.size(); i++) {
        auto &line = outLines[i];
        auto &p1 = line.first;
        auto &p2 = line.second;
        p1.x = glm::abs(p1.x) < 0.001f ? 0.0f : p1.x;
        p1.y = glm::abs(p1.y) < 0.001f ? 0.0f : p1.y;
        p1.z = glm::abs(p1.z) < 0.001f ? 0.0f : p1.z;
        p2.x = glm::abs(p2.x) < 0.001f ? 0.0f : p2.x;
        p2.y = glm::abs(p2.y) < 0.001f ? 0.0f : p2.y;
        p2.z = glm::abs(p2.z) < 0.001f ? 0.0f : p2.z;

        glm::vec3 point1 = { p1.x != 0.0f ? glm::abs(p1.x) / p1.x : _direction.x,
            p1.y != 0.0f ? glm::abs(p1.y) / p1.y : _direction.y,
            p1.z != 0.0f ? glm::abs(p1.z) / p1.z : _direction.z };
        glm::vec3 point2 = { p2.x != 0.0f ? glm::abs(p2.x) / p2.x : _direction.x,
            p2.y != 0.0f ? glm::abs(p2.y) / p2.y : _direction.y,
            p2.z != 0.0f ? glm::abs(p2.z) / p2.z : _direction.z };
        if (point1 == _direction && point2 == _direction) {
            _lines.push_back(line);
        }
    }
}

CollisionShapeExtractionMode MultiSphereShape::getExtractionModeByName(const QString& name) {
    CollisionShapeExtractionMode mode = CollisionShapeExtractionMode::Automatic;
    bool isSim = name.indexOf("SIM") == 0;
    bool isFlow = name.indexOf("FLOW") == 0;
    bool isEye = name.indexOf("EYE") > -1;
    bool isToe = name.indexOf("TOE") > -1;
    bool isShoulder = name.indexOf("SHOULDER") > -1;
    bool isNeck = name.indexOf("NECK") > -1;
    bool isRightHand = name == "RIGHTHAND";
    bool isLeftHand = name == "LEFTHAND";
    bool isRightFinger = name.indexOf("RIGHTHAND") == 0 && !isRightHand;
    bool isLeftFinger = name.indexOf("LEFTHAND") == 0 && !isLeftHand;
    
    //bool isFinger = 
    if (isNeck || isLeftFinger || isRightFinger) {
        mode = CollisionShapeExtractionMode::SpheresY;
    } else if (isShoulder) {
        mode = CollisionShapeExtractionMode::SphereCollapse;
    } else if (isRightHand || isLeftHand) {
        mode = CollisionShapeExtractionMode::SpheresXY;
    } else if (isSim || isFlow || isEye || isToe) {
        mode = CollisionShapeExtractionMode::None;
    }
    return mode;
}

void MultiSphereShape::filterUniquePoints(const std::vector<btVector3>& kdop, std::vector<glm::vec3>& uniquePoints) {
    for (size_t j = 0; j < kdop.size(); j++) {
        btVector3 btPoint = kdop[j];
        auto hit = std::find_if(uniquePoints.begin(), uniquePoints.end(), [btPoint](const glm::vec3& point) -> bool {
            return (glm::length(btPoint.getX() - point.x) < FLT_EPSILON
                && glm::length(btPoint.getY() - point.y) < FLT_EPSILON
                && glm::length(btPoint.getZ() - point.z) < FLT_EPSILON);
        });
        if (hit == uniquePoints.end()) {
            uniquePoints.push_back(bulletToGLM(btPoint));
        }
    }
}

bool MultiSphereShape::computeMultiSphereShape(int jointIndex, const QString& name, const std::vector<btVector3>& kdop, float scale) {
    _scale = scale;
    _jointIndex = jointIndex;
    _name = name;
    _mode = getExtractionModeByName(_name);
    if (_mode == CollisionShapeExtractionMode::None || kdop.size() < 4 || kdop.size() > 200) {
        return false;
    }
    std::vector<glm::vec3> points;
    filterUniquePoints(kdop, points);
    glm::vec3 min = glm::vec3(100.0f, 100.0f, 100.0f);
    glm::vec3 max = glm::vec3(-100.0f, -100.0f, -100.0f);
    _midPoint = glm::vec3(0.0f, 0.0f, 0.0f);
    std::vector<glm::vec3> relPoints;
    for (size_t i = 0; i < points.size(); i++) {

        min.x = points[i].x < min.x ? points[i].x : min.x;
        min.y = points[i].y < min.y ? points[i].y : min.y;
        min.z = points[i].z < min.z ? points[i].z : min.z;

        max.x = points[i].x > max.x ? points[i].x : max.x;
        max.y = points[i].y > max.y ? points[i].y : max.y;
        max.z = points[i].z > max.z ? points[i].z : max.z;

        _midPoint += points[i];
    }

    _midPoint /= (int)points.size();
    glm::vec3 dimensions = max - min;

    for (size_t i = 0; i < points.size(); i++) {
        glm::vec3 relPoint = points[i] - _midPoint;
        relPoints.push_back(relPoint);
    }
    CollisionShapeExtractionMode applyMode = _mode;
    float xCorrector = dimensions.x > dimensions.y && dimensions.x > dimensions.z ? -1.0f + (dimensions.x / (0.5f * (dimensions.y + dimensions.z))) : 0.0f;
    float yCorrector = dimensions.y > dimensions.x && dimensions.y > dimensions.z ? -1.0f + (dimensions.y / (0.5f * (dimensions.x + dimensions.z))) : 0.0f;
    float zCorrector = dimensions.z > dimensions.x && dimensions.z > dimensions.y ? -1.0f + (dimensions.z / (0.5f * (dimensions.x + dimensions.y))) : 0.0f;

    float xyDif = glm::abs(dimensions.x - dimensions.y);
    float xzDif = glm::abs(dimensions.x - dimensions.z);
    float yzDif = glm::abs(dimensions.y - dimensions.z);

    float xyEpsilon = (0.05f + zCorrector) * glm::max(dimensions.x, dimensions.y);
    float xzEpsilon = (0.05f + yCorrector) * glm::max(dimensions.x, dimensions.z);
    float yzEpsilon = (0.05f + xCorrector) * glm::max(dimensions.y, dimensions.z);

    if (xyDif < 0.5f * xyEpsilon && xzDif < 0.5f * xzEpsilon && yzDif < 0.5f * yzEpsilon) {
        applyMode = CollisionShapeExtractionMode::Sphere;
    } else if (xzDif < xzEpsilon) {
        applyMode = dimensions.y > dimensions.z ? CollisionShapeExtractionMode::SpheresY : CollisionShapeExtractionMode::SpheresXZ;
    } else if (xyDif < xyEpsilon) {
        applyMode = dimensions.z > dimensions.y ? CollisionShapeExtractionMode::SpheresZ : CollisionShapeExtractionMode::SpheresXY;
    } else if (yzDif < yzEpsilon) {
        applyMode = dimensions.x > dimensions.y ? CollisionShapeExtractionMode::SpheresX : CollisionShapeExtractionMode::SpheresYZ;
    } else {
        applyMode = CollisionShapeExtractionMode::SpheresXYZ;
    }

    if (_mode != CollisionShapeExtractionMode::Automatic && applyMode != _mode) {
        bool isModeSphereAxis = (_mode >= CollisionShapeExtractionMode::SpheresX && _mode <= CollisionShapeExtractionMode::SpheresZ);
        bool isApplyModeComplex = (applyMode >= CollisionShapeExtractionMode::SpheresXY && applyMode <= CollisionShapeExtractionMode::SpheresXYZ);
        applyMode = (isModeSphereAxis && isApplyModeComplex) ? CollisionShapeExtractionMode::Sphere : _mode;
    }

    std::vector<glm::vec3> axes;
    glm::vec3 axis, axis1, axis2;
    SphereShapeData sphere;
    switch (applyMode) {
    case CollisionShapeExtractionMode::None:
        break;
    case CollisionShapeExtractionMode::Automatic:
        break;
    case CollisionShapeExtractionMode::Box:
        break;
    case CollisionShapeExtractionMode::Sphere:
        sphere._radius = 0.5f * (dimensions.x + dimensions.y + dimensions.z) / 3.0f;
        sphere._position = glm::vec3(0.0f);
        _spheres.push_back(sphere);
        break;
    case CollisionShapeExtractionMode::SphereCollapse:
        sphere._radius = 0.5f * glm::min(glm::min(dimensions.x, dimensions.y), dimensions.z);
        sphere._position = glm::vec3(0.0f);
        _spheres.push_back(sphere);
        break;
    case CollisionShapeExtractionMode::SpheresX:
        axis = 0.5f* dimensions.x * Vectors::UNIT_NEG_X;
        axes = { axis, -axis };
        break;
    case CollisionShapeExtractionMode::SpheresY:
        axis = 0.5f* dimensions.y * Vectors::UNIT_NEG_Y;
        axes = { axis, -axis };
        break;
    case CollisionShapeExtractionMode::SpheresZ:
        axis = 0.5f* dimensions.z * Vectors::UNIT_NEG_Z;
        axes = { axis, -axis };
        break;
    case CollisionShapeExtractionMode::SpheresXY:
        axis1 = glm::vec3(0.5f * dimensions.x, 0.5f * dimensions.y, 0.0f);
        axis2 = glm::vec3(0.5f * dimensions.x, -0.5f * dimensions.y, 0.0f);
        axes = { axis1, axis2, -axis1, -axis2 };
        break;
    case CollisionShapeExtractionMode::SpheresYZ:
        axis1 = glm::vec3(0.0f, 0.5f * dimensions.y, 0.5f * dimensions.z);
        axis2 = glm::vec3(0.0f, 0.5f * dimensions.y, -0.5f * dimensions.z);
        axes = { axis1, axis2, -axis1, -axis2 };
        break;
    case CollisionShapeExtractionMode::SpheresXZ:
        axis1 = glm::vec3(0.5f * dimensions.x, 0.0f, 0.5f * dimensions.z);
        axis2 = glm::vec3(-0.5f * dimensions.x, 0.0f, 0.5f * dimensions.z);
        axes = { axis1, axis2, -axis1, -axis2 };
        break;
    case CollisionShapeExtractionMode::SpheresXYZ:
        for (size_t i = 0; i < CORNER_SIGNS.size(); i++) {
            axes.push_back(0.5f * (dimensions * CORNER_SIGNS[i]));
        }
        break;
    default:
        break;
    }
    if (axes.size() > 0) {
        spheresFromAxes(relPoints, axes, _spheres);
    }
    for (size_t i = 0; i < _spheres.size(); i++) {
        _spheres[i]._position += _midPoint;
    }
    
    return _mode != CollisionShapeExtractionMode::None;
}

void MultiSphereShape::spheresFromAxes(const std::vector<glm::vec3>& points, const std::vector<glm::vec3>& axes, std::vector<SphereShapeData>& spheres) {
    float maxRadius = 0.0f;
    float maxAverageRadius = 0.0f;
    float minAverageRadius = glm::length(points[0]);
    size_t sphereCount = axes.size();
    spheres.clear();
    for (size_t j = 0; j < sphereCount; j++) {
        SphereShapeData sphere = SphereShapeData();
        sphere._axis = axes[j];
        spheres.push_back(sphere);
    }
    for (size_t j = 0; j < sphereCount; j++) {
        auto axis = _spheres[j]._axis;
        float averageRadius = 0.0f;
        float maxDistance = glm::length(axis);
        glm::vec3 axisDir = glm::normalize(axis);
        for (size_t i = 0; i < points.size(); i++) {
            float dot = glm::dot(points[i], axisDir);
            if (dot > 0.0f) {
                float distancePow = glm::distance2(Vectors::ZERO, points[i]);
                float dotPow = glm::pow(dot, 2);
                float radius = (dot / maxDistance) * glm::sqrt(distancePow - dotPow);
                averageRadius += radius;
                maxRadius = radius > maxRadius ? radius : maxRadius;
            }
        }
        if (points.size() > 0) {
            averageRadius /= (int)points.size();
        }
        maxAverageRadius = glm::max(averageRadius, maxAverageRadius);
        minAverageRadius = glm::min(averageRadius, minAverageRadius);
        spheres[j]._radius = averageRadius;
    }
    if (maxAverageRadius == 0.0f) {
        maxAverageRadius = 1.0f;
    }
    float radiusRatio = maxRadius / maxAverageRadius;
    // Push the sphere into the bounding box
    float contractionRatio = 0.8f;
    for (size_t j = 0; j < sphereCount; j++) {
        auto axis = _spheres[j]._axis;
        float distance = glm::length(axis);
        float correntionRatio = radiusRatio * (spheres[j]._radius / maxAverageRadius);
        float radius = (correntionRatio < 0.8f * radiusRatio ? 0.8f * radiusRatio : correntionRatio) * spheres[j]._radius;
        if (sphereCount > 3) {
            distance = contractionRatio * distance;
        }
        spheres[j]._radius = radius;
        if (distance - radius > 0.0f) {
            spheres[j]._position = ((distance - radius) / distance) * axis;
        } else {
            spheres[j]._position = glm::vec3(0.0f);
        }
    }
    // Collapse spheres if too close
    if (sphereCount == 2) {
        int maxRadiusIndex = spheres[0]._radius > spheres[1]._radius ? 0 : 1;
        if (glm::length(spheres[0]._position - spheres[1]._position) < 0.2f * spheres[maxRadiusIndex]._radius) {
            SphereShapeData newSphere;
            newSphere._position = 0.5f * (spheres[0]._position + spheres[1]._position);
            newSphere._radius = spheres[maxRadiusIndex]._radius;
            spheres.clear();
            spheres.push_back(newSphere);
        }
    }
}

void MultiSphereShape::connectSpheres(int index1, int index2, bool onlyEdges) {
    auto sphere1 = _spheres[index1]._radius > _spheres[index2]._radius ? _spheres[index1] : _spheres[index2];
    auto sphere2 = _spheres[index1]._radius <= _spheres[index2]._radius ? _spheres[index1] : _spheres[index2];
    float distance = glm::length(sphere1._position - sphere2._position);

    auto axis = sphere1._position - sphere2._position;

    float angleOffset = glm::asin((sphere1._radius - sphere2._radius) / distance);
    float ratio1 = ((0.5f * PI) + angleOffset) / PI;
    float ratio2 = ((0.5f * PI) - angleOffset) / PI;

    std::vector<glm::vec3> edge1, edge2;
    if (onlyEdges) {
        std::vector<std::pair<glm::vec3, glm::vec3>> debugLines;
        calculateSphereLines(debugLines, sphere1._position, sphere1._radius, DEFAULT_SPHERE_SUBDIVISIONS, glm::normalize(axis), ratio1, &edge1);
        calculateSphereLines(debugLines, sphere2._position, sphere2._radius, DEFAULT_SPHERE_SUBDIVISIONS, glm::normalize(-axis), ratio2, &edge2);
    } else {
        calculateSphereLines(_debugLines, sphere1._position, sphere1._radius, DEFAULT_SPHERE_SUBDIVISIONS, glm::normalize(axis), ratio1, &edge1);
        calculateSphereLines(_debugLines, sphere2._position, sphere2._radius, DEFAULT_SPHERE_SUBDIVISIONS, glm::normalize(-axis), ratio2, &edge2);
    }
    connectEdges(_debugLines, edge1, edge2);
}

void MultiSphereShape::calculateDebugLines() {
    if (_spheres.size() == 1) {
        auto sphere = _spheres[0];
        calculateSphereLines(_debugLines, sphere._position, sphere._radius);
    } else if (_spheres.size() == 2) {
        connectSpheres(0, 1);        
    } else if (_spheres.size() == 4) {
        std::vector<glm::vec3> axes;
        axes.resize(8);
        for (size_t i = 0; i < CORNER_SIGNS.size(); i++) {
            for (size_t j = 0; j < 4; j++) {
                auto axis = _spheres[j]._position - _midPoint;
                glm::vec3 sign = { axis.x != 0.0f ? glm::abs(axis.x) / axis.x : 0.0f,
                                   axis.x != 0.0f ? glm::abs(axis.y) / axis.y : 0.0f ,
                                   axis.z != 0.0f ? glm::abs(axis.z) / axis.z : 0.0f };
                bool add = false;
                if (sign.x == 0.0f) {
                    if (sign.y == CORNER_SIGNS[i].y && sign.z == CORNER_SIGNS[i].z) {
                        add = true;
                    }
                } else if (sign.y == 0.0f) {
                    if (sign.x == CORNER_SIGNS[i].x && sign.z == CORNER_SIGNS[i].z) {
                        add = true;
                    }
                } else if (sign.z == 0.0f) {
                    if (sign.x == CORNER_SIGNS[i].x && sign.y == CORNER_SIGNS[i].y) {
                        add = true;
                    }
                } else if (sign == CORNER_SIGNS[i]) {
                    add = true;
                }
                if (add) {
                    axes[i] = axis;
                    break;
                }
            }            
        }        
        calculateChamferBox(_debugLines, _spheres[0]._radius, axes, _midPoint);
    } else if (_spheres.size() == 8) {
        std::vector<glm::vec3> axes;
        for (size_t i = 0; i < _spheres.size(); i++) {
            axes.push_back(_spheres[i]._position - _midPoint);
        }
        calculateChamferBox(_debugLines, _spheres[0]._radius, axes, _midPoint);
    }
}

void MultiSphereShape::connectEdges(std::vector<std::pair<glm::vec3, glm::vec3>>& outLines, const std::vector<glm::vec3>& edge1, const std::vector<glm::vec3>& edge2, bool reverse) {
    if (edge1.size() == edge2.size()) {
        for (size_t i = 0; i < edge1.size(); i++) {
            size_t j = reverse ? edge1.size() - i - 1 : i;
            outLines.push_back({ edge1[i], edge2[j] });
        }
    }
}

void MultiSphereShape::calculateChamferBox(std::vector<std::pair<glm::vec3, glm::vec3>>& outLines, const float& radius, const std::vector<glm::vec3>& axes, const glm::vec3& translation) {
    std::vector<std::pair<glm::vec3, glm::vec3>> sphereLines;
    calculateSphereLines(sphereLines, glm::vec3(0.0f), radius);

    std::vector<SphereRegion> regions = {
        SphereRegion({ 1.0f, 1.0f, 1.0f }),
        SphereRegion({ -1.0f, 1.0f, 1.0f }),
        SphereRegion({ -1.0f, 1.0f, -1.0f }),
        SphereRegion({ 1.0f, 1.0f, -1.0f }),
        SphereRegion({ 1.0f, -1.0f, 1.0f }),
        SphereRegion({ -1.0f, -1.0f, 1.0f }),
        SphereRegion({ -1.0f, -1.0f, -1.0f }),
        SphereRegion({ 1.0f, -1.0f, -1.0f })
    };
    
    assert(axes.size() == regions.size());

    for (size_t i = 0; i < regions.size(); i++) {
        regions[i].extractSphereRegion(sphereLines);
        regions[i].translate(translation + axes[i]);
        regions[i].extractEdges(axes[i].y < 0);
        regions[i].dump(outLines);
    }

    connectEdges(outLines, regions[0].getEdgesZ(), regions[1].getEdgesZ());
    connectEdges(outLines, regions[1].getEdgesX(), regions[2].getEdgesX());
    connectEdges(outLines, regions[2].getEdgesZ(), regions[3].getEdgesZ());
    connectEdges(outLines, regions[3].getEdgesX(), regions[0].getEdgesX());

    connectEdges(outLines, regions[4].getEdgesZ(), regions[5].getEdgesZ());
    connectEdges(outLines, regions[5].getEdgesX(), regions[6].getEdgesX());
    connectEdges(outLines, regions[6].getEdgesZ(), regions[7].getEdgesZ());
    connectEdges(outLines, regions[7].getEdgesX(), regions[4].getEdgesX());

    connectEdges(outLines, regions[0].getEdgesY(), regions[4].getEdgesY());
    connectEdges(outLines, regions[1].getEdgesY(), regions[5].getEdgesY());
    connectEdges(outLines, regions[2].getEdgesY(), regions[6].getEdgesY());
    connectEdges(outLines, regions[3].getEdgesY(), regions[7].getEdgesY());

}

void MultiSphereShape::calculateSphereLines(std::vector<std::pair<glm::vec3, glm::vec3>>& outLines, const glm::vec3& center, const float& radius,
                                     const int& subdivisions, const glm::vec3& direction, const float& percentage, std::vector<glm::vec3>* edge) {
    
    float uTotalAngle = percentage * PI;
    float vTotalAngle = 2.0f * PI;

    int uSubdivisions = (int)glm::ceil(subdivisions * 0.5f * percentage);
    int vSubdivisions = subdivisions;

    float uDeltaAngle = uTotalAngle / uSubdivisions;
    float vDeltaAngle = vTotalAngle / vSubdivisions;

    float uAngle = 0.0f;

    glm::vec3 uAxis, vAxis;
    glm::vec3 mainAxis = glm::normalize(direction);
    if (mainAxis.y == 1.0f || mainAxis.y == -1.0f) {
        uAxis = glm::vec3(1.0f, 0.0f, 0.0f);
        vAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
        uAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), mainAxis));
        vAxis = glm::normalize(glm::cross(mainAxis, uAxis));
        if ((uAxis.z == 0.0f && uAxis.x < 0.0f) || (uAxis.x == 0.0f && uAxis.z < 0.0f)) {
            uAxis = -uAxis;
        } else if (uAxis.x < 0.0f) {
            uAxis = -uAxis;
        }
        if ((vAxis.z == 0.0f && vAxis.x < 0.0f) || (vAxis.x == 0.0f && vAxis.z < 0.0f)) {
            vAxis = -vAxis;
        } else if (vAxis.x < 0.0f) {
            vAxis = -vAxis;
        }
    }

    std::vector<std::vector<glm::vec3>> arcs;
    auto origin = center;
    for (int u = 0; u < uSubdivisions + 1; u++) {
        std::vector<glm::vec3> arc;
        glm::vec3 arcCenter = origin + mainAxis * (glm::cos(uAngle) * radius);
        float vAngle = 0.0f;
        for (int v = 0; v < vSubdivisions + 1; v++) {
            float arcRadius = glm::abs(glm::sin(uAngle) * radius);
            glm::vec3 arcPoint = arcCenter + (arcRadius * glm::cos(vAngle)) * uAxis + (arcRadius * glm::sin(vAngle)) * vAxis;
            arc.push_back(arcPoint);
            if (u == uSubdivisions && edge != nullptr) {
                edge->push_back(arcPoint);
            }
            vAngle += vDeltaAngle;
        }
        arc.push_back(arc[0]);
        arcs.push_back(arc);
        uAngle += uDeltaAngle;
    }

    for (size_t i = 1; i < arcs.size(); i++) {
        auto arc1 = arcs[i];
        auto arc2 = arcs[i - 1];
        for (size_t j = 1; j < arc1.size(); j++) {
            auto point1 = arc1[j];
            auto point2 = arc1[j - 1];
            auto point3 = arc2[j];
            std::pair<glm::vec3, glm::vec3> line1 = { point1, point2 };
            std::pair<glm::vec3, glm::vec3> line2 = { point1, point3 };
            outLines.push_back(line1);
            outLines.push_back(line2);
        }
    }
}

void MultiSphereShape::setScale(float scale) {
    if (scale != _scale) {
        float deltaScale = scale / _scale;
        for (auto& sphere : _spheres) {
            sphere._axis *= deltaScale;
            sphere._position *= deltaScale;
            sphere._radius *= deltaScale;
        }
        for (auto& line : _debugLines) {
            line.first *= deltaScale;
            line.second *= deltaScale;
        }
        _scale = scale;
    }
}

AABox& MultiSphereShape::updateBoundingBox(const glm::vec3& position, const glm::quat& rotation) {
    _boundingBox = AABox();
    auto spheres = getSpheresData();
    for (size_t i = 0; i < spheres.size(); i++) {
        auto sphere = spheres[i];
        auto worldPosition = position + rotation * sphere._position;
        glm::vec3 corner = worldPosition - glm::vec3(sphere._radius);
        glm::vec3 dimensions = glm::vec3(2.0f * sphere._radius);
        _boundingBox += AABox(corner, dimensions);
    }
    return _boundingBox;
}