#include "SignupRequest.h"

#include "Helper.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

void SignupRequest::send(QNetworkAccessManager& nam, QString email, QString username, QString password) {
    if (_state != State::Unsent) {
        qDebug() << "Error: Trying to send signuprequest, but not unsent";
        return;
    }

    _state = State::Sending;

    QUrl signupURL { getMetaverseAPIDomain() };
    signupURL.setPath("/api/v1/user/channel_user");
    QNetworkRequest request(signupURL);

    request.setHeader(QNetworkRequest::UserAgentHeader, getHTTPUserAgent());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery query;
    query.addQueryItem("email", QUrl::toPercentEncoding(email));
    query.addQueryItem("username", QUrl::toPercentEncoding(username));
    query.addQueryItem("password", QUrl::toPercentEncoding(password));

    auto reply = nam.put(request, query.query(QUrl::FullyEncoded).toLatin1());
    QObject::connect(reply, &QNetworkReply::finished, this, &SignupRequest::receivedResponse);
}

void SignupRequest::receivedResponse() {
    _state = State::Finished;

    auto reply = static_cast<QNetworkReply*>(sender());

    if (reply->error() && reply->size() == 0) {
        qDebug() << "Error signing up: " << reply->error() << reply->readAll();
        _error = Error::Unknown;
        emit finished();
        return;
    }

    auto data = reply->readAll();
    qDebug() << "Signup response: " << data;
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "Error parsing response for signup " << data;
        _error = Error::Unknown;
        emit finished();
        return;
    }

    auto root = doc.object();

    auto status = root["status"];
    if (status.isString() && status.toString() != "success") {
        auto error = root["data"].toString();

        _error = Error::Unknown;

        if (error == "no_such_email") {
            _error = Error::NoSuchEmail;
        } else if (error == "user_profile_already_completed") {
            _error = Error::UserProfileAlreadyCompleted;
        } else if (error == "bad_username") {
            _error = Error::BadUsername;
        } else if (error == "existing_username") {
            _error = Error::ExistingUsername;
        } else if (error == "bad_password") {
            _error = Error::BadPassword;
        }
    }

    emit finished();
}
