//
//  Created by Wayne Chen on 2018/12/20
//  Copyright 2018 High Fidelity Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2-0.html
//
#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <functional>

using OculusTicketRequestCallback = std::function<void(QString, QString)>;


class OculusPlatformPlugin {
public:
    virtual ~OculusPlatformPlugin() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    virtual bool isRunning() = 0;

    virtual void runCallbacks() = 0;

    virtual void requestTicket(OculusTicketRequestCallback callback) = 0;

    virtual QString getUserProof() = 0;

    virtual QString getLoggedInUserID() = 0;

    virtual QString getOculusVRBuildID() = 0;
};
