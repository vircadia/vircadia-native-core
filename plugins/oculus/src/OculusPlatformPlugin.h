//
//  Created by Wayne Chen on 2019/01/08
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <plugins/OculusPlatformPlugin.h>

#include <OVR_CAPI_GL.h>

#define OVRPL_DISABLED
#include <OVR_Platform.h>

class OculusAPIPlugin : public OculusPlatformPlugin {
public:
    OculusAPIPlugin() = default;
    virtual ~OculusAPIPlugin() = default;
    QString getName() const { return NAME; }
    QString getOculusUserID() const { return _user; };

    bool isRunning() const;

    bool init();

    void shutdown();

    virtual void requestNonceAndUserID(NonceUserIDCallback callback);

    virtual void handleOVREvents();

private:
    static QString NAME;
    NonceUserIDCallback _nonceUserIDCallback;
    QString _nonce;
    bool _nonceChanged{ false };
    bool _userIDChanged{ false };
    QString _user;
    ovrID _userID;
    ovrSession _session;
};
