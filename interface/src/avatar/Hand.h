//
//  Hand.h
//  interface/src/avatar
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Hand_h
#define hifi_Hand_h

#include <vector>

#include <QAction>

#include <glm/glm.hpp>

#include <SharedUtil.h>

#include <AvatarData.h>
#include <AudioScriptingInterface.h>
#include <HandData.h>
#include <Model.h>

#include "world.h"


class Avatar;

const float HAND_PADDLE_OFFSET = 0.1f;
const float HAND_PADDLE_THICKNESS = 0.01f;
const float HAND_PADDLE_RADIUS = 0.15f;

class Hand : public HandData {
public:
    Hand(Avatar* owningAvatar);
    
    void simulate(float deltaTime, bool isMine);
    void render(RenderArgs* renderArgs, bool isMine);

    void collideAgainstAvatar(Avatar* avatar, bool isMyHand);

    void resolvePenetrations();

private:
    // disallow copies of the Hand, copy of owning Avatar is disallowed too
    Hand(const Hand&);
    Hand& operator= (const Hand&);
        
    int _controllerButtons;             ///  Button states read from hand-held controllers

    Avatar*        _owningAvatar;
    
    void renderHandTargets(RenderArgs* renderArgs, bool isMine);
};

#endif // hifi_Hand_h
