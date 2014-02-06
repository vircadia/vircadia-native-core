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
#include "ScriptCache.h"

ScriptCache* ScriptCache::getInstance() {
    static ScriptCache cache;
    return &cache;
}

ScriptCache::ScriptCache() :
    _networkAccessManager(NULL),
    _engine(NULL) {
    
    setEngine(new QScriptEngine(this));
}

void ScriptCache::setEngine(QScriptEngine* engine) {
    if (_engine && _engine->parent() == this) {
        delete _engine;
    }
    AttributeRegistry::getInstance()->configureScriptEngine(_engine = engine);
    _parametersString = engine->toStringHandle("parameters");
    _lengthString = engine->toStringHandle("length");
    _nameString = engine->toStringHandle("name");
    _typeString = engine->toStringHandle("type");
    _generatorString = engine->toStringHandle("generator");
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
        value = QSharedPointer<NetworkValue>(url.getParameters().isEmpty() ?
            (NetworkValue*)new RootNetworkValue(getProgram(url.getURL())) :
            (NetworkValue*)new DerivedNetworkValue(getValue(url.getURL()), url.getParameters()));
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
    
    emit loaded();
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

NetworkValue::~NetworkValue() {
}

RootNetworkValue::RootNetworkValue(const QSharedPointer<NetworkProgram>& program) :
    _program(program) {
}

QScriptValue& RootNetworkValue::getValue() {
    if (!_value.isValid() && _program->isLoaded()) {
        _value = _program->getCache()->getEngine()->evaluate(_program->getProgram());
    }
    return _value;
}

const QList<ParameterInfo>& RootNetworkValue::getParameterInfo() {
    if (isLoaded() && _parameterInfo.isEmpty()) {
        ScriptCache* cache = _program->getCache();
        QScriptEngine* engine = cache->getEngine();
        QScriptValue parameters = _value.property(cache->getParametersString());
        if (parameters.isArray()) {
            int length = parameters.property(cache->getLengthString()).toInt32();
            for (int i = 0; i < length; i++) {
                QScriptValue parameter = parameters.property(i);
                ParameterInfo info = { engine->toStringHandle(parameter.property(cache->getNameString()).toString()),
                    QMetaType::type(parameter.property(cache->getTypeString()).toString().toUtf8().constData()) };
                _parameterInfo.append(info);
            }
        }
    }
    return _parameterInfo;
}

DerivedNetworkValue::DerivedNetworkValue(const QSharedPointer<NetworkValue>& baseValue, const ScriptHash& parameters) :
    _baseValue(baseValue), 
    _parameters(parameters) {
}

QScriptValue& DerivedNetworkValue::getValue() {
    if (!_value.isValid() && _baseValue->isLoaded()) {
        RootNetworkValue* root = static_cast<RootNetworkValue*>(_baseValue.data());
        ScriptCache* cache = root->getProgram()->getCache();
        QScriptValue generator = _baseValue->getValue().property(cache->getGeneratorString());
        if (generator.isFunction()) {
            QScriptValueList arguments;
            foreach (const ParameterInfo& parameter, root->getParameterInfo()) {
                arguments.append(cache->getEngine()->newVariant(_parameters.value(parameter.name)));
            }
            _value = generator.call(QScriptValue(), arguments);
        
        } else {
            _value = _baseValue->getValue();                      
        }
    }
    return _value;
}
