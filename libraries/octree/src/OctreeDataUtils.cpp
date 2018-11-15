//
//  OctreeDataUtils.cpp
//  libraries/octree/src
//
//  Created by Ryan Huffman 2018-02-26
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include "OctreeDataUtils.h"
#include "OctreeEntitiesFileParser.h"

#include <Gzip.h>
#include <udt/PacketHeaders.h>

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

bool OctreeUtils::RawOctreeData::readOctreeDataInfoFromMap(const QVariantMap& map) {
    if (map.contains("Id") && map.contains("DataVersion") && map.contains("Version")) {
        id = map["Id"].toUuid();
        dataVersion = map["DataVersion"].toInt();
        version = map["Version"].toInt();
    }
    readSubclassData(map);
    return true;
}

bool OctreeUtils::RawOctreeData::readOctreeDataInfoFromData(QByteArray data) {
    QByteArray jsonData;
    if (gunzip(data, jsonData)) {
        data = jsonData;
    }

    OctreeEntitiesFileParser jsonParser;
    jsonParser.setEntitiesString(data);
    QVariantMap entitiesMap;
    if (!jsonParser.parseEntities(entitiesMap)) {
        qCritical() << "Can't parse Entities JSON: " << jsonParser.getErrorString().c_str();
        return false;
    }

    return readOctreeDataInfoFromMap(entitiesMap);
}

// Reads octree file and parses it into a RawOctreeData object.
// Returns false if readOctreeFile fails.
bool OctreeUtils::RawOctreeData::readOctreeDataInfoFromFile(QString path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open json file for reading: " << path;
        return false;
    }

    QByteArray data = file.readAll();

    return readOctreeDataInfoFromData(data);
}

QByteArray OctreeUtils::RawOctreeData::toByteArray() {
    QByteArray jsonString;

    jsonString += QString("{\n  \"DataVersion\": %1,\n").arg(dataVersion);

    writeSubclassData(jsonString);

    jsonString += QString(",\n  \"Id\": \"%1\",\n  \"Version\": %2\n}").arg(id.toString()).arg(version);

    return jsonString;
}

QByteArray OctreeUtils::RawOctreeData::toGzippedByteArray() {
    auto data = toByteArray();
    QByteArray gzData;

    if (!gzip(data, gzData, -1)) {
        qCritical("Unable to gzip data while converting json.");
        return QByteArray();
    }

    return gzData;
}

PacketType OctreeUtils::RawOctreeData::dataPacketType() const {
    Q_ASSERT(false);
    qCritical() << "Attemping to read packet type for incomplete base type 'RawOctreeData'";
    return (PacketType)0;
}

void OctreeUtils::RawOctreeData::resetIdAndVersion() {
    id = QUuid::createUuid();
    dataVersion = OctreeUtils::INITIAL_VERSION;
    qDebug() << "Reset octree data to: " << id << dataVersion;
}

void OctreeUtils::RawEntityData::readSubclassData(const QVariantMap& root) {
    variantEntityData = root["Entities"].toList();
}

void OctreeUtils::RawEntityData::writeSubclassData(QByteArray& root) const {
    root += "  \"Entities\": [";
    for (auto entityIter = variantEntityData.begin(); entityIter != variantEntityData.end(); ++entityIter) {
        if (entityIter != variantEntityData.begin()) {
            root += ",";
        }
        root += "\n    ";
        // Convert to string and remove trailing LF.
        root += QJsonDocument(entityIter->toJsonObject()).toJson().chopped(1);
    }
    root += "]";
}

PacketType OctreeUtils::RawEntityData::dataPacketType() const { return PacketType::EntityData; }
