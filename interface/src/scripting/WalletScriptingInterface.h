
//  WalletScriptingInterface.h
//  interface/src/scripting
//
//  Created by Zach Fox on 2017-09-29.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WalletScriptingInterface_h
#define hifi_WalletScriptingInterface_h

#include <QtCore/QObject>
#include <DependencyManager.h>

#include "scripting/HMDScriptingInterface.h"
#include <ui/TabletScriptingInterface.h>
#include <ui/QmlWrapper.h>
#include <OffscreenUi.h>
#include "Application.h"
#include "commerce/Wallet.h"

class CheckoutProxy : public QmlWrapper {
    Q_OBJECT
public:
    CheckoutProxy(QObject* qmlObject, QObject* parent = nullptr);
};


class WalletScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

    Q_PROPERTY(uint walletStatus READ getWalletStatus WRITE setWalletStatus NOTIFY walletStatusChanged)

public:
    WalletScriptingInterface();

    Q_INVOKABLE void refreshWalletStatus();
    Q_INVOKABLE uint getWalletStatus() { return _walletStatus; }
    // setWalletStatus() should never be made Q_INVOKABLE. If it were,
    //     scripts could cause the Wallet to incorrectly report its status.
    void setWalletStatus(const uint& status);

signals:
    void walletStatusChanged();
    void walletNotSetup();

private:
    uint _walletStatus;
};

#endif // hifi_WalletScriptingInterface_h
