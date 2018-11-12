//
//  OctreeDataUtils.h
//  libraries/octree/src
//
//  Created by Ryan Huffman 2018-02-26
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#ifndef hifi_OctreeDataUtils_h
#define hifi_OctreeDataUtils_h

#include <udt/PacketHeaders.h>

#include <QJsonObject>
#include <QUuid>
#include <QJsonArray>

namespace OctreeUtils {

using Version = int64_t;
constexpr Version INITIAL_VERSION = 0;

//using PacketType = uint8_t;

// RawOctreeData is an intermediate format between JSON and a fully deserialized Octree.
class RawOctreeData {
public:
    QUuid id { QUuid() };
    Version dataVersion { -1 };
    Version version { -1 };

    virtual PacketType dataPacketType() const;

    virtual void readSubclassData(const QVariantMap& root) { }
    virtual void writeSubclassData(QByteArray& root) const { }

    void resetIdAndVersion();
    QByteArray toByteArray();
    QByteArray toGzippedByteArray();

    bool readOctreeDataInfoFromData(QByteArray data);
    bool readOctreeDataInfoFromFile(QString path);
    bool readOctreeDataInfoFromMap(const QVariantMap& map);
};

class RawEntityData : public RawOctreeData {
public:
    PacketType dataPacketType() const override;
    void readSubclassData(const QVariantMap& root) override;
    void writeSubclassData(QByteArray& root) const override;

    QVariantList variantEntityData;
};

}

#endif // hifi_OctreeDataUtils_h