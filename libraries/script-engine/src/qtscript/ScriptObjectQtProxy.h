//
//  ScriptObjectQtProxy.h
//  libraries/script-engine/src/qtscript
//
//  Created by Heather Anderson on 12/5/21.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_ScriptObjectQtProxy_h
#define hifi_ScriptObjectQtProxy_h

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include <QtScript/QScriptable>
#include <QtScript/QScriptClass>
#include <QtScript/QScriptString>
#include <QtScript/QScriptValue>

#include "../ScriptEngine.h"
#include "ScriptEngineQtScript.h"

class ScriptEngineQtScript;
class ScriptSignalQtProxy;

/// [QtScript] (re-)implements the translation layer between ScriptValue and QObject.  This object
/// will focus exclusively on property get/set until function calls appear to be a problem
class ScriptObjectQtProxy final : public QScriptClass {
private:  // implementation
    struct PropertyDef {
        QScriptString name;
        QScriptValue::PropertyFlags flags;
    };
    struct MethodDef {
        QScriptString name;
        QList<QMetaMethod> methods;
    };
    struct SignalDef {
        QScriptString name;
        QMetaMethod signal;
    };
    using PropertyDefMap = QHash<uint, PropertyDef>;
    using MethodDefMap = QHash<uint, MethodDef>;
    using SignalDefMap = QHash<uint, SignalDef>;
    using InstanceMap = QHash<uint, QPointer<ScriptSignalQtProxy> >;

    static constexpr uint PROPERTY_TYPE = 0x1000;
    static constexpr uint METHOD_TYPE = 0x2000;
    static constexpr uint SIGNAL_TYPE = 0x3000;
    static constexpr uint TYPE_MASK = 0xF000;

public:  // construction
    inline ScriptObjectQtProxy(ScriptEngineQtScript* engine, QObject* object, bool ownsObject, const ScriptEngine::QObjectWrapOptions& options) :
        QScriptClass(engine), _engine(engine), _object(object), _wrapOptions(options), _ownsObject(ownsObject) {
        investigate();
    }
    virtual ~ScriptObjectQtProxy();

    static QScriptValue newQObject(ScriptEngineQtScript* engine,
                                   QObject* object,
                                   ScriptEngine::ValueOwnership ownership = ScriptEngine::QtOwnership,
                                   const ScriptEngine::QObjectWrapOptions& options = ScriptEngine::QObjectWrapOptions());
    static ScriptObjectQtProxy* unwrapProxy(const QScriptValue& val);
    static QObject* unwrap(const QScriptValue& val);
    inline QObject* toQtValue() const { return _object; }

public:  // QScriptClass implementation
    virtual QString name() const override;

    virtual QScriptValue property(const QScriptValue& object, const QScriptString& name, uint id) override;
    virtual QScriptValue::PropertyFlags propertyFlags(const QScriptValue& object, const QScriptString& name, uint id) override;
    virtual QueryFlags queryProperty(const QScriptValue& object, const QScriptString& name, QueryFlags flags, uint* id) override;
    virtual void setProperty(QScriptValue& object, const QScriptString& name, uint id, const QScriptValue& value) override;

private:  // implementation
    void investigate();

private:  // storage
    ScriptEngineQtScript* _engine;
    const ScriptEngine::QObjectWrapOptions _wrapOptions;
    PropertyDefMap _props;
    MethodDefMap _methods;
    SignalDefMap _signals;
    InstanceMap _signalInstances;
    const bool _ownsObject;
    QPointer<QObject> _object;

    Q_DISABLE_COPY(ScriptObjectQtProxy)
};

/// [QtScript] (re-)implements the translation layer between ScriptValue and QVariant where a prototype is set.
/// This object depends on a ScriptObjectQtProxy to provide the prototype's behavior
class ScriptVariantQtProxy final : public QScriptClass {
public:  // construction
    ScriptVariantQtProxy(ScriptEngineQtScript* engine, const QVariant& variant, QScriptValue scriptProto, ScriptObjectQtProxy* proto);

