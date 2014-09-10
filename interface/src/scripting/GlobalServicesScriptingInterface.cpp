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
    connect(&accountManager, &AccountManager::usernameChanged, this,
        &GlobalServicesScriptingInterface::myUsernameChanged);

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