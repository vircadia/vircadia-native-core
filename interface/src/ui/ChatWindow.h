//
//  ChatWindow.h
//  interface/src/ui
//
//  Created by Dimitar Dobrev on 3/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ChatWindow_h
#define hifi_ChatWindow_h

#include <QDateTime>
#include <QDockWidget>
#include <QTimer>

#include <Application.h>

#ifdef HAVE_QXMPP

#include <QXmppClient.h>
#include <QXmppMessage.h>

#endif

namespace Ui {
class ChatWindow;
}

class ChatWindow : public QDockWidget {
    Q_OBJECT

public:
    ChatWindow();
    ~ChatWindow();

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void showEvent(QShowEvent* event);

protected:
    bool eventFilter(QObject* sender, QEvent* event);

private:
#ifdef HAVE_QXMPP
    QString getParticipantName(const QString& participant);
#endif
    void startTimerForTimeStamps();
    void addTimeStamp();

    Ui::ChatWindow* ui;
    QWidget* titleBar;
    int numMessagesAfterLastTimeStamp;
    QDateTime lastMessageStamp;

private slots:
    void connected();
    void timeout();
    void togglePinned();
#ifdef HAVE_QXMPP
    void error(QXmppClient::Error error);
    void participantsChanged();
    void messageReceived(const QXmppMessage& message);
#endif
};

#endif // hifi_ChatWindow_h
