//
//  HandData.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "HandData.h"

HandData::HandData(AvatarData* owningAvatar) :
    _basePosition(0.0f, 0.0f, 0.0f),
    _baseOrientation(0.0f, 0.0f, 0.0f, 1.0f),
    _owningAvatarData(owningAvatar)
{
    for (int i = 0; i < 2; ++i)
        _palms.push_back(PalmData(this));
}

PalmData::PalmData(HandData* owningHandData) :
_rawPosition(0, 0, 0),
_rawNormal(0, 1, 0),
_isActive(false),
_owningHandData(owningHandData)
{
    for (int i = 0; i < 5; ++i)
        _fingers.push_back(FingerData(this, owningHandData));
}

FingerData::FingerData(PalmData* owningPalmData, HandData* owningHandData) :
_tipRawPosition(0, 0, 0),
_rootRawPosition(0, 0, 0),
_isActive(false),
_owningPalmData(owningPalmData),
_owningHandData(owningHandData)
{
}
