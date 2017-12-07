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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
//#include <QScopedPointer>
#include <QString>
#include <QUrl>

#include "GooglePolyScriptingInterface.h"
#include "ScriptEngineLogging.h"

QString listPolyUrl = "https://poly.googleapis.com/v1/assets?";
QString getPolyUrl = "https://poly.googleapis.com/v1/assets/model?";

GooglePolyScriptingInterface::GooglePolyScriptingInterface() {
    // nothing to be implemented
}

void GooglePolyScriptingInterface::testPrint() {
    qCDebug(scriptengine) << "Google Poly interface exists";
}

void GooglePolyScriptingInterface::setAPIKey(QString key) {
    authCode = key;
}

void GooglePolyScriptingInterface::getAssetList() {
    authCode = "AIzaSyDamk7Vth52j7aU9JVKn3ungFS0kGJYc8A";
    //authCode = "broke";
    QUrl url(listPolyUrl + "key=" + authCode);
    QByteArray jsonString = getHTTPRequest(url);
    qCDebug(scriptengine) << "the list: " << jsonString;
    QJsonObject json = makeJSONObject(&jsonString, true);

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

QJsonObject GooglePolyScriptingInterface::makeJSONObject(QByteArray* response, bool isList) {
    //QString firstItem = QString::fromUtf8(response->readAll());
    QJsonDocument doc = QJsonDocument::fromJson(*response);
    qCDebug(scriptengine) << "json doc is empty: " << doc.isEmpty();
    QJsonObject obj = doc.object();
    qCDebug(scriptengine) << "json obj: " << obj;
    qCDebug(scriptengine) << "json list: " << obj.keys();
    if (obj.keys().first() == "error") {
        qCDebug(scriptengine) << "Invalid API key";
        return obj;
    }
    qCDebug(scriptengine) << "total size: " << obj.value("totalSize").toString();
    qCDebug(scriptengine) << "in assets: " << obj.value("assets");
    return obj;
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

