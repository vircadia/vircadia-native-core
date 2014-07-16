//
//  FaceshiftScriptingInterface.h
//  interface/src/scripting
//
//  Created by Ben Arnold on 7/38/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FaceshiftScriptingInterface_h
#define hifi_FaceshiftScriptingInterface_h

#include <QDebug>
#include <QObject>
#include <QString>

class FaceshiftScriptingInterface : public QObject {
    Q_OBJECT
    FaceshiftScriptingInterface() { };
public:
    static FaceshiftScriptingInterface* getInstance();

    public slots:
  
};

#endif // hifi_FaceshiftScriptingInterface_h
