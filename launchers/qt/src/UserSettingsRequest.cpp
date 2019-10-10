#include "UserSettingsRequest.h"

#include "Helper.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

const QByteArray ACCESS_TOKEN_AUTHORIZATION_HEADER = "Authorization";

void UserSettingsRequest::send(QNetworkAccessManager& nam, const LoginToken& token) {
    _state = State::Sending;

    QUrl lockerURL{ getMetaverseAPIDomain() };
    lockerURL.setPath("/api/v1/user/locker");

    QNetworkRequest lockerRequest(lockerURL);
    lockerRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    lockerRequest.setHeader(QNetworkRequest::UserAgentHeader, getHTTPUserAgent());
    lockerRequest.setRawHeader(ACCESS_TOKEN_AUTHORIZATION_HEADER, QString("Bearer %1").arg(token.accessToken).toUtf8());

    QNetworkReply* lockerReply = nam.get(lockerRequest);
    connect(lockerReply, &QNetworkReply::finished, this, &UserSettingsRequest::receivedResponse);
}

void UserSettingsRequest::receivedResponse() {
    _state = State::Finished;

    auto reply = static_cast<QNetworkReply*>(sender());

    qDebug() << "Got reply: " << reply->error();
    if (reply->error()) {
        _error = Error::Unknown;
        emit finished();
        return;
    }

    auto data = reply->readAll();
    qDebug() << "Settings: " << data;
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "Error parsing settings";
        _error = Error::Unknown;
        emit finished();
        return;
    }

    auto root = doc.object();
    if (root["status"] != "success") {
        qDebug() << "Status is not \"success\"";
        _error = Error::Unknown;
        emit finished();
        return;
    }

    if (root["data"].toObject().contains("home_location")) {
        QJsonValue homeLocation = root["data"].toObject()["home_location"];
        if (homeLocation.isString()) {
            _userSettings.homeLocation = homeLocation.toString();
        }
    }

    emit finished();
}
