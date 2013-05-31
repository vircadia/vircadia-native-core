//
//  Skeleton.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "Skeleton.h"

const float BODY_SPRING_DEFAULT_TIGHTNESS = 1000.0f;
const float FLOATING_HEIGHT               = 0.13f;

Skeleton::Skeleton() {
}

void Skeleton::initialize() {    
    
    for (int b=0; b<NUM_AVATAR_JOINTS; b++) {
        joint[b].isCollidable        = true;
        joint[b].parent              = AVATAR_JOINT_NULL;
        joint[b].position            = glm::vec3(0.0, 0.0, 0.0);
        joint[b].defaultPosePosition = glm::vec3(0.0, 0.0, 0.0);
        joint[b].springyPosition     = glm::vec3(0.0, 0.0, 0.0);
        joint[b].springyVelocity     = glm::vec3(0.0, 0.0, 0.0);
        joint[b].rotation            = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
        joint[b].length              = 0.0;
        joint[b].radius              = 0.0;
        joint[b].touchForce          = 0.0;
        joint[b].springBodyTightness = BODY_SPRING_DEFAULT_TIGHTNESS;
    }
    
    // specify the parental hierarchy
    joint[ AVATAR_JOINT_PELVIS		      ].parent = AVATAR_JOINT_NULL;
    joint[ AVATAR_JOINT_TORSO             ].parent = AVATAR_JOINT_PELVIS;
    joint[ AVATAR_JOINT_CHEST		      ].parent = AVATAR_JOINT_TORSO;
    joint[ AVATAR_JOINT_NECK_BASE	      ].parent = AVATAR_JOINT_CHEST;
    joint[ AVATAR_JOINT_HEAD_BASE         ].parent = AVATAR_JOINT_NECK_BASE;
    joint[ AVATAR_JOINT_HEAD_TOP          ].parent = AVATAR_JOINT_HEAD_BASE;
    joint[ AVATAR_JOINT_LEFT_COLLAR       ].parent = AVATAR_JOINT_CHEST;
    joint[ AVATAR_JOINT_LEFT_SHOULDER     ].parent = AVATAR_JOINT_LEFT_COLLAR;
    joint[ AVATAR_JOINT_LEFT_ELBOW	      ].parent = AVATAR_JOINT_LEFT_SHOULDER;
    joint[ AVATAR_JOINT_LEFT_WRIST		  ].parent = AVATAR_JOINT_LEFT_ELBOW;
    joint[ AVATAR_JOINT_LEFT_FINGERTIPS   ].parent = AVATAR_JOINT_LEFT_WRIST;
    joint[ AVATAR_JOINT_RIGHT_COLLAR      ].parent = AVATAR_JOINT_CHEST;
    joint[ AVATAR_JOINT_RIGHT_SHOULDER	  ].parent = AVATAR_JOINT_RIGHT_COLLAR;
    joint[ AVATAR_JOINT_RIGHT_ELBOW	      ].parent = AVATAR_JOINT_RIGHT_SHOULDER;
    joint[ AVATAR_JOINT_RIGHT_WRIST       ].parent = AVATAR_JOINT_RIGHT_ELBOW;
    joint[ AVATAR_JOINT_RIGHT_FINGERTIPS  ].parent = AVATAR_JOINT_RIGHT_WRIST;
    joint[ AVATAR_JOINT_LEFT_HIP		  ].parent = AVATAR_JOINT_PELVIS;
    joint[ AVATAR_JOINT_LEFT_KNEE		  ].parent = AVATAR_JOINT_LEFT_HIP;
    joint[ AVATAR_JOINT_LEFT_HEEL		  ].parent = AVATAR_JOINT_LEFT_KNEE;
    joint[ AVATAR_JOINT_LEFT_TOES		  ].parent = AVATAR_JOINT_LEFT_HEEL;
    joint[ AVATAR_JOINT_RIGHT_HIP		  ].parent = AVATAR_JOINT_PELVIS;
    joint[ AVATAR_JOINT_RIGHT_KNEE		  ].parent = AVATAR_JOINT_RIGHT_HIP;
    joint[ AVATAR_JOINT_RIGHT_HEEL		  ].parent = AVATAR_JOINT_RIGHT_KNEE;
    joint[ AVATAR_JOINT_RIGHT_TOES		  ].parent = AVATAR_JOINT_RIGHT_HEEL;
    
    // specify the default pose position
    joint[ AVATAR_JOINT_PELVIS           ].defaultPosePosition = glm::vec3(  0.0,   0.0,    0.0  );
    joint[ AVATAR_JOINT_TORSO            ].defaultPosePosition = glm::vec3(  0.0,   0.09,  -0.01 );
    joint[ AVATAR_JOINT_CHEST            ].defaultPosePosition = glm::vec3(  0.0,   0.09,  -0.01 );
    joint[ AVATAR_JOINT_NECK_BASE        ].defaultPosePosition = glm::vec3(  0.0,   0.14,   0.01 );
    joint[ AVATAR_JOINT_HEAD_BASE        ].defaultPosePosition = glm::vec3(  0.0,   0.04,   0.00 );
    
    joint[ AVATAR_JOINT_LEFT_COLLAR      ].defaultPosePosition = glm::vec3( -0.06,  0.04,   0.01 );
    joint[ AVATAR_JOINT_LEFT_SHOULDER    ].defaultPosePosition = glm::vec3( -0.05,  0.0,    0.01 );
    joint[ AVATAR_JOINT_LEFT_ELBOW       ].defaultPosePosition = glm::vec3(  0.0,  -0.16,   0.0  );
    joint[ AVATAR_JOINT_LEFT_WRIST       ].defaultPosePosition = glm::vec3(  0.0,  -0.117,  0.0  );
    joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].defaultPosePosition = glm::vec3(  0.0,  -0.1,    0.0  );
    
    joint[ AVATAR_JOINT_RIGHT_COLLAR     ].defaultPosePosition = glm::vec3(  0.06,  0.04,   0.01 );
    joint[ AVATAR_JOINT_RIGHT_SHOULDER   ].defaultPosePosition = glm::vec3(  0.05,  0.0,    0.01 );
    joint[ AVATAR_JOINT_RIGHT_ELBOW      ].defaultPosePosition = glm::vec3(  0.0,  -0.16,   0.0  );
    joint[ AVATAR_JOINT_RIGHT_WRIST      ].defaultPosePosition = glm::vec3(  0.0,  -0.117,  0.0  );
    joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].defaultPosePosition = glm::vec3(  0.0,  -0.1,    0.0  );
    
    joint[ AVATAR_JOINT_LEFT_HIP         ].defaultPosePosition = glm::vec3( -0.05,  0.0,    0.02 );
    joint[ AVATAR_JOINT_LEFT_KNEE        ].defaultPosePosition = glm::vec3(  0.01, -0.25,  -0.03 );
    joint[ AVATAR_JOINT_LEFT_HEEL        ].defaultPosePosition = glm::vec3(  0.01, -0.22,   0.08 );
    joint[ AVATAR_JOINT_LEFT_TOES        ].defaultPosePosition = glm::vec3(  0.00, -0.03,  -0.05 );
    
    joint[ AVATAR_JOINT_RIGHT_HIP        ].defaultPosePosition = glm::vec3(  0.05,  0.0,    0.02 );
    joint[ AVATAR_JOINT_RIGHT_KNEE       ].defaultPosePosition = glm::vec3( -0.01, -0.25,  -0.03 );
    joint[ AVATAR_JOINT_RIGHT_HEEL       ].defaultPosePosition = glm::vec3( -0.01, -0.22,   0.08 );
    joint[ AVATAR_JOINT_RIGHT_TOES       ].defaultPosePosition = glm::vec3(  0.00, -0.03,  -0.05 );
     
    // specify the radii of the joints
    joint[ AVATAR_JOINT_PELVIS           ].radius = 0.07;
    joint[ AVATAR_JOINT_TORSO            ].radius = 0.065;
    joint[ AVATAR_JOINT_CHEST            ].radius = 0.08;
    joint[ AVATAR_JOINT_NECK_BASE        ].radius = 0.03;
    joint[ AVATAR_JOINT_HEAD_BASE        ].radius = 0.07;
    
    joint[ AVATAR_JOINT_LEFT_COLLAR      ].radius = 0.04;
    joint[ AVATAR_JOINT_LEFT_SHOULDER    ].radius = 0.03;
    joint[ AVATAR_JOINT_LEFT_ELBOW	     ].radius = 0.02;
    joint[ AVATAR_JOINT_LEFT_WRIST       ].radius = 0.02;
    joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].radius = 0.01;
    
    joint[ AVATAR_JOINT_RIGHT_COLLAR     ].radius = 0.04;
    joint[ AVATAR_JOINT_RIGHT_SHOULDER	 ].radius = 0.03;
    joint[ AVATAR_JOINT_RIGHT_ELBOW	     ].radius = 0.02;
    joint[ AVATAR_JOINT_RIGHT_WRIST	     ].radius = 0.02;
    joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].radius = 0.01;
    
    joint[ AVATAR_JOINT_LEFT_HIP		 ].radius = 0.04;
    joint[ AVATAR_JOINT_LEFT_KNEE		 ].radius = 0.025;
    joint[ AVATAR_JOINT_LEFT_HEEL		 ].radius = 0.025;
    joint[ AVATAR_JOINT_LEFT_TOES		 ].radius = 0.025;
    
    joint[ AVATAR_JOINT_RIGHT_HIP		 ].radius = 0.04;
    joint[ AVATAR_JOINT_RIGHT_KNEE		 ].radius = 0.025;
    joint[ AVATAR_JOINT_RIGHT_HEEL		 ].radius = 0.025;
    joint[ AVATAR_JOINT_RIGHT_TOES		 ].radius = 0.025;
    
    /*
    // to aid in hand-shaking and hand-holding, the right hand is not collidable
    joint[ AVATAR_JOINT_RIGHT_ELBOW	    ].isCollidable = false;
    joint[ AVATAR_JOINT_RIGHT_WRIST	    ].isCollidable = false;
    joint[ AVATAR_JOINT_RIGHT_FINGERTIPS].isCollidable = false; 
    */
    
    // calculate bone length
    calculateBoneLengths();
    
    //set spring positions to be in the skeleton bone positions
    initializeBodySprings();
}

