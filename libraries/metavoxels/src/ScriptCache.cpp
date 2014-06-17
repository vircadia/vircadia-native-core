//
//  ScriptCache.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 2/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cmath>

#include <QNetworkReply>
#include <QScriptEngine>
#include <QScriptValueIterator>
#include <QTextStream>

#include "AttributeRegistry.h"
#include "ScriptCache.h"

static int scriptValueMetaTypeId = qRegisterMetaType<QScriptValue>();
static bool scriptValueComparators = QMetaType::registerComparators<QScriptValue>();

bool operator==(const QScriptValue& first, const QScriptValue& second) {
    if (first.isUndefined()) {
        return second.isUndefined();
        
    } else if (first.isNull()) {
        return second.isNull();
    
    } else if (first.isBool()) {
        return second.isBool() && first.toBool() == second.toBool();
    
    } else if (first.isNumber()) {
        return second.isNumber() && first.toNumber() == second.toNumber();
    
    } else if (first.isString()) {
        return second.isString() && first.toString() == second.toString();
    
    } else if (first.isVariant()) {
        return second.isVariant() && first.toVariant() == second.toVariant();
        
    } else if (first.isQObject()) {
        return second.isQObject() && first.toQObject() == second.toQObject();
    
    } else if (first.isQMetaObject()) {
        return second.isQMetaObject() && first.toQMetaObject() == second.toQMetaObject();
        
    } else if (first.isDate()) {
        return second.isDate() && first.toDateTime() == second.toDateTime();
    
    } else if (first.isRegExp()) {
        return second.isRegExp() && first.toRegExp() == second.toRegExp();
    
    } else if (first.isArray()) {
        if (!second.isArray()) {
            return false;
        }
        int length = first.property(ScriptCache::getInstance()->getLengthString()).toInt32();
        if (second.property(ScriptCache::getInstance()->getLengthString()).toInt32() != length) {
            return false;
        }
        for (int i = 0; i < length; i++) {
            if (first.property(i) != second.property(i)) {
                return false;
            }
        }
        return true;
        
    } else if (first.isObject()) {
        if (!second.isObject()) {
            return false;
        }
        int propertyCount = 0;
        for (QScriptValueIterator it(first); it.hasNext(); ) {
            it.next();
            if (second.property(it.scriptName()) != it.value()) {
                return false;
            }
            propertyCount++;
        }
        // make sure the second has exactly as many properties as the first
        for (QScriptValueIterator it(second); it.hasNext(); ) {
            it.next();
            if (--propertyCount < 0) {
                return false;
            }
        }
        return true;
        
    } else {
        // if none of the above tests apply, first must be invalid
        return !second.isValid();
    }
}

bool operator!=(const QScriptValue& first, const QScriptValue& second) {
    return !(first == second);
}

bool operator<(const QScriptValue& first, const QScriptValue& second) {
    return first.lessThan(second);
}

ScriptCache* ScriptCache::getInstance() {
    static ScriptCache cache;
    return &cache;
}

ScriptCache::ScriptCache() :
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

QSharedPointer<Resource> ScriptCache::createResource(const QUrl& url,
        const QSharedPointer<Resource>& fallback, bool delayLoad, const void* extra) {
    return QSharedPointer<Resource>(new NetworkProgram(this, url), &Resource::allReferencesCleared);
}

NetworkProgram::NetworkProgram(ScriptCache* cache, const QUrl& url) :
    Resource(url),
    _cache(cache) {
}

void NetworkProgram::downloadFinished(QNetworkReply* reply) {
    _program = QScriptProgram(QTextStream(reply).readAll(), reply->url().toString());
    reply->deleteLater();
    finishedLoading(true);
    emit loaded();
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
