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
#include <QMediaPlayer>
#include <QSystemTrayIcon>
#include <QTimer>

#include <Application.h>
#include "FramelessDialog.h"

#ifdef HAVE_QXMPP

#include <QXmppClient.h>
#include <QXmppMessage.h>

#endif

namespace Ui {


// Maximum amount the chat can be scrolled up in order to auto scroll.
const int AUTO_SCROLL_THRESHOLD = 20;


class ChatWindow;
}

class ChatWindow : public QWidget {
    Q_OBJECT

public:
    ChatWindow(QWidget* parent);
    ~ChatWindow();

protected:
    bool eventFilter(QObject* sender, QEvent* event);

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void showEvent(QShowEvent* event);
    virtual bool event(QEvent* event);

private:
#ifdef HAVE_QXMPP
    QString getParticipantName(const QString& participant);
#endif
    void startTimerForTimeStamps();
    void addTimeStamp();
    bool isNearBottom();
    void scrollToBottom();

    Ui::ChatWindow* _ui;
    int _numMessagesAfterLastTimeStamp;
    QDateTime _lastMessageStamp;
    bool _mousePressed;
    QPoint _mouseStartPosition;
    QSystemTrayIcon _trayIcon;
    QStringList _mentionSounds;
    QMediaPlayer _effectPlayer;

private slots:
    void connected();
    void timeout();
#ifdef HAVE_QXMPP
    void error(QXmppClient::Error error);
    void participantsChanged();
    void messageReceived(const QXmppMessage& message);
    void notificationClicked();
#endif
};

#endif // hifi_ChatWindow_h
