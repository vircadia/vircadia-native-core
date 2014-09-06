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
#include <QPalette>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTimer>

#include "Application.h"
#include "ChatMessageArea.h"
#include "FlowLayout.h"
#include "qtimespan.h"
#include "UIUtil.h"
#include "XmppClient.h"

#include "ui_chatWindow.h"
#include "ChatWindow.h"



const int NUM_MESSAGES_TO_TIME_STAMP = 20;

const QRegularExpression regexLinks("((?:(?:ftp)|(?:https?)|(?:hifi))://\\S+)");
const QRegularExpression regexHifiLinks("([#@]\\S+)");
const QString mentionSoundsPath("/mention-sounds/");
const QString mentionRegex("@(\\b%1\\b)");

ChatWindow::ChatWindow(QWidget* parent) :
    QWidget(parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint |
            Qt::WindowCloseButtonHint),
    ui(new Ui::ChatWindow),
    numMessagesAfterLastTimeStamp(0),
    _mousePressed(false),
    _mouseStartPosition(),
    _trayIcon(parent),
    _effectPlayer()
{
    setAttribute(Qt::WA_DeleteOnClose, false);

    ui->setupUi(this);

    FlowLayout* flowLayout = new FlowLayout(0, 4, 4);
    ui->usersWidget->setLayout(flowLayout);

    ui->messagePlainTextEdit->installEventFilter(this);
    ui->messagePlainTextEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    QTextCursor cursor(ui->messagePlainTextEdit->textCursor());

    cursor.movePosition(QTextCursor::Start);

    QTextBlockFormat format = cursor.blockFormat();
    format.setLineHeight(130, QTextBlockFormat::ProportionalHeight);

    cursor.setBlockFormat(format);

    ui->messagePlainTextEdit->setTextCursor(cursor);

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
        ui->usersArea->hide();
        ui->messagesScrollArea->hide();
        ui->messagePlainTextEdit->hide();
        connect(&XmppClient::getInstance(), SIGNAL(joinedPublicChatRoom()), this, SLOT(connected()));
    }
    connect(&xmppClient, SIGNAL(messageReceived(QXmppMessage)), this, SLOT(messageReceived(QXmppMessage)));
    connect(&_trayIcon, SIGNAL(messageClicked()), this, SLOT(notificationClicked()));
#endif // HAVE_QXMPP

    QDir mentionSoundsDir(Application::resourcesPath() + mentionSoundsPath);
    _mentionSounds = mentionSoundsDir.entryList(QDir::Files);
    _trayIcon.setIcon(QIcon( Application::resourcesPath() + "/images/hifi-logo.svg"));
}

ChatWindow::~ChatWindow() {
#ifdef HAVE_QXMPP
    const QXmppClient& xmppClient = XmppClient::getInstance().getXMPPClient();
    disconnect(&xmppClient, SIGNAL(joinedPublicChatRoom()), this, SLOT(connected()));
    disconnect(&xmppClient, SIGNAL(messageReceived(QXmppMessage)), this, SLOT(messageReceived(QXmppMessage)));

    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    disconnect(publicChatRoom, SIGNAL(participantsChanged()), this, SLOT(participantsChanged()));
#endif // HAVE_QXMPP
    delete ui;
}

void ChatWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        Application::getInstance()->getWindow()->activateWindow();
        hide();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void ChatWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    if (!event->spontaneous()) {
        ui->messagePlainTextEdit->setFocus();
    }
    const QRect parentGeometry = parentWidget()->geometry();
    int titleBarHeight = UIUtil::getWindowTitleBarHeight(this);
    int menuBarHeight = Menu::getInstance()->geometry().height();
    int topMargin = titleBarHeight + menuBarHeight;

    setGeometry(parentGeometry.topRight().x() - size().width() + 1, parentGeometry.topRight().y() + topMargin,
                size().width(), parentWidget()->height() - topMargin);

    Application::processEvents();

    scrollToBottom();

#ifdef HAVE_QXMPP
    const QXmppClient& xmppClient = XmppClient::getInstance().getXMPPClient();
    if (xmppClient.isConnected()) {
        participantsChanged();
    }
#endif // HAVE_QXMPP
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
#endif // HAVE_QXMPP
                QTextCursor cursor = ui->messagePlainTextEdit->textCursor();
                cursor.select(QTextCursor::Document);
                cursor.removeSelectedText();
            }
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        QVariant userVar = sender->property("user");
        if (userVar.isValid()) {
            Menu::getInstance()->goToUser("@" + userVar.toString());
            return true;
        }
    }
    return QWidget::eventFilter(sender, event);
}

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
        timeLabel->setStyleSheet("color: #333333;"
                                 "background-color: white;"
                                 "font-size: 14px;"
                                 "padding: 4px;");
        timeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        timeLabel->setAlignment(Qt::AlignLeft);

        bool atBottom = isNearBottom();

        ui->messagesVBoxLayout->addWidget(timeLabel);
        ui->messagesVBoxLayout->parentWidget()->updateGeometry();

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
    ui->usersArea->show();
    ui->messagesScrollArea->show();
    ui->messagePlainTextEdit->show();
    ui->messagePlainTextEdit->setFocus();
#ifdef HAVE_QXMPP
    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    connect(publicChatRoom, SIGNAL(participantsChanged()), this, SLOT(participantsChanged()));
#endif // HAVE_QXMPP
    startTimerForTimeStamps();
}

void ChatWindow::timeout() {
    if (numMessagesAfterLastTimeStamp >= NUM_MESSAGES_TO_TIME_STAMP) {
        addTimeStamp();
    }
}

#ifdef HAVE_QXMPP
void ChatWindow::notificationClicked() {
    if (parentWidget()->isMinimized()) {
        parentWidget()->showNormal();
    }
    if (isHidden()) {
        show();
    }

    // find last mention
    int messageCount = ui->messagesVBoxLayout->count();
    for (unsigned int i = messageCount; i > 0; i--) {
        ChatMessageArea* area = (ChatMessageArea*)ui->messagesVBoxLayout->itemAt(i - 1)->widget();
        QRegularExpression usernameMention(mentionRegex.arg(AccountManager::getInstance().getAccountInfo().getUsername()));
        if (area->toPlainText().contains(usernameMention)) {
            int top = area->geometry().top();
            int height = area->geometry().height();

            QScrollBar* verticalScrollBar = ui->messagesScrollArea->verticalScrollBar();
            verticalScrollBar->setSliderPosition(top - verticalScrollBar->size().height() + height);
            return;
        }
    }
    Application::processEvents();

    scrollToBottom();
}

QString ChatWindow::getParticipantName(const QString& participant) {
    const QXmppMucRoom* publicChatRoom = XmppClient::getInstance().getPublicChatRoom();
    return participant.right(participant.count() - 1 - publicChatRoom->jid().count());
}

void ChatWindow::error(QXmppClient::Error error) {
    ui->connectingToXMPPLabel->setText(QString::number(error));
}

void ChatWindow::participantsChanged() {
    bool atBottom = isNearBottom();

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
                                 "font-size: 14px;"
                                 "font-weight: bold");
        userLabel->setProperty("user", participantName);
        userLabel->setCursor(Qt::PointingHandCursor);
        userLabel->installEventFilter(this);
        ui->usersWidget->layout()->addWidget(userLabel);
    }
    Application::processEvents();

    if (atBottom) {
        scrollToBottom();
    }
}

void ChatWindow::messageReceived(const QXmppMessage& message) {
    if (message.type() != QXmppMessage::GroupChat) {
        return;
    }

    // Update background if this is a message from the current user
    bool fromSelf = getParticipantName(message.from()) == AccountManager::getInstance().getAccountInfo().getUsername();

    // Create message area
    ChatMessageArea* messageArea = new ChatMessageArea(true);
    messageArea->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    messageArea->setTextInteractionFlags(Qt::TextBrowserInteraction);
    messageArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    messageArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    messageArea->setReadOnly(true);

    messageArea->setStyleSheet("QTextBrowser{ padding-bottom: 2px;"
                               "padding-left: 2px;"
                               "padding-top: 2px;"
                               "padding-right: 20px;"
                               "margin: 0px;"
                               "color: #333333;"
                               "font-size: 14px;"
                               "background-color: rgba(0, 0, 0, 0%);"
                               "border: 0; }"
                               "QMenu{ border: 2px outset gray; }");

    QString userLabel = getParticipantName(message.from());
    if (fromSelf) {
        userLabel = "<b style=\"color: #4a6f91\">" + userLabel + ": </b>";
        messageArea->setStyleSheet(messageArea->styleSheet() + "background-color: #e1e8ea");
    } else {
        userLabel = "<b>" + userLabel + ": </b>";
    }

    messageArea->document()->setDefaultStyleSheet("a { text-decoration: none; font-weight: bold; color: #267077;}");
    QString messageText = message.body().toHtmlEscaped();
    messageText = messageText.replace(regexLinks, "<a href=\"\\1\">\\1</a>");
    messageText = messageText.replace(regexHifiLinks, "<a href=\"hifi://\\1\">\\1</a>");
    messageArea->setHtml(userLabel + messageText);

    bool atBottom = isNearBottom();

    ui->messagesVBoxLayout->addWidget(messageArea);
    ui->messagesVBoxLayout->parentWidget()->updateGeometry();
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

    QRegularExpression usernameMention(mentionRegex.arg(AccountManager::getInstance().getAccountInfo().getUsername()));
    if (isHidden() && message.body().contains(usernameMention)) {
        if (_effectPlayer.state() != QMediaPlayer::PlayingState) {
            // get random sound
            QFileInfo inf = QFileInfo(Application::resourcesPath()  +
                                      mentionSoundsPath +
                                      _mentionSounds.at(rand() % _mentionSounds.size()));
            _effectPlayer.setMedia(QUrl::fromLocalFile(inf.absoluteFilePath()));
            _effectPlayer.play();
        }

        _trayIcon.show();
        _trayIcon.showMessage(windowTitle(), message.body());
    }
}
#endif // HAVE_QXMPP

bool ChatWindow::isNearBottom() {
    QScrollBar* verticalScrollBar = ui->messagesScrollArea->verticalScrollBar();
    return verticalScrollBar->value() >= verticalScrollBar->maximum() - Ui::AUTO_SCROLL_THRESHOLD;
}

// Scroll chat message area to bottom.
void ChatWindow::scrollToBottom() {
    QScrollBar* verticalScrollBar = ui->messagesScrollArea->verticalScrollBar();
    verticalScrollBar->setValue(verticalScrollBar->maximum());
}

bool ChatWindow::event(QEvent* event) {
    if (event->type() == QEvent::WindowActivate) {
        ui->messagePlainTextEdit->setFocus();
    }
    return QWidget::event(event);
}
