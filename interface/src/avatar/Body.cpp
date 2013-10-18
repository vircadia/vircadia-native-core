//
//  Body.cpp
//  interface
//
//  Created by Andrzej Kapolka on 10/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <glm/gtx/transform.hpp>

#include "Application.h"
#include "Body.h"

Body::Body(Avatar* owningAvatar) : _owningAvatar(owningAvatar) {
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
}

void Body::simulate(float deltaTime) {
    if (!isActive()) {
        return;
    }
    
    // set up joint states on first simulate after load
    const FBXGeometry& geometry = _skeletonGeometry->getFBXGeometry();
    if (_jointStates.isEmpty()) {
        foreach (const FBXJoint& joint, geometry.joints) {
            JointState state;
            state.rotation = joint.rotation;
            _jointStates.append(state);
        }
    }
    
    glm::quat orientation = _owningAvatar->getOrientation();
    const float MODEL_SCALE = 0.05f;
    glm::vec3 scale = glm::vec3(-1.0f, 1.0f, -1.0f) * _owningAvatar->getScale() * MODEL_SCALE;
    glm::mat4 baseTransform = glm::translate(_owningAvatar->getPosition()) * glm::mat4_cast(orientation) * glm::scale(scale);
    
    // update the world space transforms for all joints
    for (int i = 0; i < _jointStates.size(); i++) {
        JointState& state = _jointStates[i];
        const FBXJoint& joint = geometry.joints.at(i);
        if (joint.parentIndex == -1) {
            state.transform = baseTransform * geometry.offset * joint.preRotation *
                glm::mat4_cast(state.rotation) * joint.postRotation;
        
        } else {
            state.transform = _jointStates[joint.parentIndex].transform * joint.preRotation *
                glm::mat4_cast(state.rotation) * joint.postRotation;
        }
    }
}

bool Body::render(float alpha) {
    if (_jointStates.isEmpty()) {
        return false;
    }
    
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    
    for (int i = 0; i < _jointStates.size(); i++) {
        const JointState& state = _jointStates[i];
        glPushMatrix();
        glMultMatrixf((const GLfloat*)&state.transform);
        
        glutSolidSphere(0.2f, 10, 10);
        
        glPopMatrix();
    }
    
    return true;
}
    
void Body::setSkeletonModelURL(const QUrl& url) {
    // don't recreate the geometry if it's the same URL
    if (_skeletonModelURL == url) {
        return;
    }
    _skeletonModelURL = url;
    _skeletonGeometry = Application::getInstance()->getGeometryCache()->getGeometry(url);
}
