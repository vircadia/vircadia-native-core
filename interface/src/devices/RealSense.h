//
//  RealSense.h
//  interface/src/devices
//
//  Created by Thijs Wenker on 12/10/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RealSense_h
#define hifi_RealSense_h

#include <QDateTime>

#include "MotionTracker.h"

#ifdef HAVE_RSSDK
#include <pxcsession.h>
#include <pxchandmodule.h>
#include <pxchandconfiguration.h>
#include <pxcsensemanager.h>
#include <pxchanddata.h>
#endif

/// Handles interaction with the RealSense skeleton tracking suit.
class RealSense : public QObject, public MotionTracker {
    Q_OBJECT

public:
    static const Name NAME;

    static void init();

    /// RealSense MotionTracker factory
    static RealSense* getInstance();

    bool isActive() const { return _active; }

    virtual void update();

    void loadRSSDKFile();

protected:
    RealSense();
    virtual ~RealSense();

    void initSession(bool playback, QString filename);

private:
#ifdef HAVE_RSSDK
    PXCSession* _session;
    PXCSenseManager* _manager;
    PXCHandModule* _handController;
    PXCHandData* _handData;
    PXCHandConfiguration* _config;
#endif
    bool _properlyInitialized;
    bool _active;    
};

#endif // hifi_RealSense_h
