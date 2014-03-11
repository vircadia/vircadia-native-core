//
//  ChatWindow.cpp
//  interface
//
//  Created by Dimitar Dobrev on 3/6/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <QFormLayout>
#include <QFrame>
#include <QLayoutItem>
#include <QPalette>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTextDocument>
#include <QTimer>

#include "Application.h"
#include "FlowLayout.h"
#include "qtimespan.h"
#include "ui_chatWindow.h"
#include "XmppClient.h"

#include "ChatWindow.h"

const int NUM_MESSAGES_TO_TIME_STAMP = 20;

const QRegularExpression regexLinks("((?:(?:ftp)|(?:https?))://\\S+)");

ChatWindow::ChatWindow() :
    QDialog(Application::getInstance()->getGLWidget(), Qt::Tool),
    ui(new Ui::ChatWindow),
    numMessagesAfterLastTimeStamp(0)
{
    ui->setupUi(this);

    FlowLayout* flowLayout = new FlowLayout(0, 4, 4);
    ui->usersWidget->setLayout(flowLayout);

    ui->messagePlainTextEdit->installEventFilter(this);

    setAttribute(Qt::WA_DeleteOnClose);

    const QXmppClient& xmppClient = XmppClient::getInstance().getXMPPClient();
    if (xmppClient.isConnected()) {
        participantsChanged();
        const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
        connect(publicChatRoom, SIGNAL(participantsChanged()), this, SLOT(participantsChanged()));
        ui->connectingToXMPPLabel->hide();
        startTimerForTimeStamps();
    } else {
        ui->numOnlineLabel->hide();
        ui->usersWidget->hide();
        ui->messagesScrollArea->hide();
        ui->messagePlainTextEdit->hide();
        connect(&xmppClient, SIGNAL(connected()), this, SLOT(connected()));
    }
    connect(&xmppClient, SIGNAL(messageReceived(QXmppMessage)), this, SLOT(messageReceived(QXmppMessage)));
}

ChatWindow::~ChatWindow() {
    const QXmppClient& xmppClient = XmppClient::getInstance().getXMPPClient();
    disconnect(&xmppClient, SIGNAL(connected()), this, SLOT(connected()));
    disconnect(&xmppClient, SIGNAL(messageReceived(QXmppMessage)), this, SLOT(messageReceived(QXmppMessage)));

    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    disconnect(publicChatRoom, SIGNAL(participantsChanged()), this, SLOT(participantsChanged()));

    delete ui;
}

bool ChatWindow::eventFilter(QObject* sender, QEvent* event) {
    Q_UNUSED(sender);

    if (event->type() != QEvent::KeyPress) {
        return false;
    }
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) &&
        (keyEvent->modifiers() & Qt::ShiftModifier) == 0) {
        QString messageText = ui->messagePlainTextEdit->document()->toPlainText().trimmed();
        if (!messageText.isEmpty()) {
            const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
            QXmppMessage message;
            message.setTo(publicChatRoom->jid());
            message.setType(QXmppMessage::GroupChat);
            message.setBody(messageText);
            XmppClient::getInstance().getXMPPClient().sendPacket(message);
            ui->messagePlainTextEdit->document()->clear();
        }
        return true;
    }
    return false;
}

QString ChatWindow::getParticipantName(const QString& participant) {
    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    return participant.right(participant.count() - 1 - publicChatRoom->jid().count());
}

void ChatWindow::addTimeStamp() {
    QTimeSpan timePassed = QDateTime::currentDateTime() - lastMessageStamp;
    int times[] = { timePassed.daysPart(), timePassed.hoursPart(), timePassed.minutesPart() };
    QString strings[] = { tr("day", 0, times[0]), tr("hour", 0, times[1]), tr("minute", 0, times[2]) };
    QString timeString = "";
    for (int i = 0; i < 3; i++) {
        if (times[i] > 0) {
            timeString += strings[i] + " ";
        }
    }
    timeString.chop(1);
    QLabel* timeLabel = new QLabel(timeString);
    timeLabel->setStyleSheet("color: palette(shadow);"
                             "background-color: palette(highlight);"
                             "padding: 4px;");
    timeLabel->setAlignment(Qt::AlignHCenter);
    ui->messagesFormLayout->addRow(timeLabel);
    numMessagesAfterLastTimeStamp = 0;
}

void ChatWindow::startTimerForTimeStamps() {
    QTimer* timer = new QTimer(this);
    timer->setInterval(10 * 60 * 1000);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
    timer->start();
}

void ChatWindow::connected() {
    ui->connectingToXMPPLabel->hide();
    ui->numOnlineLabel->show();
    ui->usersWidget->show();
    ui->messagesScrollArea->show();
    ui->messagePlainTextEdit->show();

    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    connect(publicChatRoom, SIGNAL(participantsChanged()), this, SLOT(participantsChanged()));

    startTimerForTimeStamps();
}

void ChatWindow::timeout() {
    if (numMessagesAfterLastTimeStamp >= NUM_MESSAGES_TO_TIME_STAMP) {
        addTimeStamp();
    }
}

void ChatWindow::error(QXmppClient::Error error) {
    ui->connectingToXMPPLabel->setText(QString::number(error));
}

void ChatWindow::participantsChanged() {
    QStringList participants = XmppClient::getInstance().getPublicChatRoom()->participants();
    ui->numOnlineLabel->setText(tr("%1 online now:").arg(participants.count()));

    while (QLayoutItem* item = ui->usersWidget->layout()->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    foreach (const QString& participant, participants) {
        QLabel* userLabel = new QLabel(getParticipantName(participant));
        userLabel->setStyleSheet("background-color: palette(light);"
                                 "border-radius: 5px;"
                                 "color: #267077;"
                                 "padding: 2px;"
                                 "border: 1px solid palette(shadow);"
                                 "font-weight: bold");
        ui->usersWidget->layout()->addWidget(userLabel);
    }
}

void ChatWindow::messageReceived(const QXmppMessage& message) {
    QLabel* userLabel = new QLabel(getParticipantName(message.from()));
    QFont font = userLabel->font();
    font.setBold(true);
    userLabel->setFont(font);
    userLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    userLabel->setStyleSheet("padding: 2px;");
    userLabel->setAlignment(Qt::AlignTop);

    QLabel* messageLabel = new QLabel(message.body().replace(regexLinks, "<a href=\"\\1\">\\1</a>"));
    messageLabel->setWordWrap(true);
    messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    messageLabel->setOpenExternalLinks(true);
    messageLabel->setStyleSheet("padding: 2px;");
    messageLabel->setAlignment(Qt::AlignTop);

    ui->messagesFormLayout->addRow(userLabel, messageLabel);
    ui->messagesFormLayout->parentWidget()->updateGeometry();
    Application::processEvents();
    QScrollBar* verticalScrollBar = ui->messagesScrollArea->verticalScrollBar();
    verticalScrollBar->setSliderPosition(verticalScrollBar->maximum());
    messageLabel->updateGeometry();

    ++numMessagesAfterLastTimeStamp;
    if (message.stamp().isValid()) {
        lastMessageStamp = message.stamp().toLocalTime();
    } else {
        lastMessageStamp = QDateTime::currentDateTime();
    }
}
