//
//  MyCharacterController.h
//  interface/src/avatar
//
//  Created by AndrewMeadows 2015.10.21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_MyCharacterController_h
#define hifi_MyCharacterController_h

#include <CharacterController.h>
//#include <SharedUtil.h>

class btCollisionShape;
class MyAvatar;

class MyCharacterController : public CharacterController {
public:
    explicit MyCharacterController(MyAvatar* avatar);
    ~MyCharacterController ();

    virtual void updateShapeIfNecessary() override;

protected:
    MyAvatar* _avatar { nullptr };
};

#endif // hifi_MyCharacterController_h
