//
//  ScriptCache.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 2/4/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <cmath>

#include <QNetworkReply>
#include <QScriptEngine>
#include <QTextStream>
#include <QTimer>
#include <QtDebug>

#include "AttributeRegistry.h"
#include "MetavoxelUtil.h"
#include "ScriptCache.h"

ScriptCache* ScriptCache::getInstance() {
    static ScriptCache cache;
    return &cache;
}

ScriptCache::ScriptCache() :
    _networkAccessManager(NULL),
    _engine(new QScriptEngine(this)) {
    
    AttributeRegistry::getInstance()->configureScriptEngine(_engine);
}

void ScriptCache::setEngine(QScriptEngine* engine) {
    if (_engine->parent() == this) {
        delete _engine;
    }
    _engine = engine;
}

QSharedPointer<NetworkProgram> ScriptCache::getProgram(const QUrl& url) {
    QSharedPointer<NetworkProgram> program = _networkPrograms.value(url);
    if (program.isNull()) {
        program = QSharedPointer<NetworkProgram>(new NetworkProgram(this, url));
        _networkPrograms.insert(url, program);
    }
    return program;
}

QSharedPointer<NetworkValue> ScriptCache::getValue(const ParameterizedURL& url) {
    QSharedPointer<NetworkValue> value = _networkValues.value(url);
    if (value.isNull()) {
        value = QSharedPointer<NetworkValue>(new NetworkValue(getProgram(url.getURL()), url.getParameters()));
        _networkValues.insert(url, value);
    }
    return value;
}

NetworkProgram::NetworkProgram(ScriptCache* cache, const QUrl& url) :
    _cache(cache),
    _request(url),
    _reply(NULL),
    _attempts(0) {
    
    if (!url.isValid()) {
        return;
    }
    _request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    makeRequest();
}

NetworkProgram::~NetworkProgram() {
    if (_reply != NULL) {
        delete _reply;
    }
}

void NetworkProgram::makeRequest() {
    QNetworkAccessManager* manager = _cache->getNetworkAccessManager();
    if (manager == NULL) {
        return;
    }
    _reply = manager->get(_request);
    
    connect(_reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(handleDownloadProgress(qint64,qint64)));
    connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleReplyError()));
}

void NetworkProgram::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesReceived < bytesTotal && !_reply->isFinished()) {
        return;
    }
    _program = QScriptProgram(QTextStream(_reply).readAll(), _reply->url().toString());
    
    _reply->disconnect(this);
    _reply->deleteLater();
    _reply = NULL;
}

void NetworkProgram::handleReplyError() {
    QDebug debug = qDebug() << _reply->errorString();
    
    _reply->disconnect(this);
    _reply->deleteLater();
    _reply = NULL;
    
    // retry with increasing delays
    const int MAX_ATTEMPTS = 8;
    const int BASE_DELAY_MS = 1000;
    if (++_attempts < MAX_ATTEMPTS) {
        QTimer::singleShot(BASE_DELAY_MS * (int)pow(2.0, _attempts), this, SLOT(makeRequest()));
        debug << " -- retrying...";
    }
}

NetworkValue::NetworkValue(const QSharedPointer<NetworkProgram>& program, const QVariantHash& parameters) :
    _program(program),
    _parameters(parameters) {
}

QScriptValue& NetworkValue::getValue() {
    if (!_value.isValid() && _program->isLoaded()) {
        _value = _program->getCache()->getEngine()->evaluate(_program->getProgram());
    }
    return _value;
}
