//
//  Wallet.cpp
//  interface/src/commerce
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <quuid.h>
#include "CommerceLogging.h"
#include "Ledger.h"
#include "Wallet.h"

bool Wallet::createIfNeeded() {
    // FIXME: persist in file between sessions.
    if (_publicKeys.count() > 0) return false;
    qCInfo(commerce) << "Creating wallet.";
    return generateKeyPair();
}

bool Wallet::generateKeyPair() {
    // FIXME: need private key, too, and persist in file.
    qCInfo(commerce) << "Generating keypair.";
    QString key = QUuid::createUuid().toString();
    _publicKeys.push_back(key);
    auto ledger = DependencyManager::get<Ledger>();
    return ledger->receiveAt(key);
}
QStringList Wallet::listPublicKeys() {
    qCInfo(commerce) << "Enumerating public keys.";
    createIfNeeded();
    return _publicKeys;
}

QString Wallet::signWithKey(const QString& text, const QString& key) {
    qCInfo(commerce) << "Signing text.";
    return "fixme signed";
}