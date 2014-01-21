//
//  DataServerScriptingInterface.h
//  hifi
//
//  Created by Stephen Birarda on 1/20/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__DataServerScriptingInterface__
#define __hifi__DataServerScriptingInterface__

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtScript/QScriptValue>

class DataServerScriptingInterface : public QObject {
    Q_OBJECT
public:
    DataServerScriptingInterface();
    
    void refreshUUID() { _uuid = QUuid::createUuid(); }
    const QUuid& getUUID() const { return _uuid; }
    
public slots:
    void setValueForKey(const QString& key, const QString& value);
private:
    QUuid _uuid;
};

#endif /* defined(__hifi__DataServerScriptingInterface__) */