    static QScriptValue newVariant(ScriptEngineQtScript* engine, const QVariant& variant, QScriptValue proto);
    static ScriptVariantQtProxy* unwrapProxy(const QScriptValue& val);
    static QVariant unwrap(const QScriptValue& val);
    inline QVariant toQtValue() const { return _variant; }

public:  // QScriptClass implementation
    virtual QString name() const override { return _name; }

    virtual QScriptValue prototype() const override { return _scriptProto; }

    virtual QScriptValue property(const QScriptValue& object, const QScriptString& name, uint id) override {
        return _proto->property(object, name, id);
    }
    virtual QScriptValue::PropertyFlags propertyFlags(const QScriptValue& object, const QScriptString& name, uint id) override {
        return _proto->propertyFlags(object, name, id);
    }
    virtual QueryFlags queryProperty(const QScriptValue& object, const QScriptString& name, QueryFlags flags, uint* id) override {
        return _proto->queryProperty(object, name, flags, id);
    }
    virtual void setProperty(QScriptValue& object, const QScriptString& name, uint id, const QScriptValue& value) override {
        return _proto->setProperty(object, name, id, value);
    }

private:  // storage
    ScriptEngineQtScript* _engine;
    QVariant _variant;
    QScriptValue _scriptProto;
    ScriptObjectQtProxy* _proto;
    QString _name;

    Q_DISABLE_COPY(ScriptVariantQtProxy)
};

class ScriptMethodQtProxy final : public QScriptClass {
public:  // construction
    inline ScriptMethodQtProxy(ScriptEngineQtScript* engine, QObject* object, QScriptValue lifetime, const QList<QMetaMethod>& metas) :
        QScriptClass(engine), _engine(engine), _object(object), _objectLifetime(lifetime), _metas(metas) {}

public:  // QScriptClass implementation
    virtual QString name() const override { return fullName(); }
    virtual bool supportsExtension(Extension extension) const override;
    virtual QVariant extension(Extension extension, const QVariant& argument = QVariant()) override;

private:
    QString fullName() const;

private:  // storage
    ScriptEngineQtScript* _engine;
    QPointer<QObject> _object;
    QScriptValue _objectLifetime;
    const QList<QMetaMethod> _metas;

    Q_DISABLE_COPY(ScriptMethodQtProxy)
};

// This abstract base class serves solely to declare the Q_INVOKABLE methods for ScriptSignalQtProxy
// as we're overriding qt_metacall later for the signal callback yet still want to support
// metacalls for the connect/disconnect API
class ScriptSignalQtProxyBase : public QObject, protected QScriptable {
    Q_OBJECT
public:  // API
    Q_INVOKABLE virtual void connect(QScriptValue arg0, QScriptValue arg1 = QScriptValue()) = 0;
    Q_INVOKABLE virtual void disconnect(QScriptValue arg0, QScriptValue arg1 = QScriptValue()) = 0;
};

class ScriptSignalQtProxy final : public ScriptSignalQtProxyBase {
private:  // storage
    struct Connection {
        QScriptValue thisValue;
        QScriptValue callback;
    };
    using ConnectionList = QList<Connection>;

public:  // construction
    inline ScriptSignalQtProxy(ScriptEngineQtScript* engine, QObject* object, QScriptValue lifetime, const QMetaMethod& meta) :
        _engine(engine), _object(object), _objectLifetime(lifetime), _meta(meta), _metaCallId(discoverMetaCallIdx()) {}

private:  // implementation
    virtual int qt_metacall(QMetaObject::Call call, int id, void** arguments);
    int discoverMetaCallIdx();
    ConnectionList::iterator findConnection(QScriptValue thisObject, QScriptValue callback);
    QString fullName() const;

public:  // API
    virtual void connect(QScriptValue arg0, QScriptValue arg1 = QScriptValue()) override;
    virtual void disconnect(QScriptValue arg0, QScriptValue arg1 = QScriptValue()) override;

private:  // storage
    ScriptEngineQtScript* _engine;
    QPointer<QObject> _object;
    QScriptValue _objectLifetime;
    const QMetaMethod _meta;
    const int _metaCallId;
    ConnectionList _connections;
    bool _isConnected{ false };

    Q_DISABLE_COPY(ScriptSignalQtProxy)
};

#endif  // hifi_ScriptObjectQtProxy_h

/// @}
