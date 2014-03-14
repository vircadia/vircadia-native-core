//
//  SkeletonModel.h
//  interface
//
//  Created by Andrzej Kapolka on 10/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__SkeletonModel__
#define __interface__SkeletonModel__


#include "renderer/Model.h"

class Avatar;

/// A skeleton loaded from a model.
class SkeletonModel : public Model {
    Q_OBJECT
    
public:

    SkeletonModel(Avatar* owningAvatar);
    
    void simulate(float deltaTime, bool fullUpdate = true);
    bool render(float alpha);

    /// \param jointIndex index of hand joint
    /// \param shapes[out] list in which is stored pointers to hand shapes
    void getHandShapes(int jointIndex, QVector<const Shape*>& shapes) const;
    
protected:
    
    void applyHandPosition(int jointIndex, const glm::vec3& position);
    
    void applyPalmData(int jointIndex, const QVector<int>& fingerJointIndices,
        const QVector<int>& fingertipJointIndices, PalmData& palm);
    
    /// Updates the state of the joint at the specified index.
    virtual void updateJointState(int index);   
    
    virtual void maybeUpdateLeanRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    
private:
    
    /// Using the current position and rotation of the identified (hand) joint, computes a
    /// reasonable stretched configuration for the connected arm.
    void stretchArm(int jointIndex, const glm::vec3& position);
    
    Avatar* _owningAvatar;
};

#endif /* defined(__interface__SkeletonModel__) */
