//
//  AnimNodeLoader.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonarray.h>
#include <qfile.h>

#include "AnimNode.h"
#include "AnimClip.h"
#include "AnimBlendLinear.h"
#include "AnimationLogging.h"
#include "AnimOverlay.h"
#include "AnimNodeLoader.h"

struct TypeInfo {
    AnimNode::Type type;
    const char* str;
};

// This will result in a compile error if someone adds a new
// item to the AnimNode::Type enum.  This is by design.
static TypeInfo typeInfoArray[AnimNode::NumTypes] = {
    { AnimNode::ClipType, "clip" },
    { AnimNode::BlendLinearType, "blendLinear" },
    { AnimNode::OverlayType, "overlay" }
};

typedef AnimNode::Pointer (*NodeLoaderFunc)(const QJsonObject& jsonObj, const QString& id, const QString& jsonUrl);

static AnimNode::Pointer loadClipNode(const QJsonObject& jsonObj, const QString& id, const QString& jsonUrl);
static AnimNode::Pointer loadBlendLinearNode(const QJsonObject& jsonObj, const QString& id, const QString& jsonUrl);
static AnimNode::Pointer loadOverlayNode(const QJsonObject& jsonObj, const QString& id, const QString& jsonUrl);

static NodeLoaderFunc nodeLoaderFuncs[AnimNode::NumTypes] = {
    loadClipNode,
    loadBlendLinearNode,
    loadOverlayNode
};

#define READ_STRING(NAME, JSON_OBJ, ID, URL)                            \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    if (!NAME##_VAL.isString()) {                                       \
        qCCritical(animation) << "AnimNodeLoader, error reading string" \
                              << #NAME << ", id =" << ID                \
                              << ", url =" << URL;                      \
        return nullptr;                                                 \
    }                                                                   \
    QString NAME = NAME##_VAL.toString()

#define READ_BOOL(NAME, JSON_OBJ, ID, URL)                              \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    if (!NAME##_VAL.isBool()) {                                         \
        qCCritical(animation) << "AnimNodeLoader, error reading bool"   \
                              << #NAME << ", id =" << ID                \
                              << ", url =" << URL;                      \
        return nullptr;                                                 \
    }                                                                   \
    bool NAME = NAME##_VAL.toBool()

#define READ_FLOAT(NAME, JSON_OBJ, ID, URL)                             \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    if (!NAME##_VAL.isDouble()) {                                       \
        qCCritical(animation) << "AnimNodeLoader, error reading double" \
                              << #NAME << "id =" << ID                  \
                              << ", url =" << URL;                      \
        return nullptr;                                                 \
    }                                                                   \
    float NAME = (float)NAME##_VAL.toDouble()

static AnimNode::Type stringToEnum(const QString& str) {
    for (int i = 0; i < AnimNode::NumTypes; i++ ) {
        if (str == typeInfoArray[i].str) {
            return typeInfoArray[i].type;
        }
    }
    return AnimNode::NumTypes;
}

static AnimNode::Pointer loadNode(const QJsonObject& jsonObj, const QString& jsonUrl) {
    auto idVal = jsonObj.value("id");
    if (!idVal.isString()) {
        qCCritical(animation) << "AnimNodeLoader, bad string \"id\", url =" << jsonUrl;
        return nullptr;
    }
    QString id = idVal.toString();

    auto typeVal = jsonObj.value("type");
    if (!typeVal.isString()) {
        qCCritical(animation) << "AnimNodeLoader, bad object \"type\", id =" << id << ", url =" << jsonUrl;
        return nullptr;
    }
    QString typeStr = typeVal.toString();
    AnimNode::Type type = stringToEnum(typeStr);
    if (type == AnimNode::NumTypes) {
        qCCritical(animation) << "AnimNodeLoader, unknown node type" << typeStr << ", id =" << id << ", url =" << jsonUrl;
        return nullptr;
    }

    auto dataValue = jsonObj.value("data");
    if (!dataValue.isObject()) {
        qCCritical(animation) << "AnimNodeLoader, bad string \"data\", id =" << id << ", url =" << jsonUrl;
        return nullptr;
    }
    auto dataObj = dataValue.toObject();

    assert((int)type >= 0 && type < AnimNode::NumTypes);
    auto node = nodeLoaderFuncs[type](dataObj, id, jsonUrl);

    auto childrenValue = jsonObj.value("children");
    if (!childrenValue.isArray()) {
        qCCritical(animation) << "AnimNodeLoader, bad array \"children\", id =" << id << ", url =" << jsonUrl;
        return nullptr;
    }
    auto childrenAry = childrenValue.toArray();
    for (const auto& childValue : childrenAry) {
        if (!childValue.isObject()) {
            qCCritical(animation) << "AnimNodeLoader, bad object in \"children\", id =" << id << ", url =" << jsonUrl;
            return nullptr;
        }
        node->addChild(loadNode(childValue.toObject(), jsonUrl));
    }
    return node;
}

