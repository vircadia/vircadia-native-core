//
//  PrepareJointsTask.cpp
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/01/25.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PrepareJointsTask.h"

#include "ModelBakerLogging.h"

QMap<QString, QString> getJointNameMapping(const QVariantHash& mapping) {
    static const QString JOINT_NAME_MAPPING_FIELD = "jointMap";
    QMap<QString, QString> hfmToHifiJointNameMap;
    if (!mapping.isEmpty() && mapping.contains(JOINT_NAME_MAPPING_FIELD) && mapping[JOINT_NAME_MAPPING_FIELD].type() == QVariant::Hash) {
        auto jointNames = mapping[JOINT_NAME_MAPPING_FIELD].toHash();
        for (auto itr = jointNames.begin(); itr != jointNames.end(); itr++) {
            hfmToHifiJointNameMap.insert(itr.key(), itr.value().toString());
            qCDebug(model_baker) << "the mapped key " << itr.key() << " has a value of " << hfmToHifiJointNameMap[itr.key()];
        }
    }
    return hfmToHifiJointNameMap;
}

QMap<QString, glm::quat> getJointRotationOffsets(const QVariantHash& mapping) {
    QMap<QString, glm::quat> jointRotationOffsets;
    static const QString JOINT_ROTATION_OFFSET_FIELD = "jointRotationOffset";
    static const QString JOINT_ROTATION_OFFSET2_FIELD = "jointRotationOffset2";
    if (!mapping.isEmpty() && ((mapping.contains(JOINT_ROTATION_OFFSET_FIELD) && mapping[JOINT_ROTATION_OFFSET_FIELD].type() == QVariant::Hash) || (mapping.contains(JOINT_ROTATION_OFFSET2_FIELD) && mapping[JOINT_ROTATION_OFFSET2_FIELD].type() == QVariant::Hash))) {
        QHash<QString, QVariant> offsets;
        if (mapping.contains(JOINT_ROTATION_OFFSET_FIELD)) {
            offsets = mapping[JOINT_ROTATION_OFFSET_FIELD].toHash();
        } else {
            offsets = mapping[JOINT_ROTATION_OFFSET2_FIELD].toHash();
        }
        for (auto itr = offsets.begin(); itr != offsets.end(); itr++) {
            QString jointName = itr.key();
            QString line = itr.value().toString();
            auto quatCoords = line.split(',');
            if (quatCoords.size() == 4) {
                float quatX = quatCoords[0].mid(1).toFloat();
                float quatY = quatCoords[1].toFloat();
                float quatZ = quatCoords[2].toFloat();
                float quatW = quatCoords[3].mid(0, quatCoords[3].size() - 1).toFloat();
                if (!isNaN(quatX) && !isNaN(quatY) && !isNaN(quatZ) && !isNaN(quatW)) {
                    glm::quat rotationOffset = glm::quat(quatW, quatX, quatY, quatZ);
                    jointRotationOffsets.insert(jointName, rotationOffset);
                }
            }
        }
    }
    return jointRotationOffsets;
}

void PrepareJointsTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const auto& jointsIn = input.get0();
    const auto& mapping = input.get1();
    auto& jointsOut = output.edit0();
    auto& jointRotationOffsets = output.edit1();
    auto& jointIndices = output.edit2();

    bool newJointRot = false;
    static const QString JOINT_ROTATION_OFFSET2_FIELD = "jointRotationOffset2";
    QVariantHash fstHashMap = mapping.second;
    if (fstHashMap.contains(JOINT_ROTATION_OFFSET2_FIELD)) {
        newJointRot = true;
    } else {
        newJointRot = false;
    }

    // Get joint renames
    auto jointNameMapping = getJointNameMapping(mapping.second);
    // Apply joint metadata from FST file mappings
    for (const auto& jointIn : jointsIn) {
        jointsOut.push_back(jointIn);
        auto& jointOut = jointsOut.back();

        if (!newJointRot) {
            auto jointNameMapKey = jointNameMapping.key(jointIn.name);
            if (jointNameMapping.contains(jointNameMapKey)) {
                jointOut.name = jointNameMapKey;
            }
        }
        jointIndices.insert(jointOut.name, (int)jointsOut.size());
    }

    // Get joint rotation offsets from FST file mappings
    auto offsets = getJointRotationOffsets(mapping.second);
    for (auto itr = offsets.begin(); itr != offsets.end(); itr++) {
        QString jointName = itr.key();
        int jointIndex = jointIndices.value(jointName) - 1;
        if (jointIndex >= 0) {
            glm::quat rotationOffset = itr.value();
            jointRotationOffsets.insert(jointIndex, rotationOffset);
            qCDebug(model_baker) << "Joint Rotation Offset added to Rig._jointRotationOffsets : " << " jointName: " << jointName << " jointIndex: " << jointIndex << " rotation offset: " << rotationOffset;
        }
    }

    if (newJointRot) {
        for (auto& jointOut : jointsOut) {

            auto jointNameMapKey = jointNameMapping.key(jointOut.name);
            int mappedIndex = jointIndices.value(jointOut.name);
            if (jointNameMapping.contains(jointNameMapKey)) {
                // delete and replace with hifi name
                jointIndices.remove(jointOut.name);
                jointOut.name = jointNameMapKey;
                jointIndices.insert(jointOut.name, mappedIndex);
            } else {

                // nothing mapped to this fbx joint name
                if (jointNameMapping.contains(jointOut.name)) {
                    // but the name is in the list of hifi names is mapped to a different joint
                    int extraIndex = jointIndices.value(jointOut.name);
                    jointIndices.remove(jointOut.name);
                    jointOut.name = "";
                    jointIndices.insert(jointOut.name, extraIndex);
                }
            }
        }
    }
}
