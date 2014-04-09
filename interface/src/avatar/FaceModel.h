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

#ifndef __interface__FaceModel__
#define __interface__FaceModel__

#include "renderer/Model.h"

class Head;

/// A face formed from a linear mix of blendshapes according to a set of coefficients.
class FaceModel : public Model {
    Q_OBJECT
    
public:

    FaceModel(Head* owningHead);

    virtual void simulate(float deltaTime, bool fullUpdate = true);
    
protected:

    virtual void maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    virtual void maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    
private:
    
    Head* _owningHead;
};

#endif /* defined(__interface__FaceModel__) */
