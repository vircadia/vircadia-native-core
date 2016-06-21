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
    glm::vec3 getContentsDimensions(); /// returns the overall dimensions of everything on the blipboard
    float getClipboardContentsLargestDimension(); /// returns the largest dimension of everything on the clipboard
    bool importEntities(const QString& filename);
    bool exportEntities(const QString& filename, const QVector<EntityItemID>& entityIDs);
    bool exportEntities(const QString& filename, float x, float y, float z, float s);
    QVector<EntityItemID> pasteEntities(glm::vec3 position);
};

#endif // hifi_ClipboardScriptingInterface_h
