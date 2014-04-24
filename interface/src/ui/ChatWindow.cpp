//
//  ChatWindow.cpp
//  interface/src/ui
//
//  Created by Dimitar Dobrev on 3/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QGridLayout>
#include <QFrame>
#include <QLayoutItem>
#include <QMainWindow>
#include <QPalette>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTimer>
#include <QWidget>

#include "Application.h"
#include "FlowLayout.h"
#include "qtimespan.h"
#include "ui_chatWindow.h"
#include "XmppClient.h"
#include "ChatMessageArea.h"

#include "ChatWindow.h"

const int NUM_MESSAGES_TO_TIME_STAMP = 20;

const QRegularExpression regexLinks("((?:(?:ftp)|(?:https?))://\\S+)");

ChatWindow::ChatWindow() :
    ui(new Ui::ChatWindow),
    numMessagesAfterLastTimeStamp(0),
    _mousePressed(false),
    _mouseStartPosition()
{
    ui->setupUi(this);

    // remove the title bar (see the Qt docs on setTitleBarWidget), but we keep it for undocking
    //
    titleBar = titleBarWidget();
    setTitleBarWidget(new QWidget());

    FlowLayout* flowLayout = new FlowLayout(0, 4, 4);
    ui->usersWidget->setLayout(flowLayout);

    ui->messagesGridLayout->setColumnStretch(0, 1);
    ui->messagesGridLayout->setColumnStretch(1, 3);

    ui->messagePlainTextEdit->installEventFilter(this);

    if (!AccountManager::getInstance().isLoggedIn()) {
        ui->connectingToXMPPLabel->setText(tr("You must be logged in to chat with others."));
    }
    
#ifdef HAVE_QXMPP
    const QXmppClient& xmppClient = XmppClient::getInstance().getXMPPClient();
    if (xmppClient.isConnected()) {
        participantsChanged();
        const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
        connect(publicChatRoom, SIGNAL(participantsChanged()), this, SLOT(participantsChanged()));
        ui->connectingToXMPPLabel->hide();
        startTimerForTimeStamps();
    } else {
        ui->numOnlineLabel->hide();
        ui->closeButton->hide();
        ui->usersWidget->hide();
        ui->messagesScrollArea->hide();
        ui->messagePlainTextEdit->hide();
        connect(&xmppClient, SIGNAL(connected()), this, SLOT(connected()));
    }
    connect(&xmppClient, SIGNAL(messageReceived(QXmppMessage)), this, SLOT(messageReceived(QXmppMessage)));

#endif
}

ChatWindow::~ChatWindow() {
#ifdef HAVE_QXMPP
    const QXmppClient& xmppClient = XmppClient::getInstance().getXMPPClient();
    disconnect(&xmppClient, SIGNAL(connected()), this, SLOT(connected()));
    disconnect(&xmppClient, SIGNAL(messageReceived(QXmppMessage)), this, SLOT(messageReceived(QXmppMessage)));

    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    disconnect(publicChatRoom, SIGNAL(participantsChanged()), this, SLOT(participantsChanged()));
#endif
    delete ui;
}

void ChatWindow::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton && isFloating()) {
        _mousePressed = true;
        _mouseStartPosition = e->pos();
    }
}

void ChatWindow::mouseMoveEvent(QMouseEvent *e) {
    if (_mousePressed) {
        move(mapToParent(e->pos() - _mouseStartPosition));
    }
}

void ChatWindow::mouseReleaseEvent( QMouseEvent *e ) {
    if ( e->button() == Qt::LeftButton ) {
        _mousePressed = false;
    }
}

void ChatWindow::keyPressEvent(QKeyEvent* event) {
    QDockWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_Escape) {
        hide();
    }
}

void ChatWindow::showEvent(QShowEvent* event) {
    QDockWidget::showEvent(event);
    if (!event->spontaneous()) {
        activateWindow();
        ui->messagePlainTextEdit->setFocus();
    }
}

bool ChatWindow::eventFilter(QObject* sender, QEvent* event) {
    if (sender == ui->messagePlainTextEdit) {
        if (event->type() != QEvent::KeyPress) {
            return false;
        }
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) &&
            (keyEvent->modifiers() & Qt::ShiftModifier) == 0) {
            QString messageText = ui->messagePlainTextEdit->document()->toPlainText().trimmed();
            if (!messageText.isEmpty()) {
    #ifdef HAVE_QXMPP
                const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
                QXmppMessage message;
                message.setTo(publicChatRoom->jid());
                message.setType(QXmppMessage::GroupChat);
                message.setBody(messageText);
                XmppClient::getInstance().getXMPPClient().sendPacket(message);
    #endif
                ui->messagePlainTextEdit->document()->clear();
            }
            return true;
        }
    } else {
        if (event->type() != QEvent::MouseButtonRelease) {
            return false;
        }
        QString user = sender->property("user").toString();
        Menu::getInstance()->goToUser(user);
    }
    return false;
}

#ifdef HAVE_QXMPP
QString ChatWindow::getParticipantName(const QString& participant) {
    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    return participant.right(participant.count() - 1 - publicChatRoom->jid().count());
}
#endif

void ChatWindow::addTimeStamp() {
    QTimeSpan timePassed = QDateTime::currentDateTime() - lastMessageStamp;
    int times[] = { timePassed.daysPart(), timePassed.hoursPart(), timePassed.minutesPart() };
    QString strings[] = { tr("%n day(s)", 0, times[0]), tr("%n hour(s)", 0, times[1]), tr("%n minute(s)", 0, times[2]) };
    QString timeString = "";
    for (int i = 0; i < 3; i++) {
        if (times[i] > 0) {
            timeString += strings[i] + " ";
        }
    }
    timeString.chop(1);
    if (!timeString.isEmpty()) {
        QLabel* timeLabel = new QLabel(timeString);
        timeLabel->setStyleSheet("color: palette(shadow);"
                                 "background-color: palette(highlight);"
                                 "padding: 4px;");
        timeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        timeLabel->setAlignment(Qt::AlignHCenter);

        bool atBottom = isAtBottom();

        ui->messagesGridLayout->addWidget(timeLabel, ui->messagesGridLayout->rowCount(), 0, 1, 2);
        ui->messagesGridLayout->parentWidget()->updateGeometry();

        Application::processEvents();
        numMessagesAfterLastTimeStamp = 0;

        if (atBottom) {
            scrollToBottom();
        }
    }
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
    ui->closeButton->show();
    ui->usersWidget->show();
    ui->messagesScrollArea->show();
    ui->messagePlainTextEdit->show();
    ui->messagePlainTextEdit->setFocus();
#ifdef HAVE_QXMPP
    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    connect(publicChatRoom, SIGNAL(participantsChanged()), this, SLOT(participantsChanged()));


    // set limits
    QDateTime m_startDate = QDateTime::currentDateTime().addDays(-2);
    QDateTime m_endDate = QDateTime::currentDateTime();

    QXmppResultSetQuery rsmQuery;
    rsmQuery.setMax(100);

    QXmppArchiveManager* archiveManager = XmppClient::getInstance().getArchiveManager();
    archiveManager->listCollections("", m_startDate, m_endDate, rsmQuery);

#endif
    startTimerForTimeStamps();
}

void ChatWindow::timeout() {
    if (numMessagesAfterLastTimeStamp >= NUM_MESSAGES_TO_TIME_STAMP) {
        addTimeStamp();
    }
}

#ifdef HAVE_QXMPP

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
        QString participantName = getParticipantName(participant);
        QLabel* userLabel = new QLabel();
        userLabel->setText(participantName);
        userLabel->setStyleSheet("background-color: palette(light);"
                                 "border-radius: 5px;"
                                 "color: #267077;"
                                 "padding-top: 3px;"
                                 "padding-right: 2px;"
                                 "padding-bottom: 2px;"
                                 "padding-left: 2px;"
                                 "border: 1px solid palette(shadow);"
                                 "font-weight: bold");
        userLabel->setProperty("user", participantName);
        userLabel->setCursor(Qt::PointingHandCursor);
        userLabel->installEventFilter(this);
        ui->usersWidget->layout()->addWidget(userLabel);
    }
}

void ChatWindow::messageReceived(const QXmppMessage& message) {
    if (message.type() != QXmppMessage::GroupChat) {
        return;
    }

    QLabel* userLabel = new QLabel(getParticipantName(message.from()));
    userLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    userLabel->setStyleSheet("padding: 2px; font-weight: bold");
    userLabel->setAlignment(Qt::AlignTop | Qt::AlignRight);

    ChatMessageArea* messageArea = new ChatMessageArea();

    messageArea->setOpenLinks(true);
    messageArea->setOpenExternalLinks(true);
    messageArea->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    messageArea->setTextInteractionFlags(Qt::TextBrowserInteraction);
    messageArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    messageArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    messageArea->setReadOnly(true);

    messageArea->setStyleSheet("padding-bottom: 2px;"
                               "padding-left: 2px;"
                               "padding-top: 2px;"
                               "padding-right: 20px;"
                               "background-color: rgba(0, 0, 0, 0%);"
                               "border: 0;");

    bool fromSelf = getParticipantName(message.from()) == AccountManager::getInstance().getUsername();
    if (fromSelf) {
        userLabel->setStyleSheet(userLabel->styleSheet() + "; background-color: #e1e8ea");
        messageArea->setStyleSheet(messageArea->styleSheet() + "; background-color: #e1e8ea");
    }

    messageArea->setHtml(message.body().replace(regexLinks, "<a href=\"\\1\">\\1</a>"));

    bool atBottom = isAtBottom();
    ui->messagesGridLayout->addWidget(userLabel, ui->messagesGridLayout->rowCount(), 0);
    ui->messagesGridLayout->addWidget(messageArea, ui->messagesGridLayout->rowCount() - 1, 1);

    ui->messagesGridLayout->parentWidget()->updateGeometry();
    Application::processEvents();

    if (atBottom || fromSelf) {
        scrollToBottom();
    }

    ++numMessagesAfterLastTimeStamp;
    if (message.stamp().isValid()) {
        lastMessageStamp = message.stamp().toLocalTime();
    } else {
        lastMessageStamp = QDateTime::currentDateTime();
    }
}

#endif

bool ChatWindow::isAtBottom() {
    QScrollBar* verticalScrollBar = ui->messagesScrollArea->verticalScrollBar();
    return verticalScrollBar->sliderPosition() == verticalScrollBar->maximum();
}

// Scroll chat message area to bottom.
void ChatWindow::scrollToBottom() {
    QScrollBar* verticalScrollBar = ui->messagesScrollArea->verticalScrollBar();
    verticalScrollBar->setSliderPosition(verticalScrollBar->maximum());
}

void ChatWindow::togglePinned() {
    QMainWindow* mainWindow = Application::getInstance()->getWindow();
    mainWindow->removeDockWidget(this);
    if (ui->togglePinnedButton->isChecked()) {
        mainWindow->addDockWidget(ui->togglePinnedButton->isChecked() ? Qt::RightDockWidgetArea : Qt::NoDockWidgetArea, this);
    }
    if (!this->toggleViewAction()->isChecked()) {
        this->toggleViewAction()->trigger();
    }
    this->setFloating(!ui->togglePinnedButton->isChecked());
    setTitleBarWidget(ui->togglePinnedButton->isChecked()?new QWidget():titleBar);
}
