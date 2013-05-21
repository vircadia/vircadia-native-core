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

/*
AvatarRenderer::AvatarRenderer() {
}

// this method renders the avatar 
void AvatarRenderer::render() {


    // show avatar position
    glColor4f(0.5f, 0.5f, 0.5f, 0.6);
    glPushMatrix();
    glm::vec3 j( getJointPosition( AVATAR_JOINT_PELVIS ) );    
    glTranslatef(j.x, j.y, j.z);
    glScalef(0.08, 0.08, 0.08);
    glutSolidSphere(1, 10, 10);
    glPopMatrix();
    
    renderDiskShadow(getJointPosition( AVATAR_JOINT_PELVIS ), glm::vec3(0.0f, 1.0f, 0.0f), 0.1f, 0.2f);
    
    //renderBody(lookingInMirror);
}

void AvatarRenderer::renderBody() {
    //  Render joint positions as spheres
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        
        if (b == AVATAR_JOINT_HEAD_BASE) { // the head is rendered as a special case
            if (_displayingHead) {
                _head.render(lookingInMirror);
            }
        } else {
    
            //show direction vectors of the bone orientation
            //renderOrientationDirections(_joint[b].springyPosition, _joint[b].orientation, _joint[b].radius * 2.0);
            
            glColor3fv(_avatar->skinColor);
            glPushMatrix();
            glTranslatef(_avatar->[b].springyPosition.x, _avatar->_joint[b].springyPosition.y, _avatar->_joint[b].springyPosition.z);
            glutSolidSphere(_avatar->_joint[b].radius, 20.0f, 20.0f);
            glPopMatrix();
        }
        
        if (_joint[b].touchForce > 0.0f) {
        
            float alpha = _joint[b].touchForce * 0.2;
            float r = _joint[b].radius * 1.1f + 0.005f;
            glColor4f(0.5f, 0.2f, 0.2f, alpha);
            glPushMatrix();
            glTranslatef(_joint[b].springyPosition.x, _joint[b].springyPosition.y, _joint[b].springyPosition.z);
            glScalef(r, r, r);
            glutSolidSphere(1, 20, 20);
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
}
*/
    
