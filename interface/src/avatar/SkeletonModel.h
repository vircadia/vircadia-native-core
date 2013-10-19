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
    
    void simulate(float deltaTime);
    
private:
    
    Avatar* _owningAvatar;
};

#endif /* defined(__interface__SkeletonModel__) */
