//
//  SteamClient.h
//  steamworks-wrapper/src/steamworks-wrapper
//
//  Created by Clement Brisset on 6/8/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_SteamClient_h
#define hifi_SteamClient_h

#include <functional>

#include <QtCore/QByteArray>

using Ticket = QByteArray;
using TicketRequestCallback = std::function<void(Ticket)>;

class SteamClient {
public:
    static bool isRunning();

    static bool init();
    static void shutdown();

    static void runCallbacks();

    static void requestTicket(TicketRequestCallback callback);

};

#endif // hifi_SteamClient_h