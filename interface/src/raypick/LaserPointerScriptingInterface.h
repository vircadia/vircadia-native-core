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

#include "RegisteredMetaTypes.h"
#include "DependencyManager.h"
#include "Application.h"

class LaserPointerScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public slots:
    Q_INVOKABLE QUuid createLaserPointer(const QVariant& properties);
    Q_INVOKABLE void enableLaserPointer(QUuid uid) { qApp->getLaserPointerManager().enableLaserPointer(uid); }
    Q_INVOKABLE void disableLaserPointer(QUuid uid) { qApp->getLaserPointerManager().disableLaserPointer(uid); }
    Q_INVOKABLE void removeLaserPointer(QUuid uid) { qApp->getLaserPointerManager().removeLaserPointer(uid); }
    Q_INVOKABLE void editRenderState(QUuid uid, const QString& renderState, const QVariant& properties);
    Q_INVOKABLE void setRenderState(QUuid uid, const QString& renderState) { qApp->getLaserPointerManager().setRenderState(uid, renderState.toStdString()); }
    Q_INVOKABLE RayPickResult getPrevRayPickResult(QUuid uid) { return qApp->getLaserPointerManager().getPrevRayPickResult(uid); }

    Q_INVOKABLE void setPrecisionPicking(QUuid uid, const bool precisionPicking) { qApp->getLaserPointerManager().setPrecisionPicking(uid, precisionPicking); }
    Q_INVOKABLE void setIgnoreEntities(QUuid uid, const QScriptValue& ignoreEntities) { qApp->getLaserPointerManager().setIgnoreEntities(uid, ignoreEntities); }
    Q_INVOKABLE void setIncludeEntities(QUuid uid, const QScriptValue& includeEntities) { qApp->getLaserPointerManager().setIncludeEntities(uid, includeEntities); }
    Q_INVOKABLE void setIgnoreOverlays(QUuid uid, const QScriptValue& ignoreOverlays) { qApp->getLaserPointerManager().setIgnoreOverlays(uid, ignoreOverlays); }
    Q_INVOKABLE void setIncludeOverlays(QUuid uid, const QScriptValue& includeOverlays) { qApp->getLaserPointerManager().setIncludeOverlays(uid, includeOverlays); }
    Q_INVOKABLE void setIgnoreAvatars(QUuid uid, const QScriptValue& ignoreAvatars) { qApp->getLaserPointerManager().setIgnoreAvatars(uid, ignoreAvatars); }
    Q_INVOKABLE void setIncludeAvatars(QUuid uid, const QScriptValue& includeAvatars) { qApp->getLaserPointerManager().setIncludeAvatars(uid, includeAvatars); }

    Q_INVOKABLE void setLockEndUUID(QUuid uid, QUuid objectID, const bool isOverlay) { qApp->getLaserPointerManager().setLockEndUUID(uid, objectID, isOverlay); }

private:
    const RenderState buildRenderState(const QVariantMap& propMap);

};

#endif // hifi_LaserPointerScriptingInterface_h
