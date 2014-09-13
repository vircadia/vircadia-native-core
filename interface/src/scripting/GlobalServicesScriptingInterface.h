//
//  GlobalServicesScriptingInterface.h
//  interface/src/scripting
//
//  Created by Thijs Wenker on 9/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GlobalServicesScriptingInterface_h
#define hifi_GlobalServicesScriptingInterface_h

#include <QObject>
#include <QScriptContext>
#include <QScriptEngine>
#include <QScriptValue>
#include <QString>
#include <QStringList>

#ifdef HAVE_QXMPP

#include <QXmppClient.h>
#include <QXmppMessage.h>

#endif // HAVE_QXMPP

class GlobalServicesScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected)
    Q_PROPERTY(QString myUsername READ getMyUsername)
    Q_PROPERTY(QStringList onlineUsers READ getOnlineUsers)
    GlobalServicesScriptingInterface();
    ~GlobalServicesScriptingInterface();
public:
    static GlobalServicesScriptingInterface* getInstance();

    bool isConnected();
    QString getMyUsername();
    QStringList getOnlineUsers();

public slots:
    QScriptValue chat(const QString& message);

private slots:
    void loggedOut();
    void onConnected();
    void participantsChanged();
#ifdef HAVE_QXMPP
    void messageReceived(const QXmppMessage& message);
#endif // HAVE_QXMPP

signals:
    void connected();
    void disconnected(const QString& reason);
    void incomingMessage(const QString& username, const QString& message);
    void onlineUsersChanged(const QStringList& usernames);
    void myUsernameChanged(const QString& username);
};

#endif // hifi_GlobalServicesScriptingInterface_h
