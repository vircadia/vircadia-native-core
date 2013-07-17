//
//  Hand.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef hifi_Hand_h
#define hifi_Hand_h

#include <glm/glm.hpp>
#include <AvatarData.h>
#include <HandData.h>
#include "Balls.h"
#include "world.h"
#include "InterfaceConfig.h"
#include "SerialInterface.h"
#include <SharedUtil.h>
#include <vector>


class Avatar;
class ProgramObject;

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
        float            touchForce;     // a scalar determining the amount that the cursor (or hand) is penetrating the ball
    };

    void init();
    void reset();
    void simulate(float deltaTime, bool isMine);
    void render(bool lookingInMirror);

    void setBallColor      (glm::vec3 ballColor         ) { _ballColor          = ballColor;          }
    void setLeapFingers    (const std::vector<glm::vec3>& fingerTips,
                            const std::vector<glm::vec3>& fingerRoots);
    void setLeapHands      (const std::vector<glm::vec3>& handPositions,
                            const std::vector<glm::vec3>& handNormals);
    void setRaveGloveActive(bool active) { _isRaveGloveActive = active; }

    // getters
    const glm::vec3& getLeapBallPosition       (int ball)       const { return _leapBalls[ball].position;}
    bool isRaveGloveActive                     ()               const { return _isRaveGloveActive; }

private:
    // disallow copies of the Hand, copy of owning Avatar is disallowed too
    Hand(const Hand&);
    Hand& operator= (const Hand&);

    Avatar*     _owningAvatar;
    float       _renderAlpha;
    bool        _lookingInMirror;
    bool        _isRaveGloveActive;
    glm::vec3   _ballColor;
//    glm::vec3   _position;
//    glm::quat   _orientation;
    std::vector<HandBall>	_leapBalls;
    
    // private methods
    void renderRaveGloveStage();
    void renderHandSpheres();
    void calculateGeometry();
};

#endif