static AnimNode::Pointer loadClipNode(const QJsonObject& jsonObj, const QString& id, const QString& jsonUrl) {

    READ_STRING(url, jsonObj, id, jsonUrl);
    READ_FLOAT(startFrame, jsonObj, id, jsonUrl);
    READ_FLOAT(endFrame, jsonObj, id, jsonUrl);
    READ_FLOAT(timeScale, jsonObj, id, jsonUrl);
    READ_BOOL(loopFlag, jsonObj, id, jsonUrl);

    return std::make_shared<AnimClip>(id.toStdString(), url.toStdString(), startFrame, endFrame, timeScale, loopFlag);
}

static AnimNode::Pointer loadBlendLinearNode(const QJsonObject& jsonObj, const QString& id, const QString& jsonUrl) {

    READ_FLOAT(alpha, jsonObj, id, jsonUrl);

    return std::make_shared<AnimBlendLinear>(id.toStdString(), alpha);
}

static const char* boneSetStrings[AnimOverlay::NumBoneSets] = {
    "fullBody",
    "upperBody",
    "lowerBody",
    "rightArm",
    "leftArm",
    "aboveTheHead",
    "belowTheHead",
    "headOnly",
    "spineOnly",
    "empty"
};

static AnimOverlay::BoneSet stringToBoneSetEnum(const QString& str) {
    for (int i = 0; i < (int)AnimOverlay::NumBoneSets; i++) {
        if (str == boneSetStrings[i]) {
            return (AnimOverlay::BoneSet)i;
        }
    }
    return AnimOverlay::NumBoneSets;
}

static AnimNode::Pointer loadOverlayNode(const QJsonObject& jsonObj, const QString& id, const QString& jsonUrl) {

    READ_STRING(boneSet, jsonObj, id, jsonUrl);

    auto boneSetEnum = stringToBoneSetEnum(boneSet);
    if (boneSetEnum == AnimOverlay::NumBoneSets) {
        qCCritical(animation) << "AnimNodeLoader, unknown bone set =" << boneSet << ", defaulting to \"fullBody\", url =" << jsonUrl;
        boneSetEnum = AnimOverlay::FullBody;
    }

    return std::make_shared<AnimBlendLinear>(id.toStdString(), boneSetEnum);
}

AnimNodeLoader::AnimNodeLoader() {

}

AnimNode::Pointer AnimNodeLoader::load(const std::string& filename) const {
    // load entire file into a string.
    QString jsonUrl = QString::fromStdString(filename);
    QFile file;
    file.setFileName(jsonUrl);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCCritical(animation) << "AnimNodeLoader, could not open url =" << jsonUrl;
        return nullptr;
    }
    QString contents = file.readAll();
    file.close();

    // convert string into a json doc
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(contents.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCCritical(animation) << "AnimNodeLoader, failed to parse json, error =" << error.errorString() << ", url =" << jsonUrl;
        return nullptr;
    }
    QJsonObject obj = doc.object();

    // version
    QJsonValue versionVal = obj.value("version");
    if (!versionVal.isString()) {
        qCCritical(animation) << "AnimNodeLoader, bad string \"version\", url =" << jsonUrl;
        return nullptr;
    }
    QString version = versionVal.toString();

    // check version
    if (version != "1.0") {
        qCCritical(animation) << "AnimNodeLoader, bad version number" << version << "expected \"1.0\", url =" << jsonUrl;
        return nullptr;
    }

    // root
    QJsonValue rootVal = obj.value("root");
    if (!rootVal.isObject()) {
        qCCritical(animation) << "AnimNodeLoader, bad object \"root\", url =" << jsonUrl;
        return nullptr;
    }

    return loadNode(rootVal.toObject(), jsonUrl);
}
