//
//  FaceModel.h
//  interface
//
//  Created by Andrzej Kapolka on 9/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
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

    void simulate(float deltaTime);
    bool render(float alpha);
    
protected:

    virtual void maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    virtual void maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    
private:
    
    Head* _owningHead;
};

#endif /* defined(__interface__FaceModel__) */
