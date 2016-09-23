//
//  Created by Bradley Austin Davis on 2016/09/20
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NetworkUtils.h"
#include <QtNetwork/QNetworkInterface>

QHostAddress getGuessedLocalAddress() {

    QHostAddress localAddress;

    foreach(const QNetworkInterface &networkInterface, QNetworkInterface::allInterfaces()) {
        if (networkInterface.flags() & QNetworkInterface::IsUp
            && networkInterface.flags() & QNetworkInterface::IsRunning
            && networkInterface.flags() & ~QNetworkInterface::IsLoopBack) {
            // we've decided that this is the active NIC
            // enumerate it's addresses to grab the IPv4 address
            foreach(const QNetworkAddressEntry &entry, networkInterface.addressEntries()) {
                // make sure it's an IPv4 address that isn't the loopback
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {

                    // set our localAddress and break out
                    localAddress = entry.ip();
                    break;
                }
            }
        }

        if (!localAddress.isNull()) {
            break;
        }
    }

    // return the looked up local address
    return localAddress;
}


