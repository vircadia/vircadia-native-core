//
//  Ledger.h
//  interface/src/commerce
//
// Bottlenecks all interaction with the blockchain or other ledger system.
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Ledger_h
#define hifi_Ledger_h

#include <DependencyManager.h>
#include <qjsonobject.h>
#include <qjsonarray.h>

class Ledger : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    void buy(const QString& hfc_key, int cost, const QString& asset_id, const QString& inventory_key, const QString& buyerUsername = "");
    bool receiveAt(const QString& hfc_key);
    void balance(const QStringList& keys);
    void inventory(const QStringList& keys);

signals:
    void buyResult(const QString& failureReason);
    void receiveAtResult(const QString& failureReason);
    void balanceResult(int balance, const QString& failureReason);
    void inventoryResult(QJsonObject inventory, const QString& failureReason);

private:
    // These in-memory caches is temporary, until we start sending things to the server.
    int _balance{ -1 };
    QJsonArray _inventory{};
    int initializedBalance() { if (_balance < 0) _balance = 100; return _balance; }
};

#endif // hifi_Ledger_h
