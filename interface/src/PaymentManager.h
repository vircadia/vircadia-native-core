//
//  PaymentManager.h
//  interface/src
//
//  Created by Stephen Birarda on 2014-07-30.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PaymentManager_h
#define hifi_PaymentManager_h

#include <QtCore/QObject>

class PaymentManager : public QObject {
    Q_OBJECT
public:
    static PaymentManager& getInstance();
public slots:
    void sendSignedPayment(qint64 satoshiAmount, const QUuid& nodeUUID, const QUuid& destinationWalletUUID);
};

#endif // hifi_PaymentManager_h