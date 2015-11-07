//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Deck_h
#define hifi_Recording_Deck_h

#include "Forward.h"

#include <QtCore/QObject>

class QIODevice;

namespace recording {

class Deck : public QObject {
public:
    using Pointer = std::shared_ptr<Deck>;

    Deck(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~Deck();

    // Place a clip on the deck for recording or playback
    void queueClip(ClipPointer clip, float timeOffset = 0.0f);
    void play(float timeOffset = 0.0f);
    void reposition(float timeOffsetDelta);
    void setPlaybackSpeed(float rate);
};

}

#endif
