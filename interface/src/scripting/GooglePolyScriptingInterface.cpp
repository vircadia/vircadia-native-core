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
#include <QGlobal.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
//#include <QScopedPointer>
#include <QString>
#include <QTime>
#include <QUrl>
#include <random>

#include "GooglePolyScriptingInterface.h"
#include "ScriptEngineLogging.h"

QString listPolyUrl = "https://poly.googleapis.com/v1/assets?";
QString getPolyUrl = "https://poly.googleapis.com/v1/assets/model?";

//authCode = "broke";

QStringList validFormats = QStringList() << "BLOCKS" << "FBX" << "GLTF" << "GLTF2" << "OBJ" << "TILT" << "";
QStringList validCategories = QStringList() << "animals" << "architecture" << "art" << "food" << 
    "nature" << "objects" << "people" << "scenes" << "technology" << "transport" << "";

GooglePolyScriptingInterface::GooglePolyScriptingInterface() {
    // nothing to be implemented
}

QJsonArray GooglePolyScriptingInterface::getAssetList(QString keyword, QString category, QString format) {
    QUrl url = formatURLQuery(keyword, category, format);
    if (!url.isEmpty()) {
        QByteArray jsonString = getHTTPRequest(url);
        QJsonArray json = parseJSON(&jsonString, 0).toJsonArray();
        qCDebug(scriptengine) << "first asset name: " << json.at(0).toObject().value("displayName");
        return json;
    } else {
        qCDebug(scriptengine) << "Invalid filters were specified.";
        return QJsonArray();
    }
}

QString GooglePolyScriptingInterface::getFBX(QString keyword, QString category) {
    QUrl url = formatURLQuery(keyword, category, "FBX");
    qCDebug(scriptengine) << "google url: " << url;
    if (!url.isEmpty()) {
        QByteArray jsonString = getHTTPRequest(url);
        qCDebug(scriptengine) << "the list: " << jsonString;
        QString modelURL = parseJSON(&jsonString, 1).toString();
        
        qCDebug(scriptengine) << "the model url: " << modelURL;
        return modelURL;
    } else {
        qCDebug(scriptengine) << "Invalid filters were specified.";
        return "";
    }

}

void GooglePolyScriptingInterface::testPrint(QString input) {
    qCDebug(scriptengine) << "Google message: " << input;
}

void GooglePolyScriptingInterface::setAPIKey(QString key) {
    authCode = key;
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

// FIXME: synchronous
QByteArray GooglePolyScriptingInterface::getHTTPRequest(QUrl url) {
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(url));
    QEventLoop event;
    connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();

    return response->readAll();

}

// since the list is a QJsonArray and a single model is a QJsonObject
QVariant GooglePolyScriptingInterface::parseJSON(QByteArray* response, int fileType) {
    //QString firstItem = QString::fromUtf8(response->readAll());
    QJsonDocument doc = QJsonDocument::fromJson(*response);
    QJsonObject obj = doc.object();
    qCDebug(scriptengine) << "json obj: " << obj;
    qCDebug(scriptengine) << "json list: " << obj.keys();
    if (obj.keys().first() == "error") {
        qCDebug(scriptengine) << "Invalid API key";
        return QJsonObject();
    }
    qCDebug(scriptengine) << "the assets: " << obj.value("assets");
    if (fileType == 0 || fileType == 1) {
        QJsonArray arr = obj.value("assets").toArray();
        qCDebug(scriptengine) << "in array: " << arr;
        // return model url
        if (fileType == 1) {
            int random = getRandIntInRange(arr.size());
            QJsonObject json = arr.at(random).toObject();
            qCDebug(scriptengine) << random << "th object: " << json;
            qCDebug(scriptengine) << random << "th asset name: " << json.value("displayName");
            // nested JSONs
            return json.value("formats").toArray().at(0).toObject().value("root").toObject().value("url");
        }
        // return whole asset list
        return arr;
    // return specific object
    } else {
        return obj;
    }
}






