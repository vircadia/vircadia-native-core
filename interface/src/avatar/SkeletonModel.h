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

protected:

    void applyHandPosition(int jointIndex, const glm::vec3& position);
    
    void applyPalmData(int jointIndex, const QVector<int>& fingerJointIndices,
        const QVector<int>& fingertipJointIndices, PalmData& palm);
    
    /// Updates the state of the joint at the specified index.
    virtual void updateJointState(int index);   
    
    virtual void maybeUpdateLeanRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    
private:
    
    Avatar* _owningAvatar;
};

#endif // hifi_SkeletonModel_h
