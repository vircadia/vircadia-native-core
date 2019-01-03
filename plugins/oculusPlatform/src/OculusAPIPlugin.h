//
//  Created by Wayne Chen on 2018/12/20
//  Copyright 2018 High Fidelity Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2-0.html
//
#pragma once

#include <plugins/OculusPlatformPlugin.h>

class OculusAPIPlugin : public OculusPlatformPlugin {
public:
    bool init() override;
    void shutdown() override;

    bool isRunning() override;

    void runCallbacks() override;

    void requestTicket(TicketRequestCallback callback) override;

    QString requestUserProof() override;
    QString getLoggedInUserID() override;

    QString getOculusVRBuildID() override;
};
