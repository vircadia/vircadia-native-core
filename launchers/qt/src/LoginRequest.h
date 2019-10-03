#pragma once

#include <QObject>
#include <QNetworkAccessManager>

struct LoginToken {
    QString accessToken;
    QString tokenType;
    QString refreshToken;
};

class LoginRequest : public QObject {
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
        ServerError,
        BadResponse,
        BadUsernameOrPassword
    };
    Q_ENUM(Error)

    void send(QNetworkAccessManager& nam, QString username, QString password);
    Error getError() const { return _error; }

    // The token is only valid if the request has finished without error
    QString getRawToken() const { return _rawLoginToken; }
    LoginToken getToken() const { return _token; }

signals:
    void finished();

private slots:
    void receivedResponse();

private:
    State _state { State::Unsent };
    Error _error { Error::None };

    QString _rawLoginToken;
    LoginToken _token;
};

