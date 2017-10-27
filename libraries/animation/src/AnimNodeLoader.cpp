//
//  AnimNodeLoader.cpp
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include "AnimNode.h"
#include "AnimClip.h"
#include "AnimBlendLinear.h"
#include "AnimBlendLinearMove.h"
#include "AnimationLogging.h"
#include "AnimOverlay.h"
#include "AnimNodeLoader.h"
#include "AnimStateMachine.h"
#include "AnimManipulator.h"
#include "AnimInverseKinematics.h"
#include "AnimDefaultPose.h"

using NodeLoaderFunc = AnimNode::Pointer (*)(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);
using NodeProcessFunc = bool (*)(AnimNode::Pointer node, const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);

// factory functions
static AnimNode::Pointer loadClipNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);
static AnimNode::Pointer loadBlendLinearNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);
static AnimNode::Pointer loadBlendLinearMoveNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);
static AnimNode::Pointer loadOverlayNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);
static AnimNode::Pointer loadStateMachineNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);
static AnimNode::Pointer loadManipulatorNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);
static AnimNode::Pointer loadInverseKinematicsNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);
static AnimNode::Pointer loadDefaultPoseNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);

// called after children have been loaded
// returns node on success, nullptr on failure.
static bool processDoNothing(AnimNode::Pointer node, const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) { return true; }
bool processStateMachineNode(AnimNode::Pointer node, const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl);

static const char* animNodeTypeToString(AnimNode::Type type) {
    switch (type) {
    case AnimNode::Type::Clip: return "clip";
    case AnimNode::Type::BlendLinear: return "blendLinear";
    case AnimNode::Type::BlendLinearMove: return "blendLinearMove";
    case AnimNode::Type::Overlay: return "overlay";
    case AnimNode::Type::StateMachine: return "stateMachine";
    case AnimNode::Type::Manipulator: return "manipulator";
    case AnimNode::Type::InverseKinematics: return "inverseKinematics";
    case AnimNode::Type::DefaultPose: return "defaultPose";
    case AnimNode::Type::NumTypes: return nullptr;
    };
    return nullptr;
}

static AnimNode::Type stringToAnimNodeType(const QString& str) {
    // O(n), move to map when number of types becomes large.
    const int NUM_TYPES = static_cast<int>(AnimNode::Type::NumTypes);
    for (int i = 0; i < NUM_TYPES; i++) {
        AnimNode::Type type = static_cast<AnimNode::Type>(i);
        if (str == animNodeTypeToString(type)) {
            return type;
        }
    }
    return AnimNode::Type::NumTypes;
}

static AnimStateMachine::InterpType stringToInterpType(const QString& str) {
    if (str == "snapshotBoth") {
        return AnimStateMachine::InterpType::SnapshotBoth;
    } else if (str == "snapshotPrev") {
        return AnimStateMachine::InterpType::SnapshotPrev;
    } else {
        return AnimStateMachine::InterpType::NumTypes;
    }
}

static const char* animManipulatorJointVarTypeToString(AnimManipulator::JointVar::Type type) {
    switch (type) {
    case AnimManipulator::JointVar::Type::Absolute: return "absolute";
    case AnimManipulator::JointVar::Type::Relative: return "relative";
    case AnimManipulator::JointVar::Type::UnderPose: return "underPose";
    case AnimManipulator::JointVar::Type::Default: return "default";
    case AnimManipulator::JointVar::Type::NumTypes: return nullptr;
    };
    return nullptr;
}

static AnimManipulator::JointVar::Type stringToAnimManipulatorJointVarType(const QString& str) {
    // O(n), move to map when number of types becomes large.
    const int NUM_TYPES = static_cast<int>(AnimManipulator::JointVar::Type::NumTypes);
    for (int i = 0; i < NUM_TYPES; i++) {
        AnimManipulator::JointVar::Type type = static_cast<AnimManipulator::JointVar::Type>(i);
        if (str == animManipulatorJointVarTypeToString(type)) {
            return type;
        }
    }
    return AnimManipulator::JointVar::Type::NumTypes;
}

