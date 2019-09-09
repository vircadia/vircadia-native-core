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

QMap<QString, QString> getJointNameMapping(const hifi::VariantHash& mapping) {
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

QMap<QString, glm::quat> getJointRotationOffsets(const hifi::VariantHash& mapping) {
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

void PrepareJointsTask::configure(const Config& config) {
    _passthrough = config.passthrough;
}


void PrepareJointsTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const auto& jointsIn = input.get0();
    auto& jointsOut = output.edit0();
    std::vector<QString> jointNames;
    if (_passthrough) {
        jointsOut = jointsIn;
    } else {
        const auto& mapping = input.get1();
        auto& jointRotationOffsets = output.edit1();

        static const QString JOINT_ROTATION_OFFSET2_FIELD = "jointRotationOffset2";
        QVariantHash fstHashMap = mapping;
        bool newJointRot = fstHashMap.contains(JOINT_ROTATION_OFFSET2_FIELD);

        // Get joint renames
        QMap<QString, QString> jointNameMapping = getJointNameMapping(mapping);
        // Apply joint metadata from FST file mappings
        for (const hfm::Joint& jointIn : jointsIn) {
            jointsOut.push_back(jointIn);
            hfm::Joint& jointOut = jointsOut.back();

            if (!newJointRot) {
                // Offset rotations are referenced by the new joint's names
                // We need to assign new joint's names now, before the offsets are read.
                QString jointNameMapKey = jointNameMapping.key(jointIn.name);
                if (jointNameMapping.contains(jointNameMapKey)) {
                    jointOut.name = jointNameMapKey;
                }
            }
            jointNames.push_back(jointOut.name);
        }

        // Get joint rotation offsets from FST file mappings
        QMap<QString, glm::quat> offsets = getJointRotationOffsets(mapping);
        for (auto itr = offsets.begin(); itr != offsets.end(); itr++) {
            QString jointName = itr.key();
            auto jointToOffset = std::find_if(jointsOut.begin(), jointsOut.end(), [jointName](const hfm::Joint& joint) {
                return (joint.name == jointName && joint.isSkeletonJoint);
            });
            if (jointToOffset != jointsOut.end()) {
                // In case there are named duplicates we'll assign the offset rotation to the first skeleton joint that matches the name.
                int jointIndex = (int)distance(jointsOut.begin(), jointToOffset);
                if (jointIndex >= 0) {
                    glm::quat rotationOffset = itr.value();
                    jointRotationOffsets.insert(jointIndex, rotationOffset);
                    qCDebug(model_baker) << "Joint Rotation Offset added to Rig._jointRotationOffsets : " << " jointName: " << jointName << " jointIndex: " << jointIndex << " rotation offset: " << rotationOffset;
                }
            }
        }

        if (newJointRot) {
            // Offset rotations are referenced using the original joint's names
            // We need to apply new names now, once we have read the offsets
            for (size_t i = 0; i < jointsOut.size(); i++) {
                hfm::Joint& jointOut = jointsOut[i];
                QString jointNameMapKey = jointNameMapping.key(jointOut.name);
                if (jointNameMapping.contains(jointNameMapKey)) {
                    jointOut.name = jointNameMapKey;
                    jointNames[i] = jointNameMapKey;
                }
            }
        }

        const QString APPEND_DUPLICATE_JOINT = "_joint";
        const QString APPEND_DUPLICATE_MESH = "_mesh";

        // resolve duplicates and set jointIndices
        auto& jointIndices = output.edit2();
        for (size_t i = 0; i < jointsOut.size(); i++) {
            hfm::Joint& jointOut = jointsOut[i];
            if (jointIndices.contains(jointOut.name)) {
                int duplicatedJointIndex = jointIndices[jointOut.name] - 1;
                if (duplicatedJointIndex >= 0) {
                    auto& duplicatedJoint = jointsOut[duplicatedJointIndex];
                    bool areBothJoints = jointOut.isSkeletonJoint && duplicatedJoint.isSkeletonJoint;
                    QString existJointName = jointOut.name;
                    QString appendName;
                    if (areBothJoints) {
                        appendName = APPEND_DUPLICATE_JOINT;
                        qCWarning(model_baker) << "Duplicated skeleton joints found: " << existJointName;
                    } else {
                        appendName = APPEND_DUPLICATE_MESH;
                        qCDebug(model_baker) << "Duplicated joints found. Renaming the mesh joint: " << existJointName;
                    }
                    QString newName = existJointName + appendName;
                    // Make sure the new name is unique
                    int duplicateIndex = 0;
                    while (jointIndices.contains(newName)) {
                        newName = existJointName + appendName + QString::number(++duplicateIndex);
                    }
                    // Find and rename the mesh joint
                    if (!jointOut.isSkeletonJoint) {
                        jointOut.name = newName;
                    } else {
                        jointIndices.remove(jointOut.name);
                        jointIndices.insert(newName, duplicatedJointIndex);
                    }
                }
            }
            jointIndices.insert(jointOut.name, (int)(i + 1));
        }
    }
}
