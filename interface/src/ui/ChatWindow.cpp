#include <QFormLayout>
#include <QFrame>
#include <QLayoutItem>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTextDocument>

#include "ChatWindow.h"
#include "ui_chatwindow.h"
#include "FlowLayout.h"

#include <QXmppClient.h>
#include <QXmppMessage.h>
#include <Application.h>
#include <AccountManager.h>

const QString DEFAULT_SERVER = "chat.highfidelity.io";
const QString DEFAULT_CHAT_ROOM = "public@public-chat.highfidelity.io";

const QRegularExpression regexLinks("((?:(?:ftp)|(?:https?))://\\S+)");

ChatWindow::ChatWindow() :
    QDialog(Application::getInstance()->getGLWidget(), Qt::Tool),
    ui(new Ui::ChatWindow) {
    ui->setupUi(this);

    FlowLayout* flowLayout = new FlowLayout();
    flowLayout->setContentsMargins(0, 8, 0, 8);
    ui->usersWidget->setLayout(flowLayout);

    ui->messagePlainTextEdit->installEventFilter(this);

    ui->numOnlineLabel->hide();
    ui->usersWidget->hide();
    ui->messagesScrollArea->hide();
    ui->messagePlainTextEdit->hide();

    setAttribute(Qt::WA_DeleteOnClose);

    _xmppClient.addExtension(&_xmppMUCManager);
    connect(&_xmppClient, SIGNAL(connected()), this, SLOT(connected()));
    connect(&_xmppClient, SIGNAL(error(QXmppClient::Error)), this, SLOT(error(QXmppClient::Error)));
    connect(&_xmppClient, SIGNAL(messageReceived(QXmppMessage)), this, SLOT(messageReceived(QXmppMessage)));

    AccountManager& accountManager = AccountManager::getInstance();
    QString user = accountManager.getUsername();
    const QString& password = accountManager.getXMPPPassword();
    _xmppClient.connectToServer(user + "@" + DEFAULT_SERVER, password);
}

ChatWindow::~ChatWindow() {
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
        _xmppClient.sendMessage(_chatRoom->jid(), ui->messagePlainTextEdit->document()->toPlainText());
        ui->messagePlainTextEdit->document()->clear();
        return true;
    }
    return false;
}

QString ChatWindow::getParticipantName(const QString& participant) {
    return participant.right(participant.count() - 1 - _chatRoom->jid().count());
}

void ChatWindow::connected() {
    _chatRoom = _xmppMUCManager.addRoom(DEFAULT_CHAT_ROOM);
    connect(_chatRoom, SIGNAL(participantsChanged()), this, SLOT(participantsChanged()));
    _chatRoom->setNickName(AccountManager::getInstance().getUsername());
    _chatRoom->join();

    ui->connectingToXMPPLabel->hide();
    ui->numOnlineLabel->show();
    ui->usersWidget->show();
    ui->messagesScrollArea->show();
    ui->messagePlainTextEdit->show();
}

void ChatWindow::error(QXmppClient::Error error) {
    ui->connectingToXMPPLabel->setText(QString::number(error));
}

void ChatWindow::participantsChanged() {
    QStringList participants = _chatRoom->participants();
    ui->numOnlineLabel->setText(tr("%1 online now:").arg(participants.count()));

    while (QLayoutItem* item = ui->usersWidget->layout()->takeAt(0)) {
        delete item;
    }
    foreach (const QString& participant, participants) {
        QLabel* userLabel = new QLabel(getParticipantName(participant));
        userLabel->setStyleSheet(
            "background-color: palette(light);"
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

    QLabel* messageLabel = new QLabel(message.body().replace(regexLinks, "<a href=\"\\1\">\\1</a>"));
    messageLabel->setWordWrap(true);
    messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    messageLabel->setOpenExternalLinks(true);

    ui->messagesFormLayout->addRow(userLabel, messageLabel);
    ui->messagesFormLayout->parentWidget()->updateGeometry();
    Application::processEvents();
    QScrollBar* verticalScrollBar = ui->messagesScrollArea->verticalScrollBar();
    verticalScrollBar->setSliderPosition(verticalScrollBar->maximum());
}
