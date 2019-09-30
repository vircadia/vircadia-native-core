#pragma once

#include <QObject>
#include <QNetworkAccessManager>

class SignupRequest : public QObject {
    Q_OBJECT
public:
    enum class State {
        Unsent,
        Sending,
        Finished
    };

    enum class Error {
        None = 0,
        Unknown,
        NoSuchEmail,
        UserProfileAlreadyCompleted,
        BadUsername,
        ExistingUsername,
        BadPassword,
    };
    Q_ENUM(Error)

    void send(QNetworkAccessManager& nam, QString email, QString username, QString password);
    Error getError() const { return _error; }

signals:
    void finished();

private slots:
    void receivedResponse();

private:
    State _state { State::Unsent };
    Error _error { Error::None };
};
