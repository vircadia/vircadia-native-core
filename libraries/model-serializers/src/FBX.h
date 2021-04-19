//
//  FBX.h
//  libraries/model-serializers/src
//
//  Created by Ryan Huffman on 9/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FBX_h_
#define hifi_FBX_h_

#include <QMetaType>
#include <QVariant>
#include <QVector>

#include <glm/glm.hpp>

#include <shared/HifiTypes.h>

// See comment in FBXSerializer::parseFBX().
static const int FBX_HEADER_BYTES_BEFORE_VERSION = 23;
static const hifi::ByteArray FBX_BINARY_PROLOG("Kaydara FBX Binary  ");
static const hifi::ByteArray FBX_BINARY_PROLOG2("\0\x1a\0", 3);
static const quint32 FBX_VERSION_2015 = 7400;
static const quint32 FBX_VERSION_2016 = 7500;

static const int32_t FBX_PROPERTY_UNCOMPRESSED_FLAG = 0;
static const int32_t FBX_PROPERTY_COMPRESSED_FLAG = 1;

// The version of the FBX node containing the draco mesh. See also: DRACO_MESH_VERSION in HFM.h
static const int FBX_DRACO_MESH_VERSION = 2;

class FBXNode;
using FBXNodeList = QList<FBXNode>;


/// A node within an FBX document.
class FBXNode {
public:
    hifi::ByteArray name;
    QVariantList properties;
    FBXNodeList children;
};

#endif // hifi_FBX_h_
