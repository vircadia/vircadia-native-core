//
//  XmppClient.h
//  interface
//
//  Created by Dimitar Dobrev on 10/3/14
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__XmppClient__
#define __interface__XmppClient__

#include <QObject>
#include <QXmppClient.h>
#include <QXmppMucManager.h>

/// Generalized threaded processor for handling received inbound packets. 
class XmppClient : public QObject {
    Q_OBJECT

public:
    static XmppClient& getInstance();

    QXmppClient& getXMPPClient() { return _xmppClient; }
    const QXmppMucRoom* getPublicChatRoom() const { return _publicChatRoom; }

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
};

#endif // __interface__XmppClient__
