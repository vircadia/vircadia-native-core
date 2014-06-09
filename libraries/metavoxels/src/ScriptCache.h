//
//  ScriptCache.h
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 2/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptCache_h
#define hifi_ScriptCache_h

#include <QScriptProgram>
#include <QScriptValue>

#include <ResourceCache.h>

#include "MetavoxelUtil.h"

class QScriptEngine;

class NetworkProgram;
class NetworkValue;

/// Maintains a cache of loaded scripts.
class ScriptCache : public ResourceCache {
    Q_OBJECT

public:

    static ScriptCache* getInstance();

    ScriptCache();
    
    void setEngine(QScriptEngine* engine);
    QScriptEngine* getEngine() const { return _engine; }
    
    /// Loads a script program from the specified URL.
    QSharedPointer<NetworkProgram> getProgram(const QUrl& url) { return getResource(url).staticCast<NetworkProgram>(); }

    /// Loads a script value from the specified URL.
    QSharedPointer<NetworkValue> getValue(const ParameterizedURL& url);

    const QScriptString& getParametersString() const { return _parametersString; }
    const QScriptString& getLengthString() const { return _lengthString; }
    const QScriptString& getNameString() const { return _nameString; }
    const QScriptString& getTypeString() const { return _typeString; }
    const QScriptString& getGeneratorString() const { return _generatorString; }

protected:

    virtual QSharedPointer<Resource> createResource(const QUrl& url,
        const QSharedPointer<Resource>& fallback, bool delayLoad, const void* extra);

private:
    
    QScriptEngine* _engine;
    QHash<ParameterizedURL, QWeakPointer<NetworkValue> > _networkValues;
    QScriptString _parametersString;
    QScriptString _lengthString;
    QScriptString _nameString;
    QScriptString _typeString;
    QScriptString _generatorString;
};

Q_DECLARE_METATYPE(QScriptValue)

bool operator==(const QScriptValue& first, const QScriptValue& second);
bool operator!=(const QScriptValue& first, const QScriptValue& second);
bool operator<(const QScriptValue& first, const QScriptValue& second);

/// A program loaded from the network.
class NetworkProgram : public Resource {
    Q_OBJECT

public:

    NetworkProgram(ScriptCache* cache, const QUrl& url);
    
    ScriptCache* getCache() const { return _cache; }
    
    const QScriptProgram& getProgram() const { return _program; }

signals:

    void loaded();

protected:

    virtual void downloadFinished(QNetworkReply* reply);

private:
    
    ScriptCache* _cache;
    QScriptProgram _program;
};

/// Abstract base class of values loaded from the network.
class NetworkValue {
public:
    
    virtual ~NetworkValue();
    
    bool isLoaded() { return getValue().isValid(); }

    virtual QScriptValue& getValue() = 0;

protected:
    
    QScriptValue _value;
};

/// Contains information about a script parameter.
class ParameterInfo {
public:
    QScriptString name;
    int type;
};

/// The direct result of running a program.
class RootNetworkValue : public NetworkValue {
public:
    
    RootNetworkValue(const QSharedPointer<NetworkProgram>& program);

    const QSharedPointer<NetworkProgram>& getProgram() const { return _program; }

    virtual QScriptValue& getValue();

    const QList<ParameterInfo>& getParameterInfo();

private:
    
    QSharedPointer<NetworkProgram> _program;
    QList<ParameterInfo> _parameterInfo;
};

/// The result of running a program's generator using a set of arguments.
class DerivedNetworkValue : public NetworkValue {
public:

    DerivedNetworkValue(const QSharedPointer<NetworkValue>& baseValue, const ScriptHash& parameters);

    virtual QScriptValue& getValue();
    
private:
    
    QSharedPointer<NetworkValue> _baseValue;
    ScriptHash _parameters;
};

#endif // hifi_ScriptCache_h
