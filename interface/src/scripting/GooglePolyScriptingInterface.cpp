//
//  GooglePolyScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Elisa Lupin-Jimenez on 12/3/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GooglePolyScriptingInterface.h"

#include <QEventLoop>
#include <QtGlobal>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QTime>
#include <QUrl>

#include "ScriptEngineLogging.h"

const QString LIST_POLY_URL = "https://poly.googleapis.com/v1/assets?";
const QString GET_POLY_URL = "https://poly.googleapis.com/v1/assets/model?";

const QStringList VALID_FORMATS = QStringList() << "BLOCKS" << "FBX" << "GLTF" << "GLTF2" << "OBJ" << "TILT" << "";
const QStringList VALID_CATEGORIES = QStringList() << "animals" << "architecture" << "art" << "food" << 
    "nature" << "objects" << "people" << "scenes" << "technology" << "transport" << "";

GooglePolyScriptingInterface::GooglePolyScriptingInterface() {
    // nothing to be implemented
}

void GooglePolyScriptingInterface::setAPIKey(const QString& key) {
    _authCode = key;
}

QString GooglePolyScriptingInterface::getAssetList(const QString& keyword, const QString& category, const QString& format) {
    QUrl url = formatURLQuery(keyword, category, format);
    if (!url.isEmpty()) {
        QByteArray json = parseJSON(url, 0).toJsonDocument().toJson();
        return (QString) json;
    } else {
        qCDebug(scriptengine) << "Invalid filters were specified.";
        return "";
    }
}

QString GooglePolyScriptingInterface::getFBX(const QString& keyword, const QString& category) {
    QUrl url = formatURLQuery(keyword, category, "FBX");
    return getModelURL(url);
}

QString GooglePolyScriptingInterface::getOBJ(const QString& keyword, const QString& category) {
    QUrl url = formatURLQuery(keyword, category, "OBJ");
    return getModelURL(url);
}

QString GooglePolyScriptingInterface::getBlocks(const QString& keyword, const QString& category) {
    QUrl url = formatURLQuery(keyword, category, "BLOCKS");
    return getModelURL(url);
}

QString GooglePolyScriptingInterface::getGLTF(const QString& keyword, const QString& category) {
    QUrl url = formatURLQuery(keyword, category, "GLTF");
    return getModelURL(url);
}

QString GooglePolyScriptingInterface::getGLTF2(const QString& keyword, const QString& category) {
    QUrl url = formatURLQuery(keyword, category, "GLTF2");
    return getModelURL(url);
}
 
// This method will not be useful until we support Tilt models
QString GooglePolyScriptingInterface::getTilt(const QString& keyword, const QString& category) {
    QUrl url = formatURLQuery(keyword, category, "TILT");
    return getModelURL(url);
}

// Can provide asset name or full URL to model
QString GooglePolyScriptingInterface::getModelInfo(const QString& input) {
    QString name(input);
    if (input.contains("poly.googleapis") || input.contains("poly.google.com")) {
        QStringList list = input.split("/");
        if (input.contains("poly.googleapis")) {
            name = list[4];
        } else {
            name = list.last();
        }
    }
    QString urlString(GET_POLY_URL);
    urlString = urlString.replace("model", name) + "key=" + _authCode;
    qCDebug(scriptengine) << "Google URL request";
    QUrl url(urlString);
    QString json = parseJSON(url, 2).toString();
    return json;
}

int GooglePolyScriptingInterface::getRandIntInRange(int length) {
    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());
    return qrand() % length;
}

QUrl GooglePolyScriptingInterface::formatURLQuery(const QString& keyword, const QString& category, const QString& format) {
    QString queries;
    if (!VALID_FORMATS.contains(format, Qt::CaseInsensitive) || !VALID_CATEGORIES.contains(category, Qt::CaseInsensitive)) {
        return QUrl("");
    } else {
        if (!keyword.isEmpty()) {
            QString keywords(keyword);
            keywords.replace(" ", "+");
            queries.append("&keywords=" + keywords);
        }
        if (!category.isEmpty()) {
            queries.append("&category=" + category);
        }
        if (!format.isEmpty()) {
            queries.append("&format=" + format);
        }
        QString urlString(LIST_POLY_URL + "key=" + _authCode + queries);
        return QUrl(urlString);
    }
}

QString GooglePolyScriptingInterface::getModelURL(const QUrl& url) {
    qCDebug(scriptengine) << "Google URL request";
    if (!url.isEmpty()) {
        return parseJSON(url, 1).toString();
    } else {
        qCDebug(scriptengine) << "Invalid filters were specified.";
        return "";
    }
}

// FIXME: synchronous
QByteArray GooglePolyScriptingInterface::getHTTPRequest(const QUrl& url) {
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(url));
    QEventLoop event;
    connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();

    return response->readAll();
}

// 0 = asset list, 1 = model from asset list, 2 = specific model
QVariant GooglePolyScriptingInterface::parseJSON(const QUrl& url, int fileType) {
    QByteArray jsonString = getHTTPRequest(url);
    QJsonDocument doc = QJsonDocument::fromJson(jsonString);
    QJsonObject obj = doc.object();
    if (obj.isEmpty()) {
        qCDebug(scriptengine) << "Assets with specified filters not found";
        return "";
    }
    if (obj.keys().first() == "error") {
        QString error = obj.value("error").toObject().value("message").toString();
        qCDebug(scriptengine) << error;
        return "";
    }
    if (fileType == 0 || fileType == 1) {
        QJsonArray arr = obj.value("assets").toArray();
        // return model url
        if (fileType == 1) {
            int random = getRandIntInRange(arr.size());
            QJsonObject json = arr.at(random).toObject();
            // nested JSONs
            return json.value("formats").toArray().at(0).toObject().value("root").toObject().value("url");
        }
        // return whole asset list
        return QJsonDocument(arr);
    // return specific object
    } else {
        return jsonString;
    }
}
