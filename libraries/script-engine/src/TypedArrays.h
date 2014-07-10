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

class TypedArray : public ArrayBufferViewClass {
    Q_OBJECT
public:
    TypedArray(ScriptEngine* scriptEngine, QString name);
    virtual QScriptValue newInstance(quint32 length) = 0;
    virtual QScriptValue newInstance(QScriptValue array, bool isTypedArray) = 0;
    virtual QScriptValue newInstance(QScriptValue buffer, quint32 byteOffset, quint32 length) = 0;
    
    virtual QueryFlags queryProperty(const QScriptValue& object,
                             const QScriptString& name,
                             QueryFlags flags, uint* id);
    virtual QScriptValue property(const QScriptValue &object,
                                  const QScriptString &name, uint id);
    virtual QScriptValue::PropertyFlags propertyFlags(const QScriptValue& object,
                                                      const QScriptString& name, uint id);
    
    QString name() const;
    QScriptValue prototype() const;
    
protected:
    static QScriptValue construct(QScriptContext* context, QScriptEngine* engine);
    
    QScriptValue _proto;
    QScriptValue _ctor;
    
    QScriptString _name;
    QScriptString _bytesPerElementName;
    QScriptString _lengthName;
    
    quint32 _bytesPerElement;
};

class Int8ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Int8ArrayClass(ScriptEngine* scriptEngine);
    QScriptValue newInstance(quint32 length);
    QScriptValue newInstance(QScriptValue array, bool isTypedArray);
    QScriptValue newInstance(QScriptValue buffer, quint32 byteOffset, quint32 length);

    QScriptValue property(const QScriptValue &object,
                          const QScriptString &name, uint id);
    void setProperty(QScriptValue &object,
                     const QScriptString &name,
                     uint id, const QScriptValue &value);
};

class Uint8ArrayClass : public TypedArray {
    
};

class Uint8ClampedArrayClass : public TypedArray {
    
};

class Int16ArrayClass : public TypedArray {
    
};

class Uint16ArrayClass : public TypedArray {
    
};

class Int32ArrayClass : public TypedArray {
    
};

class Uint32ArrayClass : public TypedArray {
    
};

class Float32ArrayClass : public TypedArray {
    
};

class Float64ArrayClass : public TypedArray {
    
};

#endif // hifi_TypedArrays_h