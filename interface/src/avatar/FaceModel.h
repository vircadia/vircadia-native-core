//
//  FaceModel.h
//  interface/src/avatar
//
//  Created by Andrzej Kapolka on 9/16/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FaceModel_h
#define hifi_FaceModel_h

#include "renderer/Model.h"

class Head;

/// A face formed from a linear mix of blendshapes according to a set of coefficients.
class FaceModel : public Model {
    Q_OBJECT
    
public:

    FaceModel(Head* owningHead);

    virtual void simulate(float deltaTime, bool fullUpdate = true);
    
    virtual void maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    virtual void maybeUpdateEyeRotation(Model* model, const JointState& parentState, const FBXJoint& joint, JointState& state);
    virtual void updateJointState(int index);

    /// Retrieve the positions of up to two eye meshes.
    /// \return whether or not both eye meshes were found
    bool getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const;
    
private:
    
    Head* _owningHead;
};

#endif // hifi_FaceModel_h
