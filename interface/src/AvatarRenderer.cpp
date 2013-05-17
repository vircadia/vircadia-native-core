//
//  AvatarRenderer.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <iostream>
#include <glm/glm.hpp>
#include <SharedUtil.h>
#include "AvatarRenderer.h"
#include "InterfaceConfig.h"

AvatarRenderer::AvatarRenderer() {
}

// this method renders the avatar 
void AvatarRenderer::render(Avatar *avatarToRender, bool lookingInMirror, glm::vec3 cameraPosition) {

    avatar = avatarToRender;

    /*
    // show avatar position
    glColor4f(0.5f, 0.5f, 0.5f, 0.6);
    glPushMatrix();
    glm::vec3 j( avatar->getJointPosition( AVATAR_JOINT_PELVIS ) );    
    glTranslatef(j.x, j.y, j.z);
    glScalef(0.08, 0.08, 0.08);
    glutSolidSphere(1, 10, 10);
    glPopMatrix();
    */
    
    //renderDiskShadow(avatar->getJointPosition( AVATAR_JOINT_PELVIS ), glm::vec3(0.0f, 1.0f, 0.0f), 0.1f, 0.2f);
    
    //renderBody();
}






void AvatarRenderer::renderBody() {
/*
    //  Render joint positions as spheres
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        
        if (b != AVATAR_JOINT_HEAD_BASE) { // the head is rendered as a special case in "renderHead"
    
            //show direction vectors of the bone orientation
            //renderOrientationDirections(_joint[b].springyPosition, _joint[b].orientation, _joint[b].radius * 2.0);
            
            glm::vec3 j( avatar->getJointPosition( AVATAR_JOINT_PELVIS ) );
            glColor3fv(skinColor);
            glPushMatrix();
            glTranslatef(j.x, j.y, j.z);
            glutSolidSphere(_joint[b].radius, 20.0f, 20.0f);
            glPopMatrix();
        }
    }
    
    // Render lines connecting the joint positions
    glColor3f(0.4f, 0.5f, 0.6f);
    glLineWidth(3.0);
    
    for (int b = 1; b < NUM_AVATAR_JOINTS; b++) {
    if (_joint[b].parent != AVATAR_JOINT_NULL) 
        if (b != AVATAR_JOINT_HEAD_TOP) {
            glBegin(GL_LINE_STRIP);
            glVertex3fv(&_joint[ _joint[ b ].parent ].springyPosition.x);
            glVertex3fv(&_joint[ b ].springyPosition.x);
            glEnd();
        }
    }
    */
}