/*void GooglePolyScriptingInterface::getAssetList() {
    authCode = "AIzaSyDamk7Vth52j7aU9JVKn3ungFS0kGJYc8A";
    QUrl url(listPolyUrl + "key=" + authCode);
    QByteArray reply = getHTTPRequest(url);
    qCDebug(scriptengine) << "the list: " << test;
}

// FIXME: synchronous = not good code
QByteArray GooglePolyScriptingInterface::getHTTPRequest(QUrl url) {
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(url));
    QEventLoop event;
    connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();
    return response->readAll();
}

*/

/* useful for constructing the url?

    QUrl url("http://gdata.youtube.com/feeds/api/standardfeeds/");
    QString method = "most_popular";
    url.setPath(QString("%1%2").arg(url.path()).arg(method));

    QMap<QString, QVariant> params;
    params["alt"] = "json";
    params["v"] = "2";

    foreach(QString param, params.keys()) {
        url.addQueryItem(param, params[param].toString());
    }

*/

/* NONE OF THESE HTTP GET METHODS WORK D:
https://stackoverflow.com/questions/28267477/getting-a-page-content-with-qt kinda used rn

this correctly returns the asset list but is apparently synchronous and not a good way to do it 
https://stackoverflow.com/questions/24965972/qt-getting-source-html-code-of-a-web-page-hosted-on-the-internet
void GooglePolyScriptingInterface::getAssetList() {
    authCode = "AIzaSyDamk7Vth52j7aU9JVKn3ungFS0kGJYc8A";
    QUrl url(listPolyUrl + "key=" + authCode);
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(url));
    QEventLoop event;
    connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();
    QString firstItem = response->readAll();
    //QJsonArray arr;
    //QJsonObject jsonObject = QJsonDocument::fromJson(response->readAll()).object();
    //QString firstItem = jsonObject["assets"].toString();
    qCDebug(scriptengine) << "first item: " << firstItem;
    //qCDebug(scriptengine) << "api key: " << authCode;
    //return arr;
}

this didn't work either https://stackoverflow.com/a/24966317
    QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);
    QNetworkReply* response = manager->get(QNetworkRequest(url));
    QObject::connect(manager, &QNetworkAccessManager::finished, [manager, response] {
    response->deleteLater();
    manager->deleteLater();
    if (response->error() != QNetworkReply::NoError) return;
    QString contentType =
    response->header(QNetworkRequest::ContentTypeHeader).toString();
    if (!contentType.contains("charset=utf-8")) {
    qWarning() << "Content charsets other than utf-8 are not implemented yet.";
    return;
    }
    QString html = QString::fromUtf8(response->readAll());
    // do something with the data
    }) && manager.take();


or this :( https://stackoverflow.com/questions/39518749/make-get-request-to-web-service-get-json-response-and-update-gui-in-qt
    qCDebug(scriptengine) << "gonna get assets with " << url;
    QNetworkAccessManager manager;
    QNetworkReply* reply = manager.get(QNetworkRequest(url));
    QObject::connect(&manager, &QNetworkAccessManager::finished,
                    [reply]{
        qCDebug(scriptengine) << "boo radley";
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(scriptengine) << "Poly API failed to respond";
            //manager.clearAccessCache();
        } else {
            QJsonObject jsonObject = QJsonDocument::fromJson(reply->readAll()).object();
            QString firstItem = jsonObject["assets"].toString();
            qCDebug(scriptengine) << "first item: " << firstItem;
        }
        reply->deleteLater();
    });

or this :(( http://blog.mathieu-leplatre.info/access-a-json-webservice-with-qt-c.html
    //QJsonArray arr;
    //QJsonObject jsonObject = QJsonDocument::fromJson(response->readAll()).object();
    //QString firstItem = jsonObject["assets"].toString();
    qCDebug(scriptengine) << "first item: " << firstItem;
    //qCDebug(scriptengine) << "api key: " << authCode;
    //return arr;
    QNetworkAccessManager manager;
    QNetworkReply* response = manager.get(QNetworkRequest(url));
    QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onResult(QNetworkReply*)));

    return "";
    }

    void GooglePolyScriptingInterface::onResult(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
    qCDebug(scriptengine) << "Poly API failed to respond";
    return;
    }
    QString firstItem = reply->readAll();

    qCDebug(scriptengine) << "first item: " << firstItem;
    }

*/