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

#include "InterfaceConfig.h"
#include "renderer/Model.h"
#include "world.h"


class Avatar;
class ProgramObject;

const float HAND_PADDLE_OFFSET = 0.1f;
const float HAND_PADDLE_THICKNESS = 0.01f;
const float HAND_PADDLE_RADIUS = 0.15f;

class Hand : public HandData {
public:
    Hand(Avatar* owningAvatar);
    
    struct HandBall
    {
        glm::vec3        position;       // the actual dynamic position of the ball at any given time
        glm::quat        rotation;       // the rotation of the ball
        glm::vec3        velocity;       // the velocity of the ball
        float            radius;         // the radius of the ball
        bool             isCollidable;   // whether or not the ball responds to collisions
        bool             isColliding;    // ball is currently colliding
        float            touchForce;     // a scalar determining the amount that the cursor (or hand) is penetrating the ball
    };
    
    void simulate(float deltaTime, bool isMine);
    void render(bool isMine, Model::RenderMode renderMode = Model::DEFAULT_RENDER_MODE);

    void collideAgainstAvatar(Avatar* avatar, bool isMyHand);

    void resolvePenetrations();

private:
    // disallow copies of the Hand, copy of owning Avatar is disallowed too
    Hand(const Hand&);
    Hand& operator= (const Hand&);
        
    int _controllerButtons;             ///  Button states read from hand-held controllers

    Avatar*        _owningAvatar;
    
    void renderHandTargets(bool isMine);
};

#endif // hifi_Hand_h
