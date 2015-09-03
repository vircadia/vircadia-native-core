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

#include <HandData.h>

class Avatar;
class RenderArgs;

class Hand : public HandData {
public:
    Hand(Avatar* owningAvatar);
    
    void simulate(float deltaTime, bool isMine);
    void renderHandTargets(RenderArgs* renderArgs, bool isMine);

private:
    // disallow copies of the Hand, copy of owning Avatar is disallowed too
    Hand(const Hand&);
    Hand& operator= (const Hand&);
        
    int _controllerButtons; ///  Button states read from hand-held controllers

    Avatar* _owningAvatar;
};

#endif // hifi_Hand_h
