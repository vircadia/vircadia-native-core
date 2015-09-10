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

#include <AnimNodeLoader.h>
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


void makeTestFBXJoints(std::vector<FBXJoint>& fbxJoints) {
    FBXJoint joint;
    joint.isFree = false;
    joint.freeLineage.clear();
    joint.parentIndex = -1;
    joint.distanceToParent = 1.0f;
    joint.boneRadius = 1.0f;

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
    fbxJoints.push_back(joint);

    joint.name = "B";
    joint.parentIndex = 0;
    joint.translation = xAxis;
    fbxJoints.push_back(joint);

    joint.name = "C";
    joint.parentIndex = 1;
    joint.translation = xAxis;
    fbxJoints.push_back(joint);

    joint.name = "D";
    joint.parentIndex = 2;
    joint.translation = xAxis;
    fbxJoints.push_back(joint);

    // compute each joint's transform
    for (int i = 1; i < (int)fbxJoints.size(); ++i) {
        FBXJoint& j = fbxJoints[i];
        int parentIndex = j.parentIndex;
        // World = ParentWorld * T * (Roff * Rp) * Rpre * R * Rpost * (Rp-1 * Soff * Sp * S * Sp-1)
        j.transform = fbxJoints[parentIndex].transform *
            glm::translate(j.translation) *
            j.preTransform *
            glm::mat4_cast(j.preRotation * j.rotation * j.postRotation) *
            j.postTransform;
        j.inverseBindRotation = identity;
        j.bindTransform = j.transform;
    }
}

void AnimInverseKinematicsTests::testSingleChain() {
    std::vector<FBXJoint> fbxJoints;
    AnimPose offset;
    makeTestFBXJoints(fbxJoints, offset);

    // create a skeleton and doll
    AnimSkeleton* skeleton = new AnimSkeleton(fbxJoints);
    AnimSkeleton::Pointer skeletonPtr(skeleton);
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

        std::vector<AnimPose> poses;
        poses.push_back(pose);

        pose.trans = xAxis;
        for (int i = 1; i < (int)fbxJoints.size(); ++i) {
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
        int indexD = 3;
        glm::vec3 targetPosition(2.0f, 1.0f, 0.0f);
        glm::quat targetRotation = glm::angleAxis(PI / 2.0f, zAxis);
        ikDoll.updateTarget(indexD, targetPosition, targetRotation);

        // the IK solution should be:
        //
        //                 D
        //                 |
        //                 |
        // A------>B------>C
        //
        float dt = 1.0f;
        ikDoll.evaluate(dt);

        // verify absolute results
        // NOTE: since we expect this solution to converge very quickly (one loop)
        // we can impose very tight error thresholds.
        std::vector<AnimPose> absolutePoses;
        ikDoll.computeAbsolutePoses(absolutePoses);
        float acceptableAngle = 0.0001f;
        QCOMPARE_QUATS(absolutePoses[0].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(absolutePoses[1].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(absolutePoses[2].rot, quaterTurnAroundZ, acceptableAngle);
        QCOMPARE_QUATS(absolutePoses[3].rot, quaterTurnAroundZ, acceptableAngle);

        QCOMPARE_WITH_ABS_ERROR(absolutePoses[0].trans, origin, EPSILON);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[1].trans, xAxis, EPSILON);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[2].trans, 2.0f * xAxis, EPSILON);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[3].trans, targetPosition, EPSILON);

        // verify relative results
        const std::vector<AnimPose>& relativePoses = ikDoll.getRelativePoses();
        QCOMPARE_QUATS(relativePoses[0].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(relativePoses[1].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(relativePoses[2].rot, quaterTurnAroundZ, acceptableAngle);
        QCOMPARE_QUATS(relativePoses[3].rot, identity, acceptableAngle);

        QCOMPARE_WITH_ABS_ERROR(relativePoses[0].trans, origin, EPSILON);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[1].trans, xAxis, EPSILON);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[2].trans, xAxis, EPSILON);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[3].trans, xAxis, EPSILON);
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
        
        std::vector<AnimPose> poses;
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
        int indexD = 3;
        glm::vec3 targetPosition(3.0f, 0.0f, 0.0f);
        glm::quat targetRotation = identity;
        ikDoll.updateTarget(indexD, targetPosition, targetRotation);

        // the IK solution should be:
        //
        // A------>B------>C------>D
        //
        float dt = 1.0f;
        ikDoll.evaluate(dt);

        // verify absolute results
        // NOTE: the IK algorithm doesn't converge very fast for full-reach targets,
        // so we'll consider the poses as good if they are closer than they started.
        //
        // NOTE: constraints may help speed up convergence since some joints may get clamped
        // to maximum extension. TODO: experiment with tightening the error thresholds when
        // constraints are working.
        std::vector<AnimPose> absolutePoses;
        ikDoll.computeAbsolutePoses(absolutePoses);
        float acceptableAngle = 0.1f; // radians
        QCOMPARE_QUATS(absolutePoses[0].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(absolutePoses[1].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(absolutePoses[2].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(absolutePoses[3].rot, identity, acceptableAngle);

        float acceptableDistance = 0.4f;
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[0].trans, origin, EPSILON);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[1].trans, xAxis, acceptableDistance);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[2].trans, 2.0f * xAxis, acceptableDistance);
        QCOMPARE_WITH_ABS_ERROR(absolutePoses[3].trans, 3.0f * xAxis, acceptableDistance);

        // verify relative results
        const std::vector<AnimPose>& relativePoses = ikDoll.getRelativePoses();
        QCOMPARE_QUATS(relativePoses[0].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(relativePoses[1].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(relativePoses[2].rot, identity, acceptableAngle);
        QCOMPARE_QUATS(relativePoses[3].rot, identity, acceptableAngle);

        QCOMPARE_WITH_ABS_ERROR(relativePoses[0].trans, origin, EPSILON);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[1].trans, xAxis, EPSILON);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[2].trans, xAxis, EPSILON);
        QCOMPARE_WITH_ABS_ERROR(relativePoses[3].trans, xAxis, EPSILON);
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

