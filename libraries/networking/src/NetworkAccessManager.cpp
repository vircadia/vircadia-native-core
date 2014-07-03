//
//  NetworkAccessManager.cpp
//
//
//  Created by Clement on 7/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QMetaObject>
#include <QThread>

#include "NetworkAccessManager.h"

NetworkAccessManager& NetworkAccessManager::getInstance() {
    static NetworkAccessManager sharedInstance;
    return sharedInstance;
}

NetworkAccessManager::NetworkAccessManager() {
}

QNetworkReply* NetworkAccessManager::get(const QNetworkRequest& request) {
    if (QThread::currentThread() != thread()) {
        QNetworkReply* result;
        QMetaObject::invokeMethod(this,
                                  "get",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QNetworkReply*, result),
                                  Q_ARG(const QNetworkRequest, request));
        return result;
    }
    return QNetworkAccessManager::get(request);
}

QNetworkReply* NetworkAccessManager::head(const QNetworkRequest& request) {
    if (QThread::currentThread() != thread()) {
        QNetworkReply* result;
        QMetaObject::invokeMethod(this,
                                  "head",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QNetworkReply*, result),
                                  Q_ARG(const QNetworkRequest, request));
        return result;
    }
    return QNetworkAccessManager::head(request);
}

QNetworkReply* NetworkAccessManager::post(const QNetworkRequest& request, QIODevice* data) {
    if (QThread::currentThread() != thread()) {
        QNetworkReply* result;
        QMetaObject::invokeMethod(this,
                                  "post",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QNetworkReply*, result),
                                  Q_ARG(const QNetworkRequest, request),
                                  Q_ARG(QIODevice*, data));
        return result;
    }
    return QNetworkAccessManager::post(request, data);
}

QNetworkReply* NetworkAccessManager::post(const QNetworkRequest& request, const QByteArray& data) {
    if (QThread::currentThread() != thread()) {
        QNetworkReply* result;
        QMetaObject::invokeMethod(this,
                                  "post",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QNetworkReply*, result),
                                  Q_ARG(const QNetworkRequest, request),
                                  Q_ARG(const QByteArray, data));
        return result;
    }
    return QNetworkAccessManager::post(request, data);
}

QNetworkReply* NetworkAccessManager::post(const QNetworkRequest& request, QHttpMultiPart* multiPart) {
    if (QThread::currentThread() != thread()) {
        QNetworkReply* result;
        QMetaObject::invokeMethod(this,
                                  "post",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QNetworkReply*, result),
                                  Q_ARG(const QNetworkRequest, request),
                                  Q_ARG(QHttpMultiPart*, multiPart));
        return result;
    }
    return QNetworkAccessManager::post(request, multiPart);
}

QNetworkReply* NetworkAccessManager::put(const QNetworkRequest& request, QIODevice* data) {
    if (QThread::currentThread() != thread()) {
        QNetworkReply* result;
        QMetaObject::invokeMethod(this,
                                  "put",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QNetworkReply*, result),
                                  Q_ARG(const QNetworkRequest, request),
                                  Q_ARG(QIODevice*, data));
        return result;
    }
    return QNetworkAccessManager::put(request, data);
}

QNetworkReply* NetworkAccessManager::put(const QNetworkRequest& request, QHttpMultiPart* multiPart) {
    if (QThread::currentThread() != thread()) {
        QNetworkReply* result;
        QMetaObject::invokeMethod(this,
                                  "put",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QNetworkReply*, result),
                                  Q_ARG(const QNetworkRequest, request),
                                  Q_ARG(QHttpMultiPart*, multiPart));
        return result;
    }
    return QNetworkAccessManager::put(request, multiPart);
}

QNetworkReply* NetworkAccessManager::put(const QNetworkRequest & request, const QByteArray & data) {
    if (QThread::currentThread() != thread()) {
        QNetworkReply* result;
        QMetaObject::invokeMethod(this,
                                  "put",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QNetworkReply*, result),
                                  Q_ARG(const QNetworkRequest, request),
                                  Q_ARG(const QByteArray, data));
        return result;
    }
    return QNetworkAccessManager::put(request, data);
}


QNetworkReply* NetworkAccessManager::sendCustomRequest(const QNetworkRequest& request, const QByteArray& verb, QIODevice* data) {
    if (QThread::currentThread() != thread()) {
        QNetworkReply* result;
        QMetaObject::invokeMethod(this,
                                  "sendCustomRequest",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QNetworkReply*, result),
                                  Q_ARG(const QNetworkRequest, request),
                                  Q_ARG(const QByteArray, verb),
                                  Q_ARG(QIODevice*, data));
        return result;
    }
    return QNetworkAccessManager::sendCustomRequest(request, verb, data);
}