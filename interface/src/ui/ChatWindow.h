//
//  ChatWindow.h
//  interface
//
//  Created by Dimitar Dobrev on 3/6/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__ChatWindow__
#define __interface__ChatWindow__

#include <QDialog>
#include <QDateTime>
#include <QTimer>

#include <Application.h>

#ifdef HAVE_QXMPP

#include <QXmppClient.h>
#include <QXmppMessage.h>

#endif

namespace Ui {
class ChatWindow;
}

class ChatWindow : public QDialog {
    Q_OBJECT

public:
    ChatWindow();
    ~ChatWindow();

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
