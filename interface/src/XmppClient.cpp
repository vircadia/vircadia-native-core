//
//  XmppClient.cpp
//  interface/src
//
//  Created by Dimitar Dobrev on 10/3/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifdef HAVE_QXMPP

#include <AccountManager.h>

#include "XmppClient.h"

const QString DEFAULT_XMPP_SERVER = "chat.highfidelity.io";
const QString DEFAULT_CHAT_ROOM = "test@public-chat.highfidelity.io";

XmppClient::XmppClient() :
    _xmppClient(),
    _xmppMUCManager()
{
    AccountManager& accountManager = AccountManager::getInstance();
    connect(&accountManager, SIGNAL(accessTokenChanged()), this, SLOT(connectToServer()));
    connect(&accountManager, SIGNAL(logoutComplete()), this, SLOT(disconnectFromServer()));
}

XmppClient& XmppClient::getInstance() {
    static XmppClient sharedInstance;
    return sharedInstance;
}

void XmppClient::xmppConnected() {
    _publicChatRoom = _xmppMUCManager.addRoom(DEFAULT_CHAT_ROOM);
    _publicChatRoom->setNickName(AccountManager::getInstance().getAccountInfo().getUsername());
    _publicChatRoom->join();
    emit joinedPublicChatRoom();
}

void XmppClient::xmppError(QXmppClient::Error error) {
    qDebug() << "Error connnecting to XMPP for user "
        << AccountManager::getInstance().getAccountInfo().getUsername() << ": " << error;
}

void XmppClient::connectToServer() {
    disconnectFromServer();

    if (_xmppClient.addExtension(&_xmppMUCManager)) {
        connect(&_xmppClient, SIGNAL(connected()), this, SLOT(xmppConnected()));
        connect(&_xmppClient, SIGNAL(error(QXmppClient::Error)), this, SLOT(xmppError(QXmppClient::Error)));
    }
    AccountManager& accountManager = AccountManager::getInstance();
    QString user = accountManager.getAccountInfo().getUsername();
    const QString& password = accountManager.getAccountInfo().getXMPPPassword();
    _xmppClient.connectToServer(user + "@" + DEFAULT_XMPP_SERVER, password);
}

void XmppClient::disconnectFromServer() {
    if (_xmppClient.isConnected()) {
        _xmppClient.disconnectFromServer();
    }
}

XmppClient::XmppClient(const XmppClient& other) {
    Q_UNUSED(other);
}

void XmppClient::operator =(XmppClient const& other) {
    Q_UNUSED(other);
}

#endif
