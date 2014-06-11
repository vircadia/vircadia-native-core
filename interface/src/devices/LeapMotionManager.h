//
//  LeapMotionManager.h
//  interface/src/devices
//
//  Created by Sam Cake on 6/2/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LeapMotionManager_h
#define hifi_LeapMotionManager_h

#include <QDateTime>
#include <QObject>
#include <QVector>

#include <glm/gtc/quaternion.hpp>

#ifdef HAVE_LEAPMOTION
#include <Leap.h>
/*extern "C" {
#include <yei_skeletal_api.h>
}*/
#endif

/// Handles interaction with the LeapMotionManager skeleton tracking suit.
class LeapMotionManager : public QObject {
    Q_OBJECT

public:
    
    LeapMotionManager();
    virtual ~LeapMotionManager();

    bool isActive() const { return !_hands.isEmpty(); }

    int nbHands() const { return _hands.size(); }

    glm::vec3 getHandPos( unsigned int handNb ) const;

    const QVector<int>& getHumanIKJointIndices() const { return _humanIKJointIndices; }
    const QVector<glm::quat>& getJointRotations() const { return _jointRotations; }
    
    void update(float deltaTime);
    void reset();

private slots:

    void renderCalibrationCountdown();

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
    QVector<glm::vec3>  _hands;

    QVector<int> _humanIKJointIndices;
    QVector<glm::quat> _jointRotations;
    QVector<glm::quat> _lastJointRotations;
    
    QDateTime _calibrationCountdownStarted;
};

#endif // hifi_LeapMotionManager_h
