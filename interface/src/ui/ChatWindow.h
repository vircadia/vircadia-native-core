#ifndef __interface__ChatWindow__
#define __interface__ChatWindow__

#include <QDialog>

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
    Ui::ChatWindow* ui;
    QXmppClient _xmppClient;
    QXmppMucManager _xmppMUCManager;
    QXmppMucRoom* _chatRoom;

    QString getParticipantName(const QString& participant);

private slots:
    void connected();
    void error(QXmppClient::Error error);
    void participantsChanged();
    void messageReceived(const QXmppMessage& message);
};

#endif /* defined(__interface__ChatWindow__) */