static NodeLoaderFunc animNodeTypeToLoaderFunc(AnimNode::Type type) {
    switch (type) {
    case AnimNode::Type::Clip: return loadClipNode;
    case AnimNode::Type::BlendLinear: return loadBlendLinearNode;
    case AnimNode::Type::BlendLinearMove: return loadBlendLinearMoveNode;
    case AnimNode::Type::Overlay: return loadOverlayNode;
    case AnimNode::Type::StateMachine: return loadStateMachineNode;
    case AnimNode::Type::Manipulator: return loadManipulatorNode;
    case AnimNode::Type::InverseKinematics: return loadInverseKinematicsNode;
    case AnimNode::Type::DefaultPose: return loadDefaultPoseNode;
    case AnimNode::Type::NumTypes: return nullptr;
    };
    return nullptr;
}

static NodeProcessFunc animNodeTypeToProcessFunc(AnimNode::Type type) {
    switch (type) {
    case AnimNode::Type::Clip: return processDoNothing;
    case AnimNode::Type::BlendLinear: return processDoNothing;
    case AnimNode::Type::BlendLinearMove: return processDoNothing;
    case AnimNode::Type::Overlay: return processDoNothing;
    case AnimNode::Type::StateMachine: return processStateMachineNode;
    case AnimNode::Type::Manipulator: return processDoNothing;
    case AnimNode::Type::InverseKinematics: return processDoNothing;
    case AnimNode::Type::DefaultPose: return processDoNothing;
    case AnimNode::Type::NumTypes: return nullptr;
    };
    return nullptr;
}

#define READ_STRING(NAME, JSON_OBJ, ID, URL, ERROR_RETURN)              \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    if (!NAME##_VAL.isString()) {                                       \
        qCCritical(animation) << "AnimNodeLoader, error reading string" \
                              << #NAME << ", id =" << ID                \
                              << ", url =" << URL.toDisplayString();    \
        return ERROR_RETURN;                                            \
    }                                                                   \
    QString NAME = NAME##_VAL.toString()

#define READ_OPTIONAL_STRING(NAME, JSON_OBJ)                            \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    QString NAME;                                                       \
    if (NAME##_VAL.isString()) {                                        \
        NAME = NAME##_VAL.toString();                                   \
    }

#define READ_BOOL(NAME, JSON_OBJ, ID, URL, ERROR_RETURN)                \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    if (!NAME##_VAL.isBool()) {                                         \
        qCCritical(animation) << "AnimNodeLoader, error reading bool"   \
                              << #NAME << ", id =" << ID                \
                              << ", url =" << URL.toDisplayString();    \
        return ERROR_RETURN;                                            \
    }                                                                   \
    bool NAME = NAME##_VAL.toBool()

#define READ_OPTIONAL_BOOL(NAME, JSON_OBJ, DEFAULT)                     \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    bool NAME = DEFAULT;                                                \
    if (NAME##_VAL.isBool()) {                                          \
        NAME = NAME##_VAL.toBool();                                     \
    }                                                                   \
    do {} while (0)

#define READ_FLOAT(NAME, JSON_OBJ, ID, URL, ERROR_RETURN)               \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    if (!NAME##_VAL.isDouble()) {                                       \
        qCCritical(animation) << "AnimNodeLoader, error reading double" \
                              << #NAME << "id =" << ID                  \
                              << ", url =" << URL.toDisplayString();    \
        return ERROR_RETURN;                                            \
    }                                                                   \
    float NAME = (float)NAME##_VAL.toDouble()

