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
#include "PhysicsLogging.h"

void SphereRegion::translate(const glm::vec3& translation) {
    for (auto &line : _lines) {
        line.first += translation;
        line.second += translation;
    }
}

void SphereRegion::scale(float scale) {
    for (auto &line : _lines) {
        line.first *= scale;
        line.second *= scale;
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

MultiSphereShape::ExtractionMode MultiSphereShape::getExtractionModeByJointName(const QString& name) {
    ExtractionMode mode = ExtractionMode::Automatic;
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
        mode = ExtractionMode::SpheresY;
    } else if (isShoulder) {
        mode = ExtractionMode::SphereCollapse;
    } else if (isRightHand || isLeftHand) {
        mode = ExtractionMode::SpheresXY;
    } else if (isSim || isFlow || isEye || isToe) {
        mode = ExtractionMode::None;
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

bool MultiSphereShape::computeMultiSphereShape(int jointIndex, const QString& jointName, const std::vector<btVector3>& kdop, float scale) {
    _scale = scale;
    _jointIndex = jointIndex;
    _jointName = jointName;
    auto mode = getExtractionModeByJointName(_jointName);
    KdopData kdopData = getKdopData(kdop);
    if (kdop.size() < 4 || mode == ExtractionMode::None || !kdopData._isValidShape) {
        return false;
    }
    bool needRecompute = true;
    while (needRecompute) {
        CollapsingMode collapsingMode = computeSpheres(mode, kdopData);
        needRecompute = collapsingMode != CollapsingMode::None;
        if (needRecompute) {
            mode = (CollapsingMode)collapsingMode;
        }
    }
    return mode != ExtractionMode::None;
}

MultiSphereShape::KdopData MultiSphereShape::getKdopData(const std::vector<btVector3>& kdop) {
    KdopData data;
    std::vector<glm::vec3> points;
    filterUniquePoints(kdop, points);
    glm::vec3 min = glm::vec3(100.0f, 100.0f, 100.0f);
    glm::vec3 max = glm::vec3(-100.0f, -100.0f, -100.0f);
    data._origin = glm::vec3(0.0f, 0.0f, 0.0f);
    std::vector<glm::vec3> relPoints;
    for (size_t i = 0; i < points.size(); i++) {

        min.x = points[i].x < min.x ? points[i].x : min.x;
        min.y = points[i].y < min.y ? points[i].y : min.y;
        min.z = points[i].z < min.z ? points[i].z : min.z;

        max.x = points[i].x > max.x ? points[i].x : max.x;
        max.y = points[i].y > max.y ? points[i].y : max.y;
        max.z = points[i].z > max.z ? points[i].z : max.z;

        data._origin += points[i];
    }

    data._origin /= (int)points.size();
    glm::vec3& dimensions = data._dimensions;
    dimensions = max - min;
    if (glm::length(dimensions) == 0.0f) {
        data._isValidShape = false;
        return data;
    }
    for (size_t i = 0; i < points.size(); i++) {
        glm::vec3 relPoint = points[i] - data._origin;
        data._relativePoints.push_back(relPoint);
    }
    glm::vec3 corrector;
    
    corrector.x = dimensions.x > dimensions.y && dimensions.x > dimensions.z ? -1.0f + (dimensions.x / (0.5f * (dimensions.y + dimensions.z))) : 0.0f;
    corrector.y = dimensions.y > dimensions.x && dimensions.y > dimensions.z ? -1.0f + (dimensions.y / (0.5f * (dimensions.x + dimensions.z))) : 0.0f;
    corrector.z = dimensions.z > dimensions.x && dimensions.z > dimensions.y ? -1.0f + (dimensions.z / (0.5f * (dimensions.x + dimensions.y))) : 0.0f;

    KdopCoefficient& diff = data._diff;
    diff.xy = glm::abs(dimensions.x - dimensions.y);
    diff.xz = glm::abs(dimensions.x - dimensions.z);
    diff.yz = glm::abs(dimensions.y - dimensions.z);

    KdopCoefficient& epsilon = data._epsilon;
    epsilon.xy = (0.05f + corrector.z) * glm::max(dimensions.x, dimensions.y);
    epsilon.xz = (0.05f + corrector.y) * glm::max(dimensions.x, dimensions.z);
    epsilon.yz = (0.05f + corrector.x) * glm::max(dimensions.y, dimensions.z);

    return data;
}

MultiSphereShape::CollapsingMode MultiSphereShape::computeSpheres(ExtractionMode mode, const KdopData& data) {
    _mode = mode;
    _midPoint = data._origin;
    ExtractionMode applyMode = mode;
    _spheres.clear();
    auto& diff = data._diff;
    auto& epsilon = data._epsilon;
    auto& dimensions = data._dimensions;

    if (_mode == ExtractionMode::Automatic) {
        if (diff.xy < 0.5f * epsilon.xy && diff.xz < 0.5f * epsilon.xz && diff.yz < 0.5f * epsilon.yz) {
            applyMode =ExtractionMode::Sphere;
        } else if (diff.xz < epsilon.xz) {
            applyMode = dimensions.y > dimensions.z ? ExtractionMode::SpheresY : ExtractionMode::SpheresXZ;
        } else if (diff.xy < epsilon.xy) {
            applyMode = dimensions.z > dimensions.y ? ExtractionMode::SpheresZ : ExtractionMode::SpheresXY;
        } else if (diff.yz < epsilon.yz) {
            applyMode = dimensions.x > dimensions.y ? ExtractionMode::SpheresX : ExtractionMode::SpheresYZ;
        } else {
            applyMode = ExtractionMode::SpheresXYZ;
        }
    } else {
        applyMode = _mode;
    }
    std::vector<glm::vec3> axes;
    glm::vec3 axis, axis1, axis2;
    SphereData sphere;
    switch (applyMode) {
    case ExtractionMode::None:
        break;
    case ExtractionMode::Automatic:
        break;
    case ExtractionMode::Box:
        break;
    case ExtractionMode::Sphere:
        sphere._radius = 0.5f * (dimensions.x + dimensions.y + dimensions.z) / 3.0f;
        sphere._position = glm::vec3(0.0f);
        _spheres.push_back(sphere);
        break;
    case ExtractionMode::SphereCollapse:
        sphere._radius = 0.5f * glm::min(glm::min(dimensions.x, dimensions.y), dimensions.z);
        sphere._position = glm::vec3(0.0f);
        _spheres.push_back(sphere);
        break;
    case ExtractionMode::SpheresX:
        axis = 0.5f* dimensions.x * Vectors::UNIT_NEG_X;
        axes = { axis, -axis };
        break;
    case ExtractionMode::SpheresY:
        axis = 0.5f* dimensions.y * Vectors::UNIT_NEG_Y;
        axes = { axis, -axis };
        break;
    case ExtractionMode::SpheresZ:
        axis = 0.5f* dimensions.z * Vectors::UNIT_NEG_Z;
        axes = { axis, -axis };
        break;
    case ExtractionMode::SpheresXY:
        axis1 = glm::vec3(0.5f * dimensions.x, 0.5f * dimensions.y, 0.0f);
        axis2 = glm::vec3(0.5f * dimensions.x, -0.5f * dimensions.y, 0.0f);
        axes = { axis1, axis2, -axis1, -axis2 };
        break;
    case ExtractionMode::SpheresYZ:
        axis1 = glm::vec3(0.0f, 0.5f * dimensions.y, 0.5f * dimensions.z);
        axis2 = glm::vec3(0.0f, 0.5f * dimensions.y, -0.5f * dimensions.z);
        axes = { axis1, axis2, -axis1, -axis2 };
        break;
    case ExtractionMode::SpheresXZ:
        axis1 = glm::vec3(0.5f * dimensions.x, 0.0f, 0.5f * dimensions.z);
        axis2 = glm::vec3(-0.5f * dimensions.x, 0.0f, 0.5f * dimensions.z);
        axes = { axis1, axis2, -axis1, -axis2 };
        break;
    case ExtractionMode::SpheresXYZ:
        for (size_t i = 0; i < CORNER_SIGNS.size(); i++) {
            axes.push_back(0.5f * (dimensions * CORNER_SIGNS[i]));
        }
        break;
    default:
        break;
    }
    CollapsingMode collapsingMode = CollapsingMode::None;
    if (axes.size() > 0) {
        collapsingMode = spheresFromAxes(data._relativePoints, axes, _spheres);
    }
    for (size_t i = 0; i < _spheres.size(); i++) {
        _spheres[i]._position += _midPoint;
    }
    // computing fails if the shape needs to be collapsed
    return collapsingMode;
}

MultiSphereShape::CollapsingMode MultiSphereShape::getNextCollapsingMode(ExtractionMode mode, const std::vector<SphereData>& spheres) {
    auto collapsingMode = CollapsingMode::None;
    int collapseCount = 0;
    glm::vec3 collapseVector;
    for (size_t i = 0; i < spheres.size() - 1; i++) {
        for (size_t j = i + 1; j < spheres.size(); j++) {
            size_t maxRadiusIndex = spheres[i]._radius > spheres[j]._radius ? i : j;
            auto pairVector = spheres[i]._position - spheres[j]._position;
            if (glm::length(pairVector) < 0.2f * spheres[maxRadiusIndex]._radius) {
                collapseCount++;
                collapseVector += spheres[i]._axis - spheres[j]._axis;
            }
        }
    }
    
    if (collapseCount > 0) {
        float collapseDistance = glm::length(collapseVector);
        bool allSpheresCollapse = collapseDistance < EPSILON;
        if (allSpheresCollapse) {
            collapsingMode = CollapsingMode::Sphere;
        } else {
            collapseVector = glm::normalize(collapseVector);
            bool alongAxis = collapseVector.x == 1.0f || collapseVector.y == 1.0f || collapseVector.z == 1.0f;
            bool alongPlane = collapseVector.x == 0.0f || collapseVector.y == 0.0f || collapseVector.z == 0.0f;
            int halfSphere3DCount = 4;
            int halfSphere2DCount = 2;
            bool modeSpheres3D = mode == ExtractionMode::SpheresXYZ;
            bool modeSpheres2D = mode == ExtractionMode::SpheresXY ||
                mode == ExtractionMode::SpheresYZ ||
                mode == ExtractionMode::SpheresXZ;
            bool modeSpheres1D = mode == ExtractionMode::SpheresX ||
                mode == ExtractionMode::SpheresY ||
                mode == ExtractionMode::SpheresZ;
            // SpheresXYZ will collapse along XY YZ XZ planes or X Y Z axes.
            // SpheresXY, SpheresYZ and Spheres XZ will collapse only along X Y Z axes.
            // Other occurences will be collapsed to a single sphere.
            bool isCollapseValid = (modeSpheres3D && (alongAxis || alongPlane)) ||
                                   (modeSpheres2D && (alongAxis));
            bool collapseToSphere = !isCollapseValid || (modeSpheres3D && collapseCount > halfSphere3DCount) ||
                                                        (modeSpheres2D && collapseCount > halfSphere2DCount) ||
                                                         modeSpheres1D;
            if (collapseToSphere) {
                collapsingMode = CollapsingMode::Sphere;
            } else if (modeSpheres3D) {
                if (alongAxis) {
                    if (collapseVector.x == 1.0f) {
                        collapsingMode = CollapsingMode::SpheresYZ;
                    } else if (collapseVector.y == 1.0f) {
                        collapsingMode = CollapsingMode::SpheresXZ;
                    } else if (collapseVector.z == 1.0f) {
                        collapsingMode = CollapsingMode::SpheresXY;
                    }
                } else if (alongPlane) {
                    if (collapseVector.x == 0.0f) {
                        collapsingMode = CollapsingMode::SpheresX;
                    } else if (collapseVector.y == 0.0f) {
                        collapsingMode = CollapsingMode::SpheresY;
                    } else if (collapseVector.z == 0.0f) {
                        collapsingMode = CollapsingMode::SpheresZ;
                    }
                }
            } else if (modeSpheres2D) {
                if (collapseVector.x == 1.0f) {
                    if (mode == ExtractionMode::SpheresXY) {
                        collapsingMode = CollapsingMode::SpheresY;
                    } else if (mode == ExtractionMode::SpheresXZ) {
                        collapsingMode = CollapsingMode::SpheresZ;
                    }
                } else if (collapseVector.y == 1.0f) {
                    if (mode == ExtractionMode::SpheresXY) {
                        collapsingMode = CollapsingMode::SpheresX;
                    } else if (mode == ExtractionMode::SpheresYZ) {
                        collapsingMode = CollapsingMode::SpheresZ;
                    }
                } else if (collapseVector.z == 1.0f) {
                    if (mode == ExtractionMode::SpheresXZ) {
                        collapsingMode = CollapsingMode::SpheresX;
                    } else if (mode == ExtractionMode::SpheresYZ) {
                        collapsingMode = CollapsingMode::SpheresY;
                    }
                }
            }
        }
    }
    return collapsingMode;
}

MultiSphereShape::CollapsingMode MultiSphereShape::spheresFromAxes(const std::vector<glm::vec3>& points,
                                                                   const std::vector<glm::vec3>& axes, std::vector<SphereData>& spheres) {
    float maxRadius = 0.0f;
    float maxAverageRadius = 0.0f;
    float minAverageRadius = glm::length(points[0]);
    size_t sphereCount = axes.size();
    spheres.clear();
    for (size_t j = 0; j < sphereCount; j++) {
        SphereData sphere = SphereData();
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
        maxRadius = radius > maxRadius ? radius : maxRadius;
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
    CollapsingMode collapsingMode = ExtractionMode::None;
    if (sphereCount > 1) {
        collapsingMode = getNextCollapsingMode(_mode, spheres);
    }
    return collapsingMode;
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
    std::vector<float> radiuses;
    if (_spheres.size() == 1) {
        auto sphere = _spheres[0];
        calculateSphereLines(_debugLines, sphere._position, sphere._radius);
    } else if (_spheres.size() == 2) {
        connectSpheres(0, 1);        
    } else if (_spheres.size() == 4) {
        std::vector<glm::vec3> axes;
        axes.resize(8);
        const float AXIS_DOT_THRESHOLD = 0.3f;
        for (size_t i = 0; i < CORNER_SIGNS.size(); i++) {
            for (size_t j = 0; j < _spheres.size(); j++) {
                auto axis = _spheres[j]._position - _midPoint;
                if (glm::length(axes[i]) == 0.0f && glm::length(axis) > 0.0f && glm::dot(CORNER_SIGNS[i], glm::normalize(axis)) > AXIS_DOT_THRESHOLD) {
                    radiuses.push_back(_spheres[j]._radius);
                    axes[i] = axis;
                    break;
                }
            }
        }
        calculateChamferBox(_debugLines, radiuses, axes, _midPoint);
    } else if (_spheres.size() == 8) {
        std::vector<glm::vec3> axes;
        for (size_t i = 0; i < _spheres.size(); i++) {
            radiuses.push_back(_spheres[i]._radius);
            axes.push_back(_spheres[i]._position - _midPoint);
        }
        calculateChamferBox(_debugLines, radiuses, axes, _midPoint);
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

void MultiSphereShape::calculateChamferBox(std::vector<std::pair<glm::vec3, glm::vec3>>& outLines, const std::vector<float>& radiuses, const std::vector<glm::vec3>& axes, const glm::vec3& translation) {
    if (radiuses.size() == 0) {
        return;
    }

    std::vector<std::pair<glm::vec3, glm::vec3>> sphereLines;
    calculateSphereLines(sphereLines, glm::vec3(0.0f), radiuses[0]);

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
        regions[i].scale(radiuses[i]/radiuses[0]);
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