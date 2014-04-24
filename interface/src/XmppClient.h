//
//  XmppClient.h
//  interface/src
//
//  Created by Dimitar Dobrev on 10/3/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifdef HAVE_QXMPP

#ifndef hifi_XmppClient_h
#define hifi_XmppClient_h

#include <QObject>
#include <QXmppClient.h>
#include <QXmppMucManager.h>
#include "QXmppArchiveManager.h"

/// Generalized threaded processor for handling received inbound packets. 
class XmppClient : public QObject {
    Q_OBJECT

public:
    static XmppClient& getInstance();

    QXmppClient& getXMPPClient() { return _xmppClient; }
    const QXmppMucRoom* getPublicChatRoom() const { return _publicChatRoom; }
    QXmppArchiveManager* getArchiveManager() const { return _archiveManager; }

private slots:
    void xmppConnected();
    void xmppError(QXmppClient::Error error);

    void connectToServer();
    void disconnectFromServer();

private:
    XmppClient();
    XmppClient(XmppClient const& other); // not implemented
    void operator=(XmppClient const& other); // not implemented

    QXmppClient _xmppClient;
    QXmppMucManager _xmppMUCManager;
    QXmppMucRoom* _publicChatRoom;
    QXmppArchiveManager* _archiveManager;
};

#endif // __interface__XmppClient__

#endif // hifi_XmppClient_h
