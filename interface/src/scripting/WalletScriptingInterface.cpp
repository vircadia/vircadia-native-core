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

WalletScriptingInterface::WalletScriptingInterface() {
}

static const QString CHECKOUT_QML_PATH = qApp->applicationDirPath() + "../../../qml/hifi/commerce/checkout/Checkout.qml";
void WalletScriptingInterface::buy() {
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));

    if (!tablet->isPathLoaded(CHECKOUT_QML_PATH)) {
        tablet->loadQMLSource(CHECKOUT_QML_PATH);
    }
    DependencyManager::get<HMDScriptingInterface>()->openTablet();

    QVariant message = {};


    tablet->sendToQml(message); .sendToQml({
    method: 'updateCheckoutQML', params : {
    itemId: '0d90d21c-ce7a-4990-ad18-e9d2cf991027',
        itemName : 'Test Flaregun',
        itemAuthor : 'hifiDave',
        itemPrice : 17,
        itemHref : 'http://mpassets.highfidelity.com/0d90d21c-ce7a-4990-ad18-e9d2cf991027-v1/flaregun.json',
    },
        canRezCertifiedItems: Entities.canRezCertified || Entities.canRezTmpCertified
    });
}