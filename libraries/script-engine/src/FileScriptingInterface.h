//
//  FileScriptingInterface.h
//  interface/src/scripting
//
//  Created by Elisa Lupin-Jimenez on 6/28/16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FileScriptingInterface_h
#define hifi_FileScriptingInterface_h

#include <QtCore/QObject>

class FileScriptingInterface : public QObject {
    Q_OBJECT

public:
    FileScriptingInterface(QObject* parent);

public slots:
    QScriptValue hasFocus();

};



#endif // hifi_FileScriptingInterface_h