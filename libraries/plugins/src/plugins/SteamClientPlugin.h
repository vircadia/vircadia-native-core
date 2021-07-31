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

/*@jsdoc
 * The <code>Steam</code> API provides facilities for working with the Steam version of Interface.
 *
 * @namespace Steam
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {boolean} running - <code>true</code> if Interface is running under Steam, <code>false</code> if it isn't. 
 *     <em>Read-only.</em>
 */

class SteamScriptingInterface : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool running READ isRunning)

public:
    SteamScriptingInterface(QObject* parent, SteamClientPlugin* plugin) : QObject(parent), _plugin(plugin) {}

public slots:

    /*@jsdoc
     * Gets whether Interface is running under Steam.
     * @function Steam.isRunning
     * @returns {boolean} <code>true</code> if Interface is running under Steam, <code>false</code> if it isn't. 
     */
    bool isRunning() const { return _plugin && _plugin->isRunning(); }

    /*@jsdoc
     * Opens Steam's "Choose Friends to invite" dialog if Interface is running under Steam.
     * @function Steam.openInviteOverlay
     * @example <caption>Invite Steam friends to join you in Vircadia.</caption>
     * if (Steam.running) {
     *     print("Invite Steam friends to joint you...");
     *     Steam.openInviteOverlay();
     * } else {
     *     print("Interface isn't running under Steam.");
     * }
     */
    void openInviteOverlay() const { if (_plugin) { _plugin->openInviteOverlay(); } }

private:
    SteamClientPlugin* _plugin;
};

#endif /* hifi_SteamClientPlugin_h */
