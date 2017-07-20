//
//  LaserPointerScriptingInterface.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_LaserPointerScriptingInterface_h
#define hifi_LaserPointerScriptingInterface_h

#include <QtCore/QObject>

#include "LaserPointerManager.h"
#include "RegisteredMetaTypes.h"
#include "DependencyManager.h"

class LaserPointerScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public slots:
    Q_INVOKABLE unsigned int createLaserPointer(const QVariant& properties);
    Q_INVOKABLE void enableLaserPointer(unsigned int uid) { DependencyManager::get<LaserPointerManager>()->enableLaserPointer(uid); }
    Q_INVOKABLE void disableLaserPointer(unsigned int uid) { DependencyManager::get<LaserPointerManager>()->disableLaserPointer(uid); }
    Q_INVOKABLE void removeLaserPointer(unsigned int uid) { DependencyManager::get<LaserPointerManager>()->removeLaserPointer(uid); }
    Q_INVOKABLE void editRenderState(unsigned int uid, const QString& renderState, const QVariant& properties);
    Q_INVOKABLE void setRenderState(unsigned int uid, const QString& renderState) { DependencyManager::get<LaserPointerManager>()->setRenderState(uid, renderState); }
    Q_INVOKABLE RayPickResult getPrevRayPickResult(unsigned int uid) { return DependencyManager::get<LaserPointerManager>()->getPrevRayPickResult(uid); }

    Q_INVOKABLE void setIgnoreEntities(unsigned int uid, const QScriptValue& ignoreEntities) { DependencyManager::get<LaserPointerManager>()->setIgnoreEntities(uid, ignoreEntities); }
    Q_INVOKABLE void setIncludeEntities(unsigned int uid, const QScriptValue& includeEntities) { DependencyManager::get<LaserPointerManager>()->setIncludeEntities(uid, includeEntities); }
    Q_INVOKABLE void setIgnoreOverlays(unsigned int uid, const QScriptValue& ignoreOverlays) { DependencyManager::get<LaserPointerManager>()->setIgnoreOverlays(uid, ignoreOverlays); }
    Q_INVOKABLE void setIncludeOverlays(unsigned int uid, const QScriptValue& includeOverlays) { DependencyManager::get<LaserPointerManager>()->setIncludeOverlays(uid, includeOverlays); }
    Q_INVOKABLE void setIgnoreAvatars(unsigned int uid, const QScriptValue& ignoreAvatars) { DependencyManager::get<LaserPointerManager>()->setIgnoreAvatars(uid, ignoreAvatars); }
    Q_INVOKABLE void setIncludeAvatars(unsigned int uid, const QScriptValue& includeAvatars) { DependencyManager::get<LaserPointerManager>()->setIncludeAvatars(uid, includeAvatars); }

private:
    const RenderState buildRenderState(const QVariantMap & propMap);

};

#endif // hifi_LaserPointerScriptingInterface_h
