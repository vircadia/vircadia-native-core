//
//  AvatarTouch.h
//  interface
//
//  Created by Jeffrey Ventrella
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AvatarTouch__
#define __interface__AvatarTouch__

#include <glm/glm.hpp>

const float HANDS_CLOSE_ENOUGH_TO_GRASP = 0.1;

class AvatarTouch {
public:

    AvatarTouch();

    void simulate(float deltaTime);
    void render();
    
    void setMyHandPosition  (glm::vec3 position);
    void setYourHandPosition(glm::vec3 position);
    void setMyBodyPosition  (glm::vec3 position);
    void setYourBodyPosition(glm::vec3 position);
    void setMyHandState     (int state);
    void setYourHandState   (int state);
    void setReachableRadius (float r);
    void setAbleToReachOtherAvatar (bool a) {_canReachToOtherAvatar   = a;}
    void setHandsCloseEnoughToGrasp(bool h) {_handsCloseEnoughToGrasp = h;}

    bool getAbleToReachOtherAvatar () {return _canReachToOtherAvatar;}
    bool getHandsCloseEnoughToGrasp() {return _handsCloseEnoughToGrasp;}

private:

    static const int NUM_POINTS = 100;
    
    glm::vec3 _point [NUM_POINTS];
    glm::vec3 _myBodyPosition;
    glm::vec3 _yourBodyPosition;
    glm::vec3 _myHandPosition;
    glm::vec3 _yourHandPosition;
    int       _myHandState;
    int       _yourHandState;
    bool      _canReachToOtherAvatar;
    bool      _handsCloseEnoughToGrasp;
    float     _reachableRadius;
};

#endif
