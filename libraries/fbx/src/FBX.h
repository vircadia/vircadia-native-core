//
//  FBX.h
//  libraries/fbx/src
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
#include <QVarLengthArray>
#include <QVariant>
#include <QVector>

#include <glm/glm.hpp>

#if defined(Q_OS_ANDROID)
#define FBX_PACK_NORMALS 0
#else
#define FBX_PACK_NORMALS 1
#endif

#if FBX_PACK_NORMALS
using NormalType = glm::uint32;
#define FBX_NORMAL_ELEMENT gpu::Element::VEC4F_NORMALIZED_XYZ10W2
#else
using NormalType = glm::vec3;
#define FBX_NORMAL_ELEMENT gpu::Element::VEC3F_XYZ
#endif

// See comment in FBXReader::parseFBX().
static const int FBX_HEADER_BYTES_BEFORE_VERSION = 23;
static const QByteArray FBX_BINARY_PROLOG("Kaydara FBX Binary  ");
static const QByteArray FBX_BINARY_PROLOG2("\0\x1a\0", 3);
static const quint32 FBX_VERSION_2015 = 7400;
static const quint32 FBX_VERSION_2016 = 7500;

static const int DRACO_BEGIN_CUSTOM_HIFI_ATTRIBUTES = 1000;
static const int DRACO_ATTRIBUTE_MATERIAL_ID = DRACO_BEGIN_CUSTOM_HIFI_ATTRIBUTES;
static const int DRACO_ATTRIBUTE_TEX_COORD_1 = DRACO_BEGIN_CUSTOM_HIFI_ATTRIBUTES + 1;
static const int DRACO_ATTRIBUTE_ORIGINAL_INDEX = DRACO_BEGIN_CUSTOM_HIFI_ATTRIBUTES + 2;

static const int32_t FBX_PROPERTY_UNCOMPRESSED_FLAG = 0;
static const int32_t FBX_PROPERTY_COMPRESSED_FLAG = 1;

class FBXNode;
using FBXNodeList = QList<FBXNode>;


/// A node within an FBX document.
class FBXNode {
public:
    QByteArray name;
    QVariantList properties;
    FBXNodeList children;
};

#endif // hifi_FBX_h_
