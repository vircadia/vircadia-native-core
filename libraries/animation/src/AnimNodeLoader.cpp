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

typedef AnimNode::Pointer (*NodeLoaderFunc)(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);

static AnimNode::Pointer loadClipNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);
static AnimNode::Pointer loadBlendLinearNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);
static AnimNode::Pointer loadOverlayNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);

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
                              << ", url =" << URL.toDisplayString();    \
        return nullptr;                                                 \
    }                                                                   \
    QString NAME = NAME##_VAL.toString()

#define READ_OPTIONAL_STRING(NAME, JSON_OBJ)                            \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    QString NAME;                                                       \
    if (NAME##_VAL.isString()) {                                        \
        NAME = NAME##_VAL.toString();                                   \
    }

#define READ_BOOL(NAME, JSON_OBJ, ID, URL)                              \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    if (!NAME##_VAL.isBool()) {                                         \
        qCCritical(animation) << "AnimNodeLoader, error reading bool"   \
                              << #NAME << ", id =" << ID                \
                              << ", url =" << URL.toDisplayString();    \
        return nullptr;                                                 \
    }                                                                   \
    bool NAME = NAME##_VAL.toBool()

#define READ_FLOAT(NAME, JSON_OBJ, ID, URL)                             \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    if (!NAME##_VAL.isDouble()) {                                       \
        qCCritical(animation) << "AnimNodeLoader, error reading double" \
                              << #NAME << "id =" << ID                  \
                              << ", url =" << URL.toDisplayString();    \
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

