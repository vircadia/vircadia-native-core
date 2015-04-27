//
//  AddressBarDialog.cpp
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AddressBarDialog.h"

#include <QMessageBox>

#include "DependencyManager.h"
#include "AddressManager.h"

HIFI_QML_DEF(AddressBarDialog)

AddressBarDialog::AddressBarDialog(QQuickItem* parent) : OffscreenQmlDialog(parent) {
    auto addressManager = DependencyManager::get<AddressManager>();
    connect(addressManager.data(), &AddressManager::lookupResultIsOffline, this, &AddressBarDialog::displayAddressOfflineMessage);
    connect(addressManager.data(), &AddressManager::lookupResultIsNotFound, this, &AddressBarDialog::displayAddressNotFoundMessage);
    connect(addressManager.data(), &AddressManager::lookupResultsFinished, this, &AddressBarDialog::hide);
}

void AddressBarDialog::hide() {
    ((QQuickItem*)parent())->setEnabled(false);
}

void AddressBarDialog::loadAddress(const QString& address) {
    qDebug() << "Called LoadAddress with address " << address;
    if (!address.isEmpty()) {
        DependencyManager::get<AddressManager>()->handleLookupString(address);
    }
}

void AddressBarDialog::displayAddressOfflineMessage() {
    OffscreenUi::information("Address offline",
        "That user or place is currently offline.");
}

void AddressBarDialog::displayAddressNotFoundMessage() {
    OffscreenUi::information("Address not found",
        "There is no address information for that user or place.");
}

