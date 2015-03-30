//
//  ScriptCache.cpp
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 2015-03-30
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

#include <NetworkAccessManager.h>
#include <SharedUtil.h>

#include "ScriptCache.h"

ScriptCache::ScriptCache(QObject* parent) {
    // nothing to do here...
}

// QHash<QUrl, QString> _scriptCache;

void ScriptCache::getScript(const QUrl& url, ScriptUser* scriptUser) {
    if (_scriptCache.contains(url)) {
        scriptUser->scriptContentsAvailable(url, _scriptCache[url]);
        return;
    }
    _scriptUsers.insert(url, scriptUser);
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest networkRequest = QNetworkRequest(url);
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    QNetworkReply* reply = networkAccessManager.get(networkRequest);
    connect(reply, &QNetworkReply::finished, this, &ScriptCache::scriptDownloaded);
}

void ScriptCache::scriptDownloaded() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QUrl url = reply->url();
    QList<ScriptUser*> scriptUsers = _scriptUsers.values(url);
    _scriptUsers.remove(url);
    
    if (reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
        _scriptCache[url] = reply->readAll();
        qDebug() << "_scriptCache.size:" << _scriptCache.size();
        
        foreach(ScriptUser* user, scriptUsers) {
            user->scriptContentsAvailable(url, _scriptCache[url]);
        }
    } else {
        qDebug() << "ERROR Loading file:" << reply->url().toString();
        foreach(ScriptUser* user, scriptUsers) {
            user->errorInLoadingScript(url);
        }
    }
    reply->deleteLater();
}

QString ScriptCache::getScript(const QUrl& url) {
    qDebug() << "Script requested: " << url.toString();
    QString scriptContents;
    if (_scriptCache.contains(url)) {
        qDebug() << "Script found in cache: " << url.toString();
        scriptContents = _scriptCache[url];
    } else {
        qDebug() << "Script not found in cache: " << url.toString();
        
        if (_blockingScriptsPending.contains(url)) {
            // if we're already downloading this script, wait for it to complete...
            qDebug() << "Already downloading script: " << url.toString();
            while (_blockingScriptsPending.contains(url)) {
                QEventLoop loop;
                qDebug() << "about to call loop.processEvents()....";
                loop.processEvents(QEventLoop::AllEvents, 1);
                qDebug() << "AFTER loop.processEvents()....";
            }
            qDebug() << "Done waiting on downloading script: " << url.toString();
            scriptContents = _scriptCache[url];
        } else {
            _blockingScriptsPending.insert(url);
            qDebug() << "_blockingScriptsPending: " << _blockingScriptsPending;
            
            QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
            QNetworkRequest networkRequest = QNetworkRequest(url);
            networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
            QNetworkReply* reply = networkAccessManager.get(networkRequest);


            connect(reply, &QNetworkReply::finished, this, &ScriptCache::scriptDownloadedSyncronously);
            qDebug() << "Downloading script at" << url.toString();
            //QEventLoop loop;
            //QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
            //qDebug() << "starting loop....";
            //loop.exec();
            //qDebug() << "after loop....";

            /*
            if (reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
                scriptContents = reply->readAll();
                _scriptCache[url] = scriptContents;
                qDebug() << "_scriptCache.size:" << _scriptCache.size();
            } else {
                qDebug() << "ERROR Loading file:" << url.toString();
            }
            delete reply;
            _blockingScriptsPending.remove(url);
            qDebug() << "_blockingScriptsPending: " << _blockingScriptsPending;
            */
            while (_blockingScriptsPending.contains(url)) {
                QEventLoop loop;
                qDebug() << "about to call loop.processEvents()....";
                loop.processEvents(QEventLoop::AllEvents, 1);
                qDebug() << "AFTER loop.processEvents()....";
            }
            qDebug() << "Done waiting on downloading script: " << url.toString();
            scriptContents = _scriptCache[url];
        }
    }
    return scriptContents;
}

void ScriptCache::scriptDownloadedSyncronously() {

    qDebug() << "ScriptCache::scriptDownloadedSyncronously()....";
    
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QUrl url = reply->url();
    
    if (reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
        _scriptCache[url] = reply->readAll();
        qDebug() << "_scriptCache.size:" << _scriptCache.size();
    } else {
        qDebug() << "ERROR Loading file:" << reply->url().toString();
    }
    
    reply->deleteLater();

    _blockingScriptsPending.remove(url);
    qDebug() << "_blockingScriptsPending: " << _blockingScriptsPending;
}
    
