//
//  StandAloneJSConsole.h
//
//
//  Created by Clement on 1/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_StandAloneJSConsole_h
#define hifi_StandAloneJSConsole_h

#include <QPointer>
#include <DependencyManager.h>

#include "JSConsole.h"

class QDialog;

class StandAloneJSConsole : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public slots:
    void toggleConsole();
    
private:
    StandAloneJSConsole() {}
    
    QPointer<QDialog> _jsConsole;
};

#endif // hifi_StandAloneJSConsole_h