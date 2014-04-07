//
//  ScriptUUID.h
//  hifi
//
//  Created by Andrew Meadows on 2014.04.07
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__ScriptUUID__
#define __hifi__ScriptUUID__

#include <QUuid>

/// Scriptable interface a UUID helper class object. Used exclusively in the JavaScript API
class ScriptUUID : public QObject {
    Q_OBJECT

public slots:
    QUuid fromString(const QString& string);
    QString toString(const QUuid& id);
    QUuid generate();
    bool isEqual(const QUuid& idA, const QUuid& idB);
    bool isNull(const QUuid& id);
    void print(const QString& lable, const QUuid& id);
};



#endif /* defined(__hifi__Vec3__) */
