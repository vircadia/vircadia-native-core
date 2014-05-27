//
//  SkeletonModel.h
//  interface/src/avatar
//
//  Created by Andrzej Kapolka on 10/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SkeletonModel_h
#define hifi_SkeletonModel_h

#include "renderer/Model.h"

class Avatar;

/// A skeleton loaded from a model.
class SkeletonModel : public Model {
    Q_OBJECT
    
public:

    SkeletonModel(Avatar* owningAvatar);
    
    void simulate(float deltaTime, bool fullUpdate = true);

    /// \param jointIndex index of hand joint
    /// \param shapes[out] list in which is stored pointers to hand shapes
    void getHandShapes(int jointIndex, QVector<const Shape*>& shapes) const;

    ///  \param shapes[out] list of shapes for body collisions
    void getBodyShapes(QVector<const Shape*>& shapes) const;

    void renderIKConstraints();
    
    /// Returns the index of the left hand joint, or -1 if not found.
    int getLeftHandJointIndex() const { return isActive() ? _geometry->getFBXGeometry().leftHandJointIndex : -1; }
    
    /// Returns the index of the right hand joint, or -1 if not found.
    int getRightHandJointIndex() const { return isActive() ? _geometry->getFBXGeometry().rightHandJointIndex : -1; }

    /// Retrieve the position of the left hand
    /// \return true whether or not the position was found
    bool getLeftHandPosition(glm::vec3& position) const;
    
    /// Retrieve the rotation of the left hand
    /// \return true whether or not the rotation was found
    bool getLeftHandRotation(glm::quat& rotation) const;
    
    /// Retrieve the position of the right hand
    /// \return true whether or not the position was found
    bool getRightHandPosition(glm::vec3& position) const;
    
    /// Retrieve the rotation of the right hand
    /// \return true whether or not the rotation was found
    bool getRightHandRotation(glm::quat& rotation) const;
    
    /// Restores some percentage of the default position of the left hand.
    /// \param percent the percentage of the default position to restore
    /// \return whether or not the left hand joint was found
    bool restoreLeftHandPosition(float percent = 1.0f, float priority = 1.0f);
    
    /// Gets the position of the left shoulder.
    /// \return whether or not the left shoulder joint was found
    bool getLeftShoulderPosition(glm::vec3& position) const;
    
    /// Returns the extended length from the left hand to its last free ancestor.
    float getLeftArmLength() const;
    
    /// Restores some percentage of the default position of the right hand.
    /// \param percent the percentage of the default position to restore
    /// \return whether or not the right hand joint was found
    bool restoreRightHandPosition(float percent = 1.0f, float priority = 1.0f);
    
    /// Gets the position of the right shoulder.
    /// \return whether or not the right shoulder joint was found
    bool getRightShoulderPosition(glm::vec3& position) const;
    
    /// Returns the extended length from the right hand to its first free ancestor.
    float getRightArmLength() const;

    /// Returns the position of the head joint.
    /// \return whether or not the head was found
    bool getHeadPosition(glm::vec3& headPosition) const;
    
    /// Returns the position of the neck joint.
    /// \return whether or not the neck was found
    bool getNeckPosition(glm::vec3& neckPosition) const;
    
    /// Returns the rotation of the neck joint's parent.
    /// \return whether or not the neck was found
    bool getNeckParentRotation(glm::quat& neckRotation) const;
    
    /// Retrieve the positions of up to two eye meshes.
    /// \return whether or not both eye meshes were found
    bool getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const;
    
protected:

    void applyHandPosition(int jointIndex, const glm::vec3& position);
    
    void applyPalmData(int jointIndex, PalmData& palm);
    
    /// Updates the state of the joint at the specified index.
    virtual void updateJointState(int index);   
    
    void maybeUpdateLeanRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    void maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    void maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    
private:

    void renderJointConstraints(int jointIndex);
    void setHandPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation);
    
    Avatar* _owningAvatar;
};

#endif // hifi_SkeletonModel_h
