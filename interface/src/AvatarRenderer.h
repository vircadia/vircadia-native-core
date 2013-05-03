//
//  AvatarRenderer.h
//  interface
//
//  Created by Jeffrey Ventrella
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AvatarRenderer__
#define __interface__AvatarRenderer__

#include "Avatar.h"
#include <glm/glm.hpp>

class AvatarRenderer {
public:

    AvatarRenderer();
    void render(Avatar *avatar, bool lookingInMirror);

private:

};

#endif
