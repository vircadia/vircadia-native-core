//
//  OctreeUtils.cpp
//  libraries/octree/src
//
//  Created by Andrew Meadows 2016.03.04
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OctreeUtils.h"

#include <mutex>

#include <glm/glm.hpp>

#include <AABox.h>
#include <Gzip.h>

#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

float calculateRenderAccuracy(const glm::vec3& position,
        const AABox& bounds,
        float octreeSizeScale,
        int boundaryLevelAdjust) {
    float largestDimension = bounds.getLargestDimension();

    const float maxScale = (float)TREE_SCALE;
    float visibleDistanceAtMaxScale = boundaryDistanceForRenderLevel(boundaryLevelAdjust, octreeSizeScale) / OCTREE_TO_MESH_RATIO;

    static std::once_flag once;
    static QMap<float, float> shouldRenderTable;
    std::call_once(once, [&] {
        float SMALLEST_SCALE_IN_TABLE = 0.001f; // 1mm is plenty small
        float scale = maxScale;
        float factor = 1.0f;

        while (scale > SMALLEST_SCALE_IN_TABLE) {
            scale /= 2.0f;
            factor /= 2.0f;
            shouldRenderTable[scale] = factor;
        }
    });

    float closestScale = maxScale;
    float visibleDistanceAtClosestScale = visibleDistanceAtMaxScale;
    QMap<float, float>::const_iterator lowerBound = shouldRenderTable.lowerBound(largestDimension);
    if (lowerBound != shouldRenderTable.constEnd()) {
        closestScale = lowerBound.key();
        visibleDistanceAtClosestScale = visibleDistanceAtMaxScale * lowerBound.value();
    }

    if (closestScale < largestDimension) {
        visibleDistanceAtClosestScale *= 2.0f;
    }

    // FIXME - for now, it's either visible or not visible. We want to adjust this to eventually return
    // a floating point for objects that have small angular size to indicate that they may be rendered
    // with lower preciscion
    float distanceToCamera = glm::length(bounds.calcCenter() - position);
    return (distanceToCamera <= visibleDistanceAtClosestScale) ? 1.0f : 0.0f;
}

float boundaryDistanceForRenderLevel(unsigned int renderLevel, float voxelSizeScale) {
    return voxelSizeScale / powf(2.0f, renderLevel);
}

float getPerspectiveAccuracyAngle(float octreeSizeScale, int boundaryLevelAdjust) {
    const float maxScale = (float)TREE_SCALE;
    float visibleDistanceAtMaxScale = boundaryDistanceForRenderLevel(boundaryLevelAdjust, octreeSizeScale) / OCTREE_TO_MESH_RATIO;
    return atan(maxScale / visibleDistanceAtMaxScale);
}

float getOrthographicAccuracySize(float octreeSizeScale, int boundaryLevelAdjust) {
    // Smallest visible element is 1cm
    const float smallestSize = 0.01f;
    return (smallestSize * MAX_VISIBILITY_DISTANCE_FOR_UNIT_ELEMENT) / boundaryDistanceForRenderLevel(boundaryLevelAdjust, octreeSizeScale);
}

bool OctreeUtils::readOctreeFile(QString path, QJsonDocument* doc) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open json file for reading: " << path;
        return false;
    }

    QByteArray data = file.readAll();
    QByteArray jsonData;

    if (path.endsWith(".json.gz")) {
        if (!gunzip(data, jsonData)) {
            qCritical() << "json File not in gzip format: " << path;
            return false;
        }
    } else {
        jsonData = data;
    }

    *doc = QJsonDocument::fromJson(jsonData);
    return !doc->isNull();
}

bool readOctreeDataInfoFromJSON(QJsonObject root, OctreeUtils::RawOctreeData* octreeData) {
    if (root.contains("Id") && root.contains("DataVersion")) {
        octreeData->id = root["Id"].toVariant().toUuid();
        octreeData->version = root["DataVersion"].toInt();
    }
    if (root.contains("Entities")) {
        octreeData->octreeData = root["Entities"].toArray();
    }
    return true;
}

bool OctreeUtils::readOctreeDataInfoFromData(QByteArray data, OctreeUtils::RawOctreeData* octreeData) {
    QByteArray jsonData;
    if (gunzip(data, jsonData)) {
        data = jsonData;
    }

    auto doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        return false;
    }

    auto root = doc.object();
    return readOctreeDataInfoFromJSON(root, octreeData);
}

bool OctreeUtils::readOctreeDataInfoFromFile(QString path, OctreeUtils::RawOctreeData* octreeData) {
    QJsonDocument doc;
    if (!OctreeUtils::readOctreeFile(path, &doc)) {
        return false;
    }

    auto root = doc.object();
    return readOctreeDataInfoFromJSON(root, octreeData);
}

QByteArray OctreeUtils::RawOctreeData::toByteArray() {
    QJsonObject obj {
        { "DataVersion", QJsonValue((qint64)version) },
        { "Id", QJsonValue(id.toString()) },
        { "Version", QJsonValue(5) },
        { "Entities", octreeData }
    };

    QJsonDocument doc;
    doc.setObject(obj);

    return doc.toJson();
}
