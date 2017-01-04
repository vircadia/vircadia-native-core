//
//  SteamClientPlugin.h
//  libraries/plugins/src/plugins
//
//  Created by Clement Brisset on 12/14/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_SteamClientPlugin_h
#define hifi_SteamClientPlugin_h

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QByteArray>

using Ticket = QByteArray;
using TicketRequestCallback = std::function<void(Ticket)>;

class SteamClientPlugin {
public:
    virtual ~SteamClientPlugin() {};

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    virtual bool isRunning() = 0;

    virtual void runCallbacks() = 0;

    virtual void requestTicket(TicketRequestCallback callback) = 0;
    virtual void updateLocation(QString status, QUrl locationUrl) = 0;
    virtual void openInviteOverlay() = 0;
    virtual void joinLobby(QString lobbyId) = 0;

    virtual int getSteamVRBuildID() = 0;
};

class SteamScriptingInterface : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isRunning READ isRunning)

public:
    SteamScriptingInterface(QObject* parent, SteamClientPlugin* plugin) : QObject(parent) {}

    public slots:
    bool isRunning() const { return _plugin->isRunning(); }
    void openInviteOverlay() const { _plugin->openInviteOverlay(); }

private:
    SteamClientPlugin* _plugin;
};

#endif /* hifi_SteamClientPlugin_h */
