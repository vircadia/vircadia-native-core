//
//  ArrayBufferPrototype.h
//
//
//  Created by Clement on 7/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ArrayBufferPrototype_h
#define hifi_ArrayBufferPrototype_h

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtScript/QScriptable>
#include <QtScript/QScriptValue>

class ArrayBufferPrototype : public QObject, public QScriptable {
    Q_OBJECT
public:
    ArrayBufferPrototype(QObject* parent = NULL);
    
public slots:
    QByteArray slice(long begin, long end = -1) const;
    
private:
    QByteArray* thisArrayBuffer() const;
};

#endif // hifi_ArrayBufferPrototype_h