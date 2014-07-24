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

#ifndef hifi_XmppClient_h
#define hifi_XmppClient_h

#include <QObject>

#ifdef HAVE_QXMPP
#include <QXmppClient.h>
#include <QXmppMucManager.h>
#endif

/// Generalized threaded processor for handling received inbound packets. 
class XmppClient : public QObject {
    Q_OBJECT
#ifdef HAVE_QXMPP
public:
    static XmppClient& getInstance();

    QXmppClient& getXMPPClient() { return _xmppClient; }
    const QXmppMucRoom* getPublicChatRoom() const { return _publicChatRoom; }

signals:
    void joinedPublicChatRoom();

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
#endif
};

#endif // hifi_XmppClient_h
