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

#include <Gzip.h>
#include <udt/PacketHeaders.h>

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

// Reads octree file and parses it into a QJsonDocument. Handles both gzipped and non-gzipped files.
// Returns true if the file was successfully opened and parsed, otherwise false.
// Example failures: file does not exist, gzipped file cannot be unzipped, invalid JSON.
bool readOctreeFile(QString path, QJsonDocument* doc) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open json file for reading: " << path;
        return false;
    }

    QByteArray data = file.readAll();
    QByteArray jsonData;

    if (!gunzip(data, jsonData)) {
        jsonData = data;
    }

    *doc = QJsonDocument::fromJson(jsonData);
    return !doc->isNull();
}

bool OctreeUtils::RawOctreeData::readOctreeDataInfoFromJSON(QJsonObject root) {
    if (root.contains("Id") && root.contains("DataVersion")) {
        id = root["Id"].toVariant().toUuid();
        version = root["DataVersion"].toInt();
    }
    readSubclassData(root);
    return true;
}

bool OctreeUtils::RawOctreeData::readOctreeDataInfoFromData(QByteArray data) {
    QByteArray jsonData;
    if (gunzip(data, jsonData)) {
        data = jsonData;
    }

    auto doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        return false;
    }

    auto root = doc.object();
    return readOctreeDataInfoFromJSON(root);
}

// Reads octree file and parses it into a RawOctreeData object.
// Returns false if readOctreeFile fails.
bool OctreeUtils::RawOctreeData::readOctreeDataInfoFromFile(QString path) {
    QJsonDocument doc;
    if (!readOctreeFile(path, &doc)) {
        return false;
    }

    auto root = doc.object();
    return readOctreeDataInfoFromJSON(root);
}

QByteArray OctreeUtils::RawOctreeData::toByteArray() {
    const auto protocolVersion = (int)versionForPacketType((PacketTypeEnum::Value)dataPacketType());
    QJsonObject obj {
        { "DataVersion", QJsonValue((qint64)version) },
        { "Id", QJsonValue(id.toString()) },
        { "Version", protocolVersion },
    };

    writeSubclassData(obj);

    QJsonDocument doc;
    doc.setObject(obj);

    return doc.toJson();
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
    version = OctreeUtils::INITIAL_VERSION;
    qDebug() << "Reset octree data to: " << id << version;
}

void OctreeUtils::RawEntityData::readSubclassData(const QJsonObject& root) {
    if (root.contains("Entities")) {
        entityData = root["Entities"].toArray();
    }
}

void OctreeUtils::RawEntityData::writeSubclassData(QJsonObject& root) const {
    root["Entities"] = entityData;
}

PacketType OctreeUtils::RawEntityData::dataPacketType() const { return PacketType::EntityData; }