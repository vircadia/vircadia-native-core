#ifndef VIRCADIA_LIBRARIES_NETWORKING_SRC_ACME_HTTP_HPP
#define VIRCADIA_LIBRARIES_NETWORKING_SRC_ACME_HTTP_HPP

#include "http.h"

#include "acme-exception.h"
#include "../NetworkAccessManager.h"

#include <curl/curl.h>

#include <cstring>
#include <functional>
#include <type_traits>

#include <QtNetwork/QNetworkReply>

namespace acme_lw_internal
{

using namespace acme_lw;
using namespace std::literals;

struct QDelayedDeleter {
    void operator()(QObject* object) { object->deleteLater(); };
};
using QDelayedNetworkReply = std::unique_ptr<QNetworkReply, QDelayedDeleter>;

template <typename Callback>
void getHeader(Callback callback, const std::string& url, const char* headerKey) {
    QNetworkRequest request(QString::fromStdString(url));

    QNetworkReply* reply = NetworkAccessManager::getInstance().head(request);
    QObject::connect(reply, &QNetworkReply::finished, [reply, callback = std::move(callback), headerKey]() {
        auto replyPtr = QDelayedNetworkReply(reply);
        if(replyPtr->error() != QNetworkReply::NoError) {
            callback(AcmeException(replyPtr->errorString().toStdString()));
        } else {
            if(replyPtr->hasRawHeader(headerKey)) {
                callback(replyPtr->rawHeader(headerKey).toStdString());
            } else {
                callback(AcmeException("Reply missing header: "s + headerKey));
            }
        }
    });
}

template <typename Callback>
void doPost(Callback callback, const std::string& url, const std::string& postBody, const char * headerKey) {
    QNetworkRequest request(QString::fromStdString(url));

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/jose+json");

    QNetworkReply* reply = NetworkAccessManager::getInstance().post(request, QByteArray::fromStdString(postBody));
    QObject::connect(reply, &QNetworkReply::finished, [reply, callback = std::move(callback), headerKey]() {
        auto replyPtr = QDelayedNetworkReply(reply);
        if(replyPtr->error() != QNetworkReply::NoError) {
            callback(AcmeException(replyPtr->errorString().toStdString()));
        } else {
            Response response{};
            if(headerKey)
            {
                if(!replyPtr->hasRawHeader(headerKey)) {
                    response.headerValue_ = replyPtr->rawHeader(headerKey).toStdString();
                } else {
                    callback(AcmeException("Reply missing header: "s + headerKey));
                    return;
                }
            };
            response.response_.resize(replyPtr->bytesAvailable());
            replyPtr->read(response.response_.data(), response.response_.size());
            callback(std::move(response));
        }
    });
}

template <typename Callback>
void doGet(Callback callback, const std::string& url) {
    QNetworkRequest request(QString::fromStdString(url));
    QNetworkReply* reply = NetworkAccessManager::getInstance().get(request);
    QObject::connect(reply, &QNetworkReply::finished, [reply, callback = std::move(callback)]() mutable {
        auto replyPtr = QDelayedNetworkReply(reply);
        if(replyPtr->error() != QNetworkReply::NoError) {
            callback(AcmeException(replyPtr->errorString().toStdString()));
        } else {
            Response response{};
            response.response_.resize(replyPtr->bytesAvailable());
            replyPtr->read(response.response_.data(), response.response_.size());
            callback(std::move(response));
        }
    });
}

}

#endif /* end of include guard */
