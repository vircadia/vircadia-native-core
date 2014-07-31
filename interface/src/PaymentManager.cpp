//
//  PaymentManager.cpp
//  interface/src
//
//  Created by Stephen Birarda on 2014-07-30.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDebug>
#include <QtCore/QUuid>

#include "PaymentManager.h"

PaymentManager& PaymentManager::getInstance() {
    static PaymentManager sharedInstance;
    return sharedInstance;
}

void PaymentManager::sendSignedPayment(qint64 satoshiAmount, const QUuid& nodeUUID, const QUuid& destinationWalletUUID) {
    qDebug() << "Paying" << satoshiAmount << "satoshis to" << destinationWalletUUID << "via" << nodeUUID;
    
    
}