//
//  ClipboardScriptingInterface.h
//  interface/src/scripting
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ClipboardScriptingInterface_h
#define hifi_ClipboardScriptingInterface_h

#include <QObject>

class ClipboardScriptingInterface : public QObject {
    Q_OBJECT
public:
    ClipboardScriptingInterface();

signals:
    void readyToImport();
    
public slots:
    bool importEntities(const QString& filename);
    bool exportEntities(const QString& filename, float x, float y, float z, float s);
    void pasteEntities(float x, float y, float z, float s);
};

#endif // hifi_ClipboardScriptingInterface_h
