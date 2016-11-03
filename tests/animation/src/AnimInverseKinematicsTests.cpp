//
//  AnimInverseKinematicsTests.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimInverseKinematicsTests.h"

#include <glm/gtx/transform.hpp>

#include <AnimInverseKinematics.h>
#include <AnimBlendLinear.h>
#include <AnimationLogging.h>
#include <NumericalConstants.h>

#include "../QTestExtensions.h"

QTEST_MAIN(AnimInverseKinematicsTests)

const glm::vec3 origin(0.0f);
const glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
const glm::vec3 zAxis(0.0f, 0.0f, 1.0f);
const glm::quat identity = glm::quat();
const glm::quat quaterTurnAroundZ = glm::angleAxis(0.5f * PI, zAxis);


void makeTestFBXJoints(FBXGeometry& geometry) {
    FBXJoint joint;
    joint.isFree = false;
    joint.freeLineage.clear();
    joint.parentIndex = -1;
    joint.distanceToParent = 1.0f;

    joint.translation = origin;        // T
    joint.preTransform = glm::mat4();  // Roff * Rp
    joint.preRotation = identity;      // Rpre
    joint.rotation = identity;         // R
    joint.postRotation = identity;     // Rpost
    joint.postTransform = glm::mat4(); // Rp-1 * Soff * Sp * S * Sp-1

    // World = ParentWorld * T * (Roff * Rp) * Rpre * R * Rpost * (Rp-1 * Soff * Sp * S * Sp-1)
    joint.transform = glm::mat4();
    joint.rotationMin = glm::vec3(-PI);
    joint.rotationMax = glm::vec3(PI);
    joint.inverseDefaultRotation = identity;
    joint.inverseBindRotation = identity;
    joint.bindTransform = glm::mat4();

    joint.name = "";
    joint.isSkeletonJoint = false;

    // we make a list of joints that look like this:
    //
    // A------>B------>C------>D

    joint.name = "A";
    joint.parentIndex = -1;
    joint.translation = origin;
    geometry.joints.push_back(joint);

    joint.name = "B";
    joint.parentIndex = 0;
    joint.translation = xAxis;
    geometry.joints.push_back(joint);

    joint.name = "C";
    joint.parentIndex = 1;
    joint.translation = xAxis;
    geometry.joints.push_back(joint);

    joint.name = "D";
    joint.parentIndex = 2;
    joint.translation = xAxis;
    geometry.joints.push_back(joint);

    // compute each joint's transform
    for (int i = 1; i < (int)geometry.joints.size(); ++i) {
        FBXJoint& j = geometry.joints[i];
        int parentIndex = j.parentIndex;
        // World = ParentWorld * T * (Roff * Rp) * Rpre * R * Rpost * (Rp-1 * Soff * Sp * S * Sp-1)
        j.transform = geometry.joints[parentIndex].transform *
            glm::translate(j.translation) *
            j.preTransform *
            glm::mat4_cast(j.preRotation * j.rotation * j.postRotation) *
            j.postTransform;
        j.inverseBindRotation = identity;
        j.bindTransform = j.transform;
    }
}