#define READ_OPTIONAL_FLOAT(NAME, JSON_OBJ, DEFAULT)                    \
    auto NAME##_VAL = JSON_OBJ.value(#NAME);                            \
    float NAME = (float)DEFAULT;                                        \
    if (NAME##_VAL.isDouble()) {                                        \
        NAME = (float)NAME##_VAL.toDouble();                            \
    }                                                                   \
    do {} while (0)

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
    AnimNode::Type type = stringToAnimNodeType(typeStr);
    if (type == AnimNode::Type::NumTypes) {
        qCCritical(animation) << "AnimNodeLoader, unknown node type" << typeStr << ", id =" << id << ", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }

    auto dataValue = jsonObj.value("data");
    if (!dataValue.isObject()) {
        qCCritical(animation) << "AnimNodeLoader, bad string \"data\", id =" << id << ", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }
    auto dataObj = dataValue.toObject();

    assert((int)type >= 0 && type < AnimNode::Type::NumTypes);
    auto node = (animNodeTypeToLoaderFunc(type))(dataObj, id, jsonUrl);
    if (!node) {
        return nullptr;
    }

    auto childrenValue = jsonObj.value("children");
    if (!childrenValue.isArray()) {
        qCCritical(animation) << "AnimNodeLoader, bad array \"children\", id =" << id << ", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }
    auto childrenArray = childrenValue.toArray();
    for (const auto& childValue : childrenArray) {
        if (!childValue.isObject()) {
            qCCritical(animation) << "AnimNodeLoader, bad object in \"children\", id =" << id << ", url =" << jsonUrl.toDisplayString();
            return nullptr;
        }
        AnimNode::Pointer child = loadNode(childValue.toObject(), jsonUrl);
        if (child) {
            node->addChild(child);
        } else {
            return nullptr;
        }
    }

    if ((animNodeTypeToProcessFunc(type))(node, dataObj, id, jsonUrl)) {
        return node;
    } else {
        return nullptr;
    }
}

static AnimNode::Pointer loadClipNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) {

    READ_STRING(url, jsonObj, id, jsonUrl, nullptr);
    READ_FLOAT(startFrame, jsonObj, id, jsonUrl, nullptr);
    READ_FLOAT(endFrame, jsonObj, id, jsonUrl, nullptr);
    READ_FLOAT(timeScale, jsonObj, id, jsonUrl, nullptr);
    READ_BOOL(loopFlag, jsonObj, id, jsonUrl, nullptr);
    READ_OPTIONAL_BOOL(mirrorFlag, jsonObj, false);

    READ_OPTIONAL_STRING(startFrameVar, jsonObj);
    READ_OPTIONAL_STRING(endFrameVar, jsonObj);
    READ_OPTIONAL_STRING(timeScaleVar, jsonObj);
    READ_OPTIONAL_STRING(loopFlagVar, jsonObj);
    READ_OPTIONAL_STRING(mirrorFlagVar, jsonObj);

    // animation urls can be relative to the containing url document.
    auto tempUrl = QUrl(url);
    tempUrl = jsonUrl.resolved(tempUrl);

    auto node = std::make_shared<AnimClip>(id, tempUrl.toString(), startFrame, endFrame, timeScale, loopFlag, mirrorFlag);

    if (!startFrameVar.isEmpty()) {
        node->setStartFrameVar(startFrameVar);
    }
    if (!endFrameVar.isEmpty()) {
        node->setEndFrameVar(endFrameVar);
    }
    if (!timeScaleVar.isEmpty()) {
        node->setTimeScaleVar(timeScaleVar);
    }
    if (!loopFlagVar.isEmpty()) {
        node->setLoopFlagVar(loopFlagVar);
    }
    if (!mirrorFlagVar.isEmpty()) {
        node->setMirrorFlagVar(mirrorFlagVar);
    }

    return node;
}

static AnimNode::Pointer loadBlendLinearNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) {

    READ_FLOAT(alpha, jsonObj, id, jsonUrl, nullptr);

    READ_OPTIONAL_STRING(alphaVar, jsonObj);

    auto node = std::make_shared<AnimBlendLinear>(id, alpha);

    if (!alphaVar.isEmpty()) {
        node->setAlphaVar(alphaVar);
    }

    return node;
}