void Skeleton::update(float deltaTime, const glm::quat& orientation, glm::vec3 position) {

    // calculate positions of all bones by traversing the skeleton tree:
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        if (joint[b].parent == AVATAR_JOINT_NULL) {
            joint[b].rotation = orientation;
            joint[b].position = position;
        }
        else {
            joint[b].rotation = joint[ joint[b].parent ].rotation;
            joint[b].position = joint[ joint[b].parent ].position;
        }

        // the following will be replaced by a proper rotation...close
        glm::vec3 rotatedJointVector = joint[b].rotation * joint[b].defaultPosePosition;

        joint[b].position += rotatedJointVector;
    }    
}

void Skeleton::initializeBodySprings() {
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        joint[b].springyPosition = joint[b].position;
        joint[b].springyVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

void Skeleton::calculateBoneLengths() {
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        joint[b].length = glm::length(joint[b].defaultPosePosition);
    }
}

float Skeleton::getArmLength() {

    return joint[ AVATAR_JOINT_RIGHT_ELBOW      ].length
         + joint[ AVATAR_JOINT_RIGHT_WRIST	    ].length
         + joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].length;
}

float Skeleton::getHeight() {

    return 
        joint[ AVATAR_JOINT_LEFT_HEEL ].radius +
        joint[ AVATAR_JOINT_LEFT_HEEL ].length +
        joint[ AVATAR_JOINT_LEFT_KNEE ].length +
        joint[ AVATAR_JOINT_PELVIS    ].length +
        joint[ AVATAR_JOINT_TORSO     ].length +
        joint[ AVATAR_JOINT_CHEST     ].length +
        joint[ AVATAR_JOINT_NECK_BASE ].length +
        joint[ AVATAR_JOINT_HEAD_BASE ].length +
        joint[ AVATAR_JOINT_HEAD_BASE ].radius;
}

float Skeleton::getPelvisStandingHeight() {
    
    return joint[ AVATAR_JOINT_LEFT_HEEL ].radius +
           joint[ AVATAR_JOINT_LEFT_HEEL ].length +
           joint[ AVATAR_JOINT_LEFT_KNEE ].length;
}

float Skeleton::getPelvisFloatingHeight() {
    
    return joint[ AVATAR_JOINT_LEFT_HEEL ].radius +
           joint[ AVATAR_JOINT_LEFT_HEEL ].length +
           joint[ AVATAR_JOINT_LEFT_KNEE ].length +
           FLOATING_HEIGHT;
}




