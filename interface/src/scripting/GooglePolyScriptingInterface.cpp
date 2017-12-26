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
#include <random>

#include "GooglePolyScriptingInterface.h"
#include "ScriptEngineLogging.h"

const QString listPolyUrl = "https://poly.googleapis.com/v1/assets?";
const QString getPolyUrl = "https://poly.googleapis.com/v1/assets/model?";

const QStringList validFormats = QStringList() << "BLOCKS" << "FBX" << "GLTF" << "GLTF2" << "OBJ" << "TILT" << "";
const QStringList validCategories = QStringList() << "animals" << "architecture" << "art" << "food" << 
    "nature" << "objects" << "people" << "scenes" << "technology" << "transport" << "";

GooglePolyScriptingInterface::GooglePolyScriptingInterface() {
    // nothing to be implemented
}

void GooglePolyScriptingInterface::setAPIKey(QString key) {
    authCode = key;
}

QString GooglePolyScriptingInterface::getAssetList(QString keyword, QString category, QString format) {
    QUrl url = formatURLQuery(keyword, category, format);
    if (!url.isEmpty()) {
        QByteArray json = parseJSON(url, 0).toJsonDocument().toJson();
        return (QString) json;
    } else {
        qCDebug(scriptengine) << "Invalid filters were specified.";
        return "";
    }
}

QString GooglePolyScriptingInterface::getFBX(QString keyword, QString category) {
    QUrl url = formatURLQuery(keyword, category, "FBX");
    return getModelURL(url);
}

QString GooglePolyScriptingInterface::getOBJ(QString keyword, QString category) {
    QUrl url = formatURLQuery(keyword, category, "OBJ");
    return getModelURL(url);
}

QString GooglePolyScriptingInterface::getBlocks(QString keyword, QString category) {
    QUrl url = formatURLQuery(keyword, category, "BLOCKS");
    return getModelURL(url);
}

QString GooglePolyScriptingInterface::getGLTF(QString keyword, QString category) {
    QUrl url = formatURLQuery(keyword, category, "GLTF");
    return getModelURL(url);
}

QString GooglePolyScriptingInterface::getGLTF2(QString keyword, QString category) {
    QUrl url = formatURLQuery(keyword, category, "GLTF2");
    return getModelURL(url);
}

/* 
// This method will not work until we support Tilt models
QString GooglePolyScriptingInterface::getTilt(QString keyword, QString category) {
    QUrl url = formatURLQuery(keyword, category, "TILT");
    return getModelURL(url);
}
*/

// Can provide asset name or full URL to model
QString GooglePolyScriptingInterface::getModelInfo(QString name) {
    if (name.contains("poly.googleapis") || name.contains("poly.google.com")) {
        QStringList list = name.split("/");
        name = list[4];
    }
    QString urlString = QString(getPolyUrl);
    urlString = urlString.replace("model", name) + "key=" + authCode;
    qCDebug(scriptengine) << "Google URL request: " << urlString;
    QUrl url(urlString);
    QString json = parseJSON(url, 2).toString();
    return json;
}

int GooglePolyScriptingInterface::getRandIntInRange(int length) {
    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());
    return qrand() % length;
}

QUrl GooglePolyScriptingInterface::formatURLQuery(QString keyword, QString category, QString format) {
    QString queries;
    if (!validFormats.contains(format) || !validCategories.contains(category)) {
        return QUrl("");
    } else {
        if (!keyword.isEmpty()) {
            keyword.replace(" ", "+");
            queries.append("&keywords=" + keyword);
        }
        if (!category.isEmpty()) {
            queries.append("&category=" + category);
        }
        if (!format.isEmpty()) {
            queries.append("&format=" + format);
        }
        QString urlString = QString(listPolyUrl + "key=" + authCode + queries);
        return QUrl(urlString);
    }
}

QString GooglePolyScriptingInterface::getModelURL(QUrl url) {
    qCDebug(scriptengine) << "Google URL request: " << url;
    if (!url.isEmpty()) {
        return parseJSON(url, 1).toString();
    } else {
        qCDebug(scriptengine) << "Invalid filters were specified.";
        return "";
    }
}

// FIXME: synchronous
QByteArray GooglePolyScriptingInterface::getHTTPRequest(QUrl url) {
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(url));
    QEventLoop event;
    connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();

    return response->readAll();
}

// 0 = asset list, 1 = model from asset list, 2 = specific model
QVariant GooglePolyScriptingInterface::parseJSON(QUrl url, int fileType) {
    //QString firstItem = QString::fromUtf8(response->readAll());
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
