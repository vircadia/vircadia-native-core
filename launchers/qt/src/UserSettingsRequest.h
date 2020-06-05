#pragma once

#include <QObject>
#include <QNetworkAccessManager>

#include "LoginRequest.h"

struct UserSettings {
    QString homeLocation;
};

class UserSettingsRequest : public QObject {
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
    };
    Q_ENUM(Error)

    void send(QNetworkAccessManager& nam, const LoginToken& token);
    Error getError() const { return _error; }

    UserSettings getUserSettings() const { return _userSettings; } 

signals:
    void finished();

private slots:
    void receivedResponse();

private:
    State _state { State::Unsent };
    Error _error { Error::None };

    UserSettings _userSettings;
};
