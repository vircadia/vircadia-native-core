//
//  AvatarRenderer.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AvatarRenderer__
#define __interface__AvatarRenderer__

#include "Avatar.h"
#include <glm/glm.hpp>

class AvatarRenderer {
public:

    AvatarRenderer();
    void render(Avatar *avatarToRender, bool lookingInMirror, glm::vec3 cameraPosition );

private:

    Avatar *avatar;
    void renderBody();
};

#endif
