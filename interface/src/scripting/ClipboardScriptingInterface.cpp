//
//  ClipboardScriptingInterface.cpp
//  interface/src/scripting
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "ClipboardScriptingInterface.h"

ClipboardScriptingInterface::ClipboardScriptingInterface() {
}

glm::vec3 ClipboardScriptingInterface::getContentsDimensions() {
    return qApp->getEntityClipboard()->getContentsDimensions();
}

float ClipboardScriptingInterface::getClipboardContentsLargestDimension() {
    return qApp->getEntityClipboard()->getContentsLargestDimension();
}

bool ClipboardScriptingInterface::exportEntities(const QString& filename, const QVector<EntityItemID>& entityIDs) {
    bool retVal;
    QMetaObject::invokeMethod(qApp, "exportEntities", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, retVal),
                              Q_ARG(const QString&, filename),
                              Q_ARG(const QVector<EntityItemID>&, entityIDs));
    return retVal;
}

bool ClipboardScriptingInterface::exportEntities(const QString& filename, float x, float y, float z, float s) {
    bool retVal;
    QMetaObject::invokeMethod(qApp, "exportEntities", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, retVal),
                              Q_ARG(const QString&, filename),
                              Q_ARG(float, x),
                              Q_ARG(float, y),
                              Q_ARG(float, z),
                              Q_ARG(float, s));
    return retVal;
}

bool ClipboardScriptingInterface::importEntities(const QString& filename) {
    bool retVal;
    QMetaObject::invokeMethod(qApp, "importEntities", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, retVal),
                              Q_ARG(const QString&, filename));
    return retVal;
}

QVector<EntityItemID> ClipboardScriptingInterface::pasteEntities(glm::vec3 position) {
    QVector<EntityItemID> retVal;
    QMetaObject::invokeMethod(qApp, "pasteEntities", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVector<EntityItemID>, retVal),
                              Q_ARG(float, position.x),
                              Q_ARG(float, position.y),
                              Q_ARG(float, position.z));
    return retVal;
}
