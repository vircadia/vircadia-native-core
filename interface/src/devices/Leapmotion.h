//
//  Leapmotion.h
//  interface/src/devices
//
//  Created by Sam Cake on 6/2/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Leapmotion_h
#define hifi_Leapmotion_h

#include <QDateTime>

#include "MotionTracker.h"

#ifdef HAVE_LEAPMOTION
#include <Leap.h>
#endif

/// Handles interaction with the Leapmotion skeleton tracking suit.
class Leapmotion : public MotionTracker {
public:
    static const Name NAME;

    static void init();
    static void destroy();

    /// Leapmotion MotionTracker factory
    static Leapmotion* getInstance();

    bool isActive() const { return _active; }

    virtual void update() override;

protected:
    Leapmotion();
    virtual ~Leapmotion();

private:
#ifdef HAVE_LEAPMOTION
    Leap::Listener _listener;
    Leap::Controller _controller;
#endif

    bool _active;
};

#endif // hifi_Leapmotion_h