static AnimNode::Pointer loadBlendLinearMoveNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) {

    READ_FLOAT(alpha, jsonObj, id, jsonUrl, nullptr);
    READ_FLOAT(desiredSpeed, jsonObj, id, jsonUrl, nullptr);

    std::vector<float> characteristicSpeeds;
    auto speedsValue = jsonObj.value("characteristicSpeeds");
    if (!speedsValue.isArray()) {
        qCCritical(animation) << "AnimNodeLoader, bad array \"characteristicSpeeds\" in blendLinearMove node, id =" << id << ", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }

    auto speedsArray = speedsValue.toArray();
    for (const auto& speedValue : speedsArray) {
        if (!speedValue.isDouble()) {
            qCCritical(animation) << "AnimNodeLoader, bad number in \"characteristicSpeeds\", id =" << id << ", url =" << jsonUrl.toDisplayString();
            return nullptr;
        }
        float speedVal = (float)speedValue.toDouble();
        characteristicSpeeds.push_back(speedVal);
    };

    READ_OPTIONAL_STRING(alphaVar, jsonObj);
    READ_OPTIONAL_STRING(desiredSpeedVar, jsonObj);

    auto node = std::make_shared<AnimBlendLinearMove>(id, alpha, desiredSpeed, characteristicSpeeds);

    if (!alphaVar.isEmpty()) {
        node->setAlphaVar(alphaVar);
    }

    if (!desiredSpeedVar.isEmpty()) {
        node->setDesiredSpeedVar(desiredSpeedVar);
    }

    return node;
}


static const char* boneSetStrings[AnimOverlay::NumBoneSets] = {
    "fullBody",
    "upperBody",
    "lowerBody",
    "leftArm",
    "rightArm",
    "aboveTheHead",
    "belowTheHead",
    "headOnly",
    "spineOnly",
    "empty",
    "leftHand",
    "rightHand",
    "hipsOnly",
    "bothFeet"
};

static AnimOverlay::BoneSet stringToBoneSetEnum(const QString& str) {
    for (int i = 0; i < (int)AnimOverlay::NumBoneSets; i++) {
        if (str == boneSetStrings[i]) {
            return (AnimOverlay::BoneSet)i;
        }
    }
    return AnimOverlay::NumBoneSets;
}

static const char* solutionSourceStrings[(int)AnimInverseKinematics::SolutionSource::NumSolutionSources] = {
    "relaxToUnderPoses",
    "relaxToLimitCenterPoses",
    "previousSolution",
    "underPoses",
    "limitCenterPoses"
};

static AnimInverseKinematics::SolutionSource stringToSolutionSourceEnum(const QString& str) {
    for (int i = 0; i < (int)AnimInverseKinematics::SolutionSource::NumSolutionSources; i++) {
        if (str == solutionSourceStrings[i]) {
            return (AnimInverseKinematics::SolutionSource)i;
        }
    }
    return AnimInverseKinematics::SolutionSource::NumSolutionSources;
}

static AnimNode::Pointer loadOverlayNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) {

    READ_STRING(boneSet, jsonObj, id, jsonUrl, nullptr);
    READ_FLOAT(alpha, jsonObj, id, jsonUrl, nullptr);

    auto boneSetEnum = stringToBoneSetEnum(boneSet);
    if (boneSetEnum == AnimOverlay::NumBoneSets) {
        qCCritical(animation) << "AnimNodeLoader, unknown bone set =" << boneSet << ", defaulting to \"fullBody\", url =" << jsonUrl.toDisplayString();
        boneSetEnum = AnimOverlay::FullBodyBoneSet;
    }

    READ_OPTIONAL_STRING(boneSetVar, jsonObj);
    READ_OPTIONAL_STRING(alphaVar, jsonObj);

    auto node = std::make_shared<AnimOverlay>(id, boneSetEnum, alpha);

    if (!boneSetVar.isEmpty()) {
        node->setBoneSetVar(boneSetVar);
    }
    if (!alphaVar.isEmpty()) {
        node->setAlphaVar(alphaVar);
    }

    return node;
}

static AnimNode::Pointer loadStateMachineNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) {
    auto node = std::make_shared<AnimStateMachine>(id);
    return node;
}

