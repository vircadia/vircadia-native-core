//
//  WalletScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Zach Fox on 2017-09-29.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WalletScriptingInterface.h"

CheckoutProxy::CheckoutProxy(QObject* qmlObject, QObject* parent) : QmlWrapper(qmlObject, parent) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
}

WalletScriptingInterface::WalletScriptingInterface() {
}

static const QString CHECKOUT_QML_PATH = qApp->applicationDirPath() + "../../../qml/hifi/commerce/checkout/Checkout.qml";
void WalletScriptingInterface::buy(const QString& name, const QString& id, const int& price, const QString& href) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "buy", Q_ARG(const QString&, name), Q_ARG(const QString&, id), Q_ARG(const int&, price), Q_ARG(const QString&, href));
        return;
    }

    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));

    tablet->loadQMLSource(CHECKOUT_QML_PATH);
    DependencyManager::get<HMDScriptingInterface>()->openTablet();

    QQuickItem* root = nullptr;
    if (tablet->getToolbarMode() || (!tablet->getTabletRoot() && !qApp->isHMDMode())) {
        root = DependencyManager::get<OffscreenUi>()->getRootItem();
    } else {
        root = tablet->getTabletRoot();
    }
    CheckoutProxy* checkout = new CheckoutProxy(root->findChild<QObject*>("checkout"));

    // Example: Wallet.buy("Test Flaregun", "0d90d21c-ce7a-4990-ad18-e9d2cf991027", 17, "http://mpassets.highfidelity.com/0d90d21c-ce7a-4990-ad18-e9d2cf991027-v1/flaregun.json");
    checkout->writeProperty("itemName", name);
    checkout->writeProperty("itemId", id);
    checkout->writeProperty("itemPrice", price);
    checkout->writeProperty("itemHref", href);
}