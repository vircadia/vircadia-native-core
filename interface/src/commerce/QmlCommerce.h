//
//  Commerce.h
//  interface/src/commerce
//
//  Guard for safe use of Commerce (Wallet, Ledger) by authorized QML.
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_QmlCommerce_h
#define hifi_QmlCommerce_h

#include <OffscreenQmlDialog.h>

class QmlCommerce : public OffscreenQmlDialog {
    Q_OBJECT
    HIFI_QML_DECL

protected:
    Q_INVOKABLE bool buy(const QString& assetId, int cost, const QString& buyerUsername = "");
};

#endif // hifi_QmlCommerce_h
