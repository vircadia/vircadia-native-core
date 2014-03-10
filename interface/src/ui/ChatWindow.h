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

#include <QXmppClient.h>
#include <QXmppMucManager.h>

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
    QString getParticipantName(const QString& participant);
    void addTimeStamp();

    Ui::ChatWindow* ui;
    QXmppClient _xmppClient;
    QXmppMucManager _xmppMUCManager;
    QXmppMucRoom* _chatRoom;
    int numMessagesAfterLastTimeStamp;
    QDateTime lastMessageStamp;

private slots:
    void connected();
    void timeout();
    void error(QXmppClient::Error error);
    void participantsChanged();
    void messageReceived(const QXmppMessage& message);
};

#endif /* defined(__interface__ChatWindow__) */