static AnimNode::Pointer loadNode(const QJsonObject& jsonObj, const QUrl& jsonUrl) {
    auto idVal = jsonObj.value("id");
    if (!idVal.isString()) {
        qCCritical(animation) << "AnimNodeLoader, bad string \"id\", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }
    QString id = idVal.toString();

    auto typeVal = jsonObj.value("type");
    if (!typeVal.isString()) {
        qCCritical(animation) << "AnimNodeLoader, bad object \"type\", id =" << id << ", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }
    QString typeStr = typeVal.toString();
    AnimNode::Type type = stringToEnum(typeStr);
    if (type == AnimNode::NumTypes) {
        qCCritical(animation) << "AnimNodeLoader, unknown node type" << typeStr << ", id =" << id << ", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }

    auto dataValue = jsonObj.value("data");
    if (!dataValue.isObject()) {
        qCCritical(animation) << "AnimNodeLoader, bad string \"data\", id =" << id << ", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }
    auto dataObj = dataValue.toObject();

    assert((int)type >= 0 && type < AnimNode::NumTypes);
    auto node = nodeLoaderFuncs[type](dataObj, id, jsonUrl);

    auto childrenValue = jsonObj.value("children");
    if (!childrenValue.isArray()) {
        qCCritical(animation) << "AnimNodeLoader, bad array \"children\", id =" << id << ", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }
    auto childrenAry = childrenValue.toArray();
    for (const auto& childValue : childrenAry) {
        if (!childValue.isObject()) {
            qCCritical(animation) << "AnimNodeLoader, bad object in \"children\", id =" << id << ", url =" << jsonUrl.toDisplayString();
            return nullptr;
        }
        node->addChild(loadNode(childValue.toObject(), jsonUrl));
    }
    return node;
}

static AnimNode::Pointer loadClipNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) {

    READ_STRING(url, jsonObj, id, jsonUrl);
    READ_FLOAT(startFrame, jsonObj, id, jsonUrl);
    READ_FLOAT(endFrame, jsonObj, id, jsonUrl);
    READ_FLOAT(timeScale, jsonObj, id, jsonUrl);
    READ_BOOL(loopFlag, jsonObj, id, jsonUrl);

    READ_OPTIONAL_STRING(startFrameVar, jsonObj);
    READ_OPTIONAL_STRING(endFrameVar, jsonObj);
    READ_OPTIONAL_STRING(timeScaleVar, jsonObj);
    READ_OPTIONAL_STRING(loopFlagVar, jsonObj);

    auto node = std::make_shared<AnimClip>(id.toStdString(), url.toStdString(), startFrame, endFrame, timeScale, loopFlag);

    if (!startFrameVar.isEmpty()) {
        node->setStartFrameVar(startFrameVar.toStdString());
    }
    if (!endFrameVar.isEmpty()) {
        node->setEndFrameVar(endFrameVar.toStdString());
    }
    if (!timeScaleVar.isEmpty()) {
        node->setTimeScaleVar(timeScaleVar.toStdString());
    }
    if (!loopFlagVar.isEmpty()) {
        node->setLoopFlagVar(loopFlagVar.toStdString());
    }

    return node;
}

static AnimNode::Pointer loadBlendLinearNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) {

    READ_FLOAT(alpha, jsonObj, id, jsonUrl);

    READ_OPTIONAL_STRING(alphaVar, jsonObj);

    auto node = std::make_shared<AnimBlendLinear>(id.toStdString(), alpha);

    if (!alphaVar.isEmpty()) {
        node->setAlphaVar(alphaVar.toStdString());
    }

    return node;
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

static AnimNode::Pointer loadOverlayNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) {

    READ_STRING(boneSet, jsonObj, id, jsonUrl);
    READ_FLOAT(alpha, jsonObj, id, jsonUrl);

    auto boneSetEnum = stringToBoneSetEnum(boneSet);
    if (boneSetEnum == AnimOverlay::NumBoneSets) {
        qCCritical(animation) << "AnimNodeLoader, unknown bone set =" << boneSet << ", defaulting to \"fullBody\", url =" << jsonUrl.toDisplayString();
        boneSetEnum = AnimOverlay::FullBodyBoneSet;
    }

    READ_OPTIONAL_STRING(boneSetVar, jsonObj);
    READ_OPTIONAL_STRING(alphaVar, jsonObj);

    auto node = std::make_shared<AnimOverlay>(id.toStdString(), boneSetEnum, alpha);

    if (!boneSetVar.isEmpty()) {
        node->setBoneSetVar(boneSetVar.toStdString());
    }
    if (!alphaVar.isEmpty()) {
        node->setAlphaVar(alphaVar.toStdString());
    }

    return node;
}

AnimNodeLoader::AnimNodeLoader(const QUrl& url) :
    _url(url),
    _resource(nullptr) {

    _resource = new Resource(url);
    connect(_resource, SIGNAL(loaded(QNetworkReply&)), SLOT(onRequestDone(QNetworkReply&)));
    connect(_resource, SIGNAL(failed(QNetworkReply::NetworkError)), SLOT(onRequestError(QNetworkReply::NetworkError)));
}

AnimNode::Pointer AnimNodeLoader::load(const QByteArray& contents, const QUrl& jsonUrl) {

    // convert string into a json doc
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(contents, &error);
    if (error.error != QJsonParseError::NoError) {
        qCCritical(animation) << "AnimNodeLoader, failed to parse json, error =" << error.errorString() << ", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }
    QJsonObject obj = doc.object();

    // version
    QJsonValue versionVal = obj.value("version");
    if (!versionVal.isString()) {
        qCCritical(animation) << "AnimNodeLoader, bad string \"version\", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }
    QString version = versionVal.toString();

    // check version
    if (version != "1.0") {
        qCCritical(animation) << "AnimNodeLoader, bad version number" << version << "expected \"1.0\", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }

    // root
    QJsonValue rootVal = obj.value("root");
    if (!rootVal.isObject()) {
        qCCritical(animation) << "AnimNodeLoader, bad object \"root\", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }

    return loadNode(rootVal.toObject(), jsonUrl);
}

void AnimNodeLoader::onRequestDone(QNetworkReply& request) {
    auto node = load(request.readAll(), _url);
    emit success(node);
}

void AnimNodeLoader::onRequestError(QNetworkReply::NetworkError netError) {
    emit error((int)netError, "Resource download error");
}