void AnimInverseKinematicsTests::testSingleChain() {
    FBXGeometry geometry;
    makeTestFBXJoints(geometry);

    // create a skeleton and doll
    AnimPose offset;
    AnimSkeleton::Pointer skeletonPtr = std::make_shared<AnimSkeleton>(geometry);
    AnimInverseKinematics ikDoll("doll");

    ikDoll.setSkeleton(skeletonPtr);

    { // easy test IK of joint C
        // load intial poses that look like this:
        //
        // A------>B------>C------>D
        AnimPose pose;
        pose.scale = glm::vec3(1.0f);
        pose.rot = identity;
        pose.trans = origin;

        AnimPoseVec poses;
        poses.push_back(pose);

        pose.trans = xAxis;
        for (int i = 1; i < (int)geometry.joints.size(); ++i) {
            poses.push_back(pose);
        }
        ikDoll.loadPoses(poses);

        // we'll put a target t on D for <2, 1, 0> like this:
        //
        //                 t
        //
        //
        // A------>B------>C------>D
        //
        glm::vec3 targetPosition(2.0f, 1.0f, 0.0f);
        glm::quat targetRotation = glm::angleAxis(PI / 2.0f, zAxis);
        AnimVariantMap varMap;
        varMap.set("positionD", targetPosition);
        varMap.set("rotationD", targetRotation);
        varMap.set("targetType", (int)IKTarget::Type::RotationAndPosition);
        ikDoll.setTargetVars(QString("D"), QString("positionD"), QString("rotationD"), QString("targetType"));
        AnimNode::Triggers triggers;

        // the IK solution should be:
        //
        //                 D
        //                 |
        //                 |
        // A------>B------>C
        //

        // iterate several times
        float dt = 1.0f;
        ikDoll.overlay(varMap, dt, triggers, poses);
        ikDoll.overlay(varMap, dt, triggers, poses);
        ikDoll.overlay(varMap, dt, triggers, poses);

        ikDoll.overlay(varMap, dt, triggers, poses);
        ikDoll.overlay(varMap, dt, triggers, poses);
        ikDoll.overlay(varMap, dt, triggers, poses);
        const AnimPoseVec& relativePoses = ikDoll.overlay(varMap, dt, triggers, poses);

        // verify absolute results
        // NOTE: since we expect this solution to converge very quickly (one loop)
        // we can impose very tight error thresholds.
        AnimPoseVec absolutePoses;
        for (auto pose : poses) {
            absolutePoses.push_back(pose);
        }
        ikDoll.computeAbsolutePoses(absolutePoses);
        const float acceptableAngleError = 0.001f;
        QCOMPARE_QUATS(absolutePoses[0].rot, identity, acceptableAngleError);
        QCOMPARE_QUATS(absolutePoses[1].rot, identity, acceptableAngleError);
        QCOMPARE_QUATS(absolutePoses[2].rot, quaterTurnAroundZ, acceptableAngleError);
        QCOMPARE_QUATS(absolutePoses[3].rot, quaterTurnAroundZ, acceptableAngleError);

        const float acceptableTranslationError = 0.025f;
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[0].trans, origin, acceptableTranslationError);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[1].trans, xAxis, acceptableTranslationError);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[2].trans, 2.0f * xAxis, acceptableTranslationError);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[3].trans, targetPosition, acceptableTranslationError);

        // verify relative results
        QCOMPARE_QUATS(relativePoses[0].rot, identity, acceptableAngleError);
        QCOMPARE_QUATS(relativePoses[1].rot, identity, acceptableAngleError);
        QCOMPARE_QUATS(relativePoses[2].rot, quaterTurnAroundZ, acceptableAngleError);
        QCOMPARE_QUATS(relativePoses[3].rot, identity, acceptableAngleError);

        QCOMPARE_WITH_ABS_ERROR(relativePoses[0].trans, origin, acceptableTranslationError);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[1].trans, xAxis, acceptableTranslationError);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[2].trans, xAxis, acceptableTranslationError);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[3].trans, xAxis, acceptableTranslationError);
    }
    { // hard test IK of joint C
        // load intial poses that look like this:
        //
        // D<------C
        //         |
        //         |
        // A------>B
        //
        AnimPose pose;
        pose.scale = glm::vec3(1.0f);
        pose.rot = identity;
        pose.trans = origin;

        AnimPoseVec poses;
        poses.push_back(pose);
        pose.trans = xAxis;

        pose.rot = quaterTurnAroundZ;
        poses.push_back(pose);
        poses.push_back(pose);
        poses.push_back(pose);
        ikDoll.loadPoses(poses);

        // we'll put a target t on D for <3, 0, 0> like this:
        //
        // D<------C
        //         |
        //         |
        // A------>B               t
        //
        glm::vec3 targetPosition(3.0f, 0.0f, 0.0f);
        glm::quat targetRotation = identity;
        AnimVariantMap varMap;
        varMap.set("positionD", targetPosition);
        varMap.set("rotationD", targetRotation);
        varMap.set("targetType", (int)IKTarget::Type::RotationAndPosition);
        ikDoll.setTargetVars(QString("D"), QString("positionD"), QString("rotationD"), QString("targetType"));
        AnimNode::Triggers triggers;

        // the IK solution should be:
        //
        // A------>B------>C------>D
        //

        // iterate several times
        float dt = 1.0f;
        ikDoll.overlay(varMap, dt, triggers, poses);
        ikDoll.overlay(varMap, dt, triggers, poses);
        ikDoll.overlay(varMap, dt, triggers, poses);
        ikDoll.overlay(varMap, dt, triggers, poses);
        ikDoll.overlay(varMap, dt, triggers, poses);
        ikDoll.overlay(varMap, dt, triggers, poses);
        ikDoll.overlay(varMap, dt, triggers, poses);
        ikDoll.overlay(varMap, dt, triggers, poses);
        const AnimPoseVec& relativePoses = ikDoll.overlay(varMap, dt, triggers, poses);

        // verify absolute results
        // NOTE: the IK algorithm doesn't converge very fast for full-reach targets,
        // so we'll consider the poses as good if they are closer than they started.
        //
        // NOTE: constraints may help speed up convergence since some joints may get clamped
        // to maximum extension. TODO: experiment with tightening the error thresholds when
        // constraints are working.
        AnimPoseVec absolutePoses;
        for (auto pose : poses) {
            absolutePoses.push_back(pose);
        }
        ikDoll.computeAbsolutePoses(absolutePoses);
        float acceptableAngle = 0.01f; // radians
        QCOMPARE_QUATS(absolutePoses[0].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(absolutePoses[1].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(absolutePoses[2].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(absolutePoses[3].rot, identity, acceptableAngle);

        float acceptableDistance = 0.03f;
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[0].trans, origin, acceptableDistance);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[1].trans, xAxis, acceptableDistance);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[2].trans, 2.0f * xAxis, acceptableDistance);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[3].trans, 3.0f * xAxis, acceptableDistance);

        // verify relative results
        QCOMPARE_QUATS(relativePoses[0].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(relativePoses[1].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(relativePoses[2].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(relativePoses[3].rot, identity, acceptableAngle);

        QCOMPARE_WITH_ABS_ERROR(relativePoses[0].trans, origin, acceptableDistance);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[1].trans, xAxis, acceptableDistance);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[2].trans, xAxis, acceptableDistance);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[3].trans, xAxis, acceptableDistance);
    }
}

void AnimInverseKinematicsTests::testBar() {
    // test AnimPose math
    // TODO: move this to other test file
    glm::vec3 transA = glm::vec3(1.0f, 2.0f, 3.0f);
    glm::vec3 transB = glm::vec3(5.0f, 7.0f, 9.0f);
    glm::quat rot = identity;
    glm::vec3 scale = glm::vec3(1.0f);

    AnimPose poseA(scale, rot, transA);
    AnimPose poseB(scale, rot, transB);
    AnimPose poseC = poseA * poseB;

    glm::vec3 expectedTransC = transA + transB;
    QCOMPARE_WITH_ABS_ERROR(expectedTransC, poseC.trans, EPSILON);
}

