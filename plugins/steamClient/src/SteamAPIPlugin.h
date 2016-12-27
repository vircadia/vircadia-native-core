//
//  SteamAPIPlugin.h
//  plugins/steamClient/src
//
//  Created by Clement Brisset on 6/8/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_SteamAPIPlugin_h
#define hifi_SteamAPIPlugin_h

#include <plugins/SteamClientPlugin.h>

class QUrl;

class SteamAPIPlugin : public SteamClientPlugin {
public:
    bool isRunning() override;

    bool init() override;
    void shutdown() override;

    void runCallbacks() override;

    void requestTicket(TicketRequestCallback callback) override;
    void updateLocation(QString status, QUrl locationUrl) override;
    void openInviteOverlay() override;
    void joinLobby(QString lobbyId) override;

    int getSteamVRBuildID() override;
};

#endif // hifi_SteamAPIPlugin_h
