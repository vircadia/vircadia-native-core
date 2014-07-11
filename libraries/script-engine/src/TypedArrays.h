//
//  TypedArrays.h
//
//
//  Created by Clement on 7/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TypedArrays_h
#define hifi_TypedArrays_h

#include "ArrayBufferViewClass.h"

static const QString BYTES_PER_ELEMENT_PROPERTY_NAME = "BYTES_PER_ELEMENT";
static const QString LENGTH_PROPERTY_NAME = "length";

static const QString INT_8_ARRAY_CLASS_NAME = "Int8Array";
static const QString UINT_8_ARRAY_CLASS_NAME = "Uint8Array";
static const QString INT_16_ARRAY_CLASS_NAME = "Int16Array";
static const QString UINT_16_ARRAY_CLASS_NAME = "Uint16Array";
static const QString INT_32_ARRAY_CLASS_NAME = "Int32Array";
static const QString UINT_32_ARRAY_CLASS_NAME = "Uint32Array";
static const QString FLOAT_32_ARRAY_CLASS_NAME = "Float32Array";
static const QString FLOAT_64_ARRAY_CLASS_NAME = "Float64Array";

class TypedArray : public ArrayBufferViewClass {
    Q_OBJECT
public:
    TypedArray(ScriptEngine* scriptEngine, QString name);
    virtual QScriptValue newInstance(quint32 length);
    virtual QScriptValue newInstance(QScriptValue array);
    virtual QScriptValue newInstance(QScriptValue buffer, quint32 byteOffset, quint32 length);
    
    virtual QueryFlags queryProperty(const QScriptValue& object,
                             const QScriptString& name,
                             QueryFlags flags, uint* id);
    virtual QScriptValue property(const QScriptValue& object,
                                  const QScriptString& name, uint id);
    virtual void setProperty(QScriptValue& object, const QScriptString& name, uint id, const QScriptValue& value) = 0;
    virtual QScriptValue::PropertyFlags propertyFlags(const QScriptValue& object,
                                                      const QScriptString& name, uint id);
    
    QString name() const;
    QScriptValue prototype() const;
    
protected:
    static QScriptValue construct(QScriptContext* context, QScriptEngine* engine);
    
    void setBytesPerElement(quint32 bytesPerElement);
    
    QScriptValue _proto;
    QScriptValue _ctor;
    
    QScriptString _name;
    QScriptString _bytesPerElementName;
    QScriptString _lengthName;
    
    quint32 _bytesPerElement;
    
    friend class TypedArrayPrototype;
};

class TypedArrayPrototype : public QObject, public QScriptable {
    Q_OBJECT
public:
    TypedArrayPrototype(QObject* parent = NULL);
    
public slots:
    void set(QScriptValue array, quint32 offset = 0);
    QScriptValue subarray(qint32 begin);
    QScriptValue subarray(qint32 begin, qint32 end);
    
    QScriptValue get(quint32 index);
    void set(quint32 index, QScriptValue& value);
private:
    QByteArray* thisArrayBuffer() const;
};

class Int8ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Int8ArrayClass(ScriptEngine* scriptEngine);
    
    QScriptValue property(const QScriptValue &object, const QScriptString &name, uint id);
    void setProperty(QScriptValue &object, const QScriptString &name, uint id, const QScriptValue &value);
};

class Uint8ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Uint8ArrayClass(ScriptEngine* scriptEngine);
    
    QScriptValue property(const QScriptValue &object, const QScriptString &name, uint id);
    void setProperty(QScriptValue &object, const QScriptString &name, uint id, const QScriptValue &value);
};

class Uint8ClampedArrayClass : public TypedArray {
    // TODO
};

class Int16ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Int16ArrayClass(ScriptEngine* scriptEngine);
    
    QScriptValue property(const QScriptValue &object, const QScriptString &name, uint id);
    void setProperty(QScriptValue &object, const QScriptString &name, uint id, const QScriptValue &value);
};

class Uint16ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Uint16ArrayClass(ScriptEngine* scriptEngine);
    
    QScriptValue property(const QScriptValue &object, const QScriptString &name, uint id);
    void setProperty(QScriptValue &object, const QScriptString &name, uint id, const QScriptValue &value);};

class Int32ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Int32ArrayClass(ScriptEngine* scriptEngine);
    
    QScriptValue property(const QScriptValue &object, const QScriptString &name, uint id);
    void setProperty(QScriptValue &object, const QScriptString &name, uint id, const QScriptValue &value);
};

class Uint32ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Uint32ArrayClass(ScriptEngine* scriptEngine);
    
    QScriptValue property(const QScriptValue &object, const QScriptString &name, uint id);
    void setProperty(QScriptValue &object, const QScriptString &name, uint id, const QScriptValue &value);
};

class Float32ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Float32ArrayClass(ScriptEngine* scriptEngine);
    
    QScriptValue property(const QScriptValue &object, const QScriptString &name, uint id);
    void setProperty(QScriptValue &object, const QScriptString &name, uint id, const QScriptValue &value);};

class Float64ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Float64ArrayClass(ScriptEngine* scriptEngine);
    
    QScriptValue property(const QScriptValue &object, const QScriptString &name, uint id);
    void setProperty(QScriptValue &object, const QScriptString &name, uint id, const QScriptValue &value);
};

#endif // hifi_TypedArrays_h