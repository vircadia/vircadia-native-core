//
//  GlobalServicesScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Thijs Wenker on 9/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AccountManager.h"
#include "XmppClient.h"

#include "GlobalServicesScriptingInterface.h"

GlobalServicesScriptingInterface::GlobalServicesScriptingInterface() {
    AccountManager& accountManager = AccountManager::getInstance();
    connect(&accountManager, &AccountManager::usernameChanged, this, &GlobalServicesScriptingInterface::myUsernameChanged);
    connect(&accountManager, &AccountManager::logoutComplete, this, &GlobalServicesScriptingInterface::loggedOut);
#ifdef HAVE_QXMPP
    const XmppClient& xmppClient = XmppClient::getInstance();
    connect(&xmppClient, &XmppClient::joinedPublicChatRoom, this, &GlobalServicesScriptingInterface::connected);
    connect(&xmppClient, &XmppClient::joinedPublicChatRoom, this, &GlobalServicesScriptingInterface::onConnected);
    const QXmppClient& qxmppClient = XmppClient::getInstance().getXMPPClient();
    connect(&qxmppClient, &QXmppClient::messageReceived, this, &GlobalServicesScriptingInterface::messageReceived);
#endif // HAVE_QXMPP
}

GlobalServicesScriptingInterface::~GlobalServicesScriptingInterface() {
    AccountManager& accountManager = AccountManager::getInstance();
    disconnect(&accountManager, &AccountManager::usernameChanged, this, &GlobalServicesScriptingInterface::myUsernameChanged);
    disconnect(&accountManager, &AccountManager::logoutComplete, this, &GlobalServicesScriptingInterface::loggedOut);
#ifdef HAVE_QXMPP
    const XmppClient& xmppClient = XmppClient::getInstance();
    disconnect(&xmppClient, &XmppClient::joinedPublicChatRoom, this, &GlobalServicesScriptingInterface::connected);
    disconnect(&xmppClient, &XmppClient::joinedPublicChatRoom, this, &GlobalServicesScriptingInterface::onConnected);
    const QXmppClient& qxmppClient = XmppClient::getInstance().getXMPPClient();
    disconnect(&qxmppClient, &QXmppClient::messageReceived, this, &GlobalServicesScriptingInterface::messageReceived);
    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    disconnect(publicChatRoom, &QXmppMucRoom::participantsChanged, this, &GlobalServicesScriptingInterface::participantsChanged);
#endif // HAVE_QXMPP
}

void GlobalServicesScriptingInterface::onConnected() {
#ifdef HAVE_QXMPP
    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    connect(publicChatRoom, &QXmppMucRoom::participantsChanged, this, &GlobalServicesScriptingInterface::participantsChanged, Qt::UniqueConnection);
#endif // HAVE_QXMPP
}

void GlobalServicesScriptingInterface::participantsChanged() {
#ifdef HAVE_QXMPP
    emit GlobalServicesScriptingInterface::onlineUsersChanged(this->getOnlineUsers());
#endif // HAVE_QXMPP
}

GlobalServicesScriptingInterface* GlobalServicesScriptingInterface::getInstance() {
    static GlobalServicesScriptingInterface sharedInstance;
    return &sharedInstance;
}

bool GlobalServicesScriptingInterface::isConnected() {
#ifdef HAVE_QXMPP
    return XmppClient::getInstance().getXMPPClient().isConnected();
#else
    return false;
#endif // HAVE_QXMPP
}

QScriptValue GlobalServicesScriptingInterface::chat(const QString& message) {
#ifdef HAVE_QXMPP
    if (XmppClient::getInstance().getXMPPClient().isConnected()) {
        const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
        QXmppMessage messageObject;
        messageObject.setTo(publicChatRoom->jid());
        messageObject.setType(QXmppMessage::GroupChat);
        messageObject.setBody(message);
        return XmppClient::getInstance().getXMPPClient().sendPacket(messageObject);
    }
#endif // HAVE_QXMPP
    return false;
}

QString GlobalServicesScriptingInterface::getMyUsername() {
    return AccountManager::getInstance().getAccountInfo().getUsername();
}

QStringList GlobalServicesScriptingInterface::getOnlineUsers() {
#ifdef HAVE_QXMPP
    if (XmppClient::getInstance().getXMPPClient().isConnected()) {
        QStringList usernames;
        const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
        foreach(const QString& participant, XmppClient::getInstance().getPublicChatRoom()->participants()) {
            usernames.append(participant.right(participant.count() - 1 - publicChatRoom->jid().count()));
        }
        return usernames;
    }
#endif // HAVE_QXMPP
    return QStringList();
}

void GlobalServicesScriptingInterface::loggedOut() {
    emit GlobalServicesScriptingInterface::disconnected(QString("logout"));
}

void GlobalServicesScriptingInterface::messageReceived(const QXmppMessage& message) {
    if (message.type() != QXmppMessage::GroupChat) {
        return;
    }
    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    emit GlobalServicesScriptingInterface::incomingMessage(message.from().right(message.from().count() - 1 - publicChatRoom->jid().count()), message.body());
}
