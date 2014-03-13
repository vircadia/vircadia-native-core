//
//  ChatWindow.h
//  interface
//
//  Created by Dimitar Dobrev on 3/6/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__ChatWindow__
#define __interface__ChatWindow__

#include <QDateTime>
#include <QTimer>
#include <QWidget>

#include <Application.h>

#ifdef HAVE_QXMPP

#include <QXmppClient.h>
#include <QXmppMessage.h>

#endif

namespace Ui {
class ChatWindow;
}

class ChatWindow : public QWidget {
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
    int numMessagesAfterLastTimeStamp;
    QDateTime lastMessageStamp;

private slots:
    void connected();
    void timeout();
#ifdef HAVE_QXMPP
    void error(QXmppClient::Error error);
    void participantsChanged();
    void messageReceived(const QXmppMessage& message);
#endif
};

#endif /* defined(__interface__ChatWindow__) */