static AnimNode::Pointer loadManipulatorNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) {

    READ_FLOAT(alpha, jsonObj, id, jsonUrl, nullptr);
    auto node = std::make_shared<AnimManipulator>(id, alpha);

    READ_OPTIONAL_STRING(alphaVar, jsonObj);
    if (!alphaVar.isEmpty()) {
        node->setAlphaVar(alphaVar);
    }

    auto jointsValue = jsonObj.value("joints");
    if (!jointsValue.isArray()) {
        qCCritical(animation) << "AnimNodeLoader, bad array \"joints\" in controller node, id =" << id << ", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }

    auto jointsArray = jointsValue.toArray();
    for (const auto& jointValue : jointsArray) {
        if (!jointValue.isObject()) {
            qCCritical(animation) << "AnimNodeLoader, bad state object in \"joints\", id =" << id << ", url =" << jsonUrl.toDisplayString();
            return nullptr;
        }
        auto jointObj = jointValue.toObject();

        READ_STRING(jointName, jointObj, id, jsonUrl, nullptr);
        READ_STRING(rotationType, jointObj, id, jsonUrl, nullptr);
        READ_STRING(translationType, jointObj, id, jsonUrl, nullptr);
        READ_STRING(rotationVar, jointObj, id, jsonUrl, nullptr);
        READ_STRING(translationVar, jointObj, id, jsonUrl, nullptr);

        AnimManipulator::JointVar::Type jointVarRotationType = stringToAnimManipulatorJointVarType(rotationType);
        if (jointVarRotationType == AnimManipulator::JointVar::Type::NumTypes) {
            qCWarning(animation) << "AnimNodeLoader, bad rotationType in \"joints\", id =" << id << ", url =" << jsonUrl.toDisplayString();
            jointVarRotationType = AnimManipulator::JointVar::Type::Default;
        }

        AnimManipulator::JointVar::Type jointVarTranslationType = stringToAnimManipulatorJointVarType(translationType);
        if (jointVarTranslationType == AnimManipulator::JointVar::Type::NumTypes) {
            qCWarning(animation) << "AnimNodeLoader, bad translationType in \"joints\", id =" << id << ", url =" << jsonUrl.toDisplayString();
            jointVarTranslationType = AnimManipulator::JointVar::Type::Default;
        }

        AnimManipulator::JointVar jointVar(jointName, jointVarRotationType, jointVarTranslationType, rotationVar, translationVar);
        node->addJointVar(jointVar);
    };

    return node;
}

AnimNode::Pointer loadInverseKinematicsNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) {
    auto node = std::make_shared<AnimInverseKinematics>(id);

    auto targetsValue = jsonObj.value("targets");
    if (!targetsValue.isArray()) {
        qCCritical(animation) << "AnimNodeLoader, bad array \"targets\" in inverseKinematics node, id =" << id << ", url =" << jsonUrl.toDisplayString();
        return nullptr;
    }

    auto targetsArray = targetsValue.toArray();
    for (const auto& targetValue : targetsArray) {
        if (!targetValue.isObject()) {
            qCCritical(animation) << "AnimNodeLoader, bad state object in \"targets\", id =" << id << ", url =" << jsonUrl.toDisplayString();
            return nullptr;
        }
        auto targetObj = targetValue.toObject();

        READ_STRING(jointName, targetObj, id, jsonUrl, nullptr);
        READ_STRING(positionVar, targetObj, id, jsonUrl, nullptr);
        READ_STRING(rotationVar, targetObj, id, jsonUrl, nullptr);
        READ_OPTIONAL_STRING(typeVar, targetObj);
        READ_OPTIONAL_STRING(weightVar, targetObj);
        READ_OPTIONAL_FLOAT(weight, targetObj, 1.0f);
        READ_OPTIONAL_STRING(poleVectorEnabledVar, targetObj);
        READ_OPTIONAL_STRING(poleReferenceVectorVar, targetObj);
        READ_OPTIONAL_STRING(poleVectorVar, targetObj);

        auto flexCoefficientsValue = targetObj.value("flexCoefficients");
        if (!flexCoefficientsValue.isArray()) {
            qCCritical(animation) << "AnimNodeLoader, bad or missing flexCoefficients array in \"targets\", id =" << id << ", url =" << jsonUrl.toDisplayString();
            return nullptr;
        }
        auto flexCoefficientsArray = flexCoefficientsValue.toArray();
        std::vector<float> flexCoefficients;
        for (const auto& value : flexCoefficientsArray) {
            flexCoefficients.push_back((float)value.toDouble());
        }

        node->setTargetVars(jointName, positionVar, rotationVar, typeVar, weightVar, weight, flexCoefficients, poleVectorEnabledVar, poleReferenceVectorVar, poleVectorVar);
    };

    READ_OPTIONAL_STRING(solutionSource, jsonObj);

    if (!solutionSource.isEmpty()) {
        AnimInverseKinematics::SolutionSource solutionSourceType = stringToSolutionSourceEnum(solutionSource);
        if (solutionSourceType != AnimInverseKinematics::SolutionSource::NumSolutionSources) {
            node->setSolutionSource(solutionSourceType);
        } else {
            qCWarning(animation) << "AnimNodeLoader, bad solutionSourceType in \"solutionSource\", id = " << id << ", url = " << jsonUrl.toDisplayString();
        }
    }

    READ_OPTIONAL_STRING(solutionSourceVar, jsonObj);

    if (!solutionSourceVar.isEmpty()) {
        node->setSolutionSourceVar(solutionSourceVar);
    }

    return node;
}

