//
//  Created by Wayne Chen on 2018/12/20
//  Copyright 2018 High Fidelity Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2-0.html
//
#pragma once

#include <QtCore/QString>

#include <functional>

using NonceUserIDCallback = std::function<void(QString, QString)>;

class OculusPlatformPlugin {
public:
    virtual ~OculusPlatformPlugin() = default;

    virtual const QString getName() const = 0;

    virtual void requestNonceAndUserID(NonceUserIDCallback callback) = 0;

    virtual void handleOVREvents() = 0;
};
