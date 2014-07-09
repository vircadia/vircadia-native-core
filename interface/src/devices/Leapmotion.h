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
    Q_OBJECT

public:

    Leapmotion();
    virtual ~Leapmotion();

    void init();

    bool isActive() const { return _active; }
    
    void update();
    void reset();

public slots:

    void updateEnabled();

private:
#ifdef HAVE_LEAPMOTION

    class SampleListener : public ::Leap::Listener
    {
    public:
        SampleListener();
        virtual ~SampleListener();

        virtual void onConnect(const ::Leap::Controller &);
        virtual void onDisconnect(const ::Leap::Controller &);
        virtual void onExit(const ::Leap::Controller &);
        virtual void onFocusGained(const ::Leap::Controller &);
        virtual void onFocusLost(const ::Leap::Controller &);
        virtual void onFrame(const ::Leap::Controller &);
        virtual void onInit(const ::Leap::Controller &);
        virtual void onServiceConnect(const ::Leap::Controller &);
        virtual void onServiceDisconnect(const ::Leap::Controller &);
    
    };

    SampleListener _listener;
    Leap::Controller _controller;
#endif
    glm::vec3           _leapBasePos;
    glm::quat           _leapBaseOri;

    void setEnabled(bool enabled);

    bool _enabled;
    bool _active;
};

#endif // hifi_Leapmotion_h