static AnimNode::Pointer loadDefaultPoseNode(const QJsonObject& jsonObj, const QString& id, const QUrl& jsonUrl) {
    auto node = std::make_shared<AnimDefaultPose>(id);
    return node;
}

void buildChildMap(std::map<QString, int>& map, AnimNode::Pointer node) {
    for (int i = 0; i < (int)node->getChildCount(); ++i) {
        map.insert(std::pair<QString, int>(node->getChild(i)->getID(), i));
    }
}

bool processStateMachineNode(AnimNode::Pointer node, const QJsonObject& jsonObj, const QString& nodeId, const QUrl& jsonUrl) {
    auto smNode = std::static_pointer_cast<AnimStateMachine>(node);
    assert(smNode);

    READ_STRING(currentState, jsonObj, nodeId, jsonUrl, false);

    auto statesValue = jsonObj.value("states");
    if (!statesValue.isArray()) {
        qCCritical(animation) << "AnimNodeLoader, bad array \"states\" in stateMachine node, id =" << nodeId << ", url =" << jsonUrl.toDisplayString();
        return false;
    }

    // build a map for all children by name.
    std::map<QString, int> childMap;
    buildChildMap(childMap, node);

    // first pass parse all the states and build up the state and transition map.
    using StringPair = std::pair<QString, QString>;
    using TransitionMap = std::multimap<AnimStateMachine::State::Pointer, StringPair>;
    TransitionMap transitionMap;

    using StateMap = std::map<QString, AnimStateMachine::State::Pointer>;
    StateMap stateMap;

    auto statesArray = statesValue.toArray();
    for (const auto& stateValue : statesArray) {
        if (!stateValue.isObject()) {
            qCCritical(animation) << "AnimNodeLoader, bad state object in \"states\", id =" << nodeId << ", url =" << jsonUrl.toDisplayString();
            return false;
        }
        auto stateObj = stateValue.toObject();

        READ_STRING(id, stateObj, nodeId, jsonUrl, false);
        READ_FLOAT(interpTarget, stateObj, nodeId, jsonUrl, false);
        READ_FLOAT(interpDuration, stateObj, nodeId, jsonUrl, false);
        READ_OPTIONAL_STRING(interpType, stateObj);

        READ_OPTIONAL_STRING(interpTargetVar, stateObj);
        READ_OPTIONAL_STRING(interpDurationVar, stateObj);
        READ_OPTIONAL_STRING(interpTypeVar, stateObj);

        auto iter = childMap.find(id);
        if (iter == childMap.end()) {
            qCCritical(animation) << "AnimNodeLoader, could not find stateMachine child (state) with nodeId =" << nodeId << "stateId =" << id << "url =" << jsonUrl.toDisplayString();
            return false;
        }

        AnimStateMachine::InterpType interpTypeEnum = AnimStateMachine::InterpType::SnapshotBoth;  // default value
        if (!interpType.isEmpty()) {
            interpTypeEnum = stringToInterpType(interpType);
            if (interpTypeEnum == AnimStateMachine::InterpType::NumTypes) {
                qCCritical(animation) << "AnimNodeLoader, bad interpType on stateMachine state, nodeId = " << nodeId << "stateId =" << id << "url = " << jsonUrl.toDisplayString();
                return false;
            }
        }

        auto statePtr = std::make_shared<AnimStateMachine::State>(id, iter->second, interpTarget, interpDuration, interpTypeEnum);
        assert(statePtr);

        if (!interpTargetVar.isEmpty()) {
            statePtr->setInterpTargetVar(interpTargetVar);
        }
        if (!interpDurationVar.isEmpty()) {
            statePtr->setInterpDurationVar(interpDurationVar);
        }
        if (!interpTypeVar.isEmpty()) {
            statePtr->setInterpTypeVar(interpTypeVar);
        }

        smNode->addState(statePtr);
        stateMap.insert(StateMap::value_type(statePtr->getID(), statePtr));

        auto transitionsValue = stateObj.value("transitions");
        if (!transitionsValue.isArray()) {
            qCritical(animation) << "AnimNodeLoader, bad array \"transitions\" in stateMachine node, stateId =" << id << "nodeId =" << nodeId << "url =" << jsonUrl.toDisplayString();
            return false;
        }

        auto transitionsArray = transitionsValue.toArray();
        for (const auto& transitionValue : transitionsArray) {
            if (!transitionValue.isObject()) {
                qCritical(animation) << "AnimNodeLoader, bad transition object in \"transtions\", stateId =" << id << "nodeId =" << nodeId << "url =" << jsonUrl.toDisplayString();
                return false;
            }
            auto transitionObj = transitionValue.toObject();

            READ_STRING(var, transitionObj, nodeId, jsonUrl, false);
            READ_STRING(state, transitionObj, nodeId, jsonUrl, false);

            transitionMap.insert(TransitionMap::value_type(statePtr, StringPair(var, state)));
        }
    }

    // second pass: now iterate thru all transitions and add them to the appropriate states.
    for (auto& transition : transitionMap) {
        AnimStateMachine::State::Pointer srcState = transition.first;
        auto iter = stateMap.find(transition.second.second);
        if (iter != stateMap.end()) {
            srcState->addTransition(AnimStateMachine::State::Transition(transition.second.first, iter->second));
        } else {
            qCCritical(animation) << "AnimNodeLoader, bad state machine transtion from srcState =" << srcState->_id << "dstState =" << transition.second.second << "nodeId =" << nodeId << "url = " << jsonUrl.toDisplayString();
            return false;
        }
    }

    auto iter = stateMap.find(currentState);
    if (iter == stateMap.end()) {
        qCCritical(animation) << "AnimNodeLoader, bad currentState =" << currentState << "could not find child node" << "id =" << nodeId << "url = " << jsonUrl.toDisplayString();
    }
    smNode->setCurrentState(iter->second);

    return true;
}

AnimNodeLoader::AnimNodeLoader(const QUrl& url) :
    _url(url)
{
    _resource = QSharedPointer<Resource>::create(url);
    _resource->setSelf(_resource);
    connect(_resource.data(), &Resource::loaded, this, &AnimNodeLoader::onRequestDone);
    connect(_resource.data(), &Resource::failed, this, &AnimNodeLoader::onRequestError);
    _resource->ensureLoading();
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

void AnimNodeLoader::onRequestDone(const QByteArray data) {
    auto node = load(data, _url);
    if (node) {
        emit success(node);
    } else {
        emit error(0, "json parse error");
    }
}

void AnimNodeLoader::onRequestError(QNetworkReply::NetworkError netError) {
    emit error((int)netError, "Resource download error");
}
