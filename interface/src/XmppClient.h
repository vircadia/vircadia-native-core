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
public:
    static XmppClient& getInstance();

#ifdef HAVE_QXMPP
    QXmppClient& getXMPPClient() { return _xmppClient; }
    const QXmppMucRoom* getPublicChatRoom() const { return _publicChatRoom; }
#endif
    
signals:
    void joinedPublicChatRoom();

private slots:
    void xmppConnected();
#ifdef HAVE_QXMPP
    void xmppError(QXmppClient::Error error);
#endif

    void connectToServer();
    void disconnectFromServer();

private:
    XmppClient();
    XmppClient(XmppClient const& other); // not implemented
    void operator=(XmppClient const& other); // not implemented

#ifdef HAVE_QXMPP
    QXmppClient _xmppClient;
    QXmppMucManager _xmppMUCManager;
    QXmppMucRoom* _publicChatRoom;
#endif
};

#endif // hifi_XmppClient_h
