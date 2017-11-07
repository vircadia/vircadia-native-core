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
    Q_INVOKABLE QUuid createLaserPointer(const QVariant& properties) const;
    Q_INVOKABLE void enableLaserPointer(const QUuid& uid) const { qApp->getLaserPointerManager().enableLaserPointer(uid); }
    Q_INVOKABLE void disableLaserPointer(const QUuid& uid) const { qApp->getLaserPointerManager().disableLaserPointer(uid); }
    Q_INVOKABLE void removeLaserPointer(const QUuid& uid) const { qApp->getLaserPointerManager().removeLaserPointer(uid); }
    Q_INVOKABLE void editRenderState(const QUuid& uid, const QString& renderState, const QVariant& properties) const;
    Q_INVOKABLE void setRenderState(const QUuid& uid, const QString& renderState) const { qApp->getLaserPointerManager().setRenderState(uid, renderState.toStdString()); }
    Q_INVOKABLE RayPickResult getPrevRayPickResult(QUuid uid) const { return qApp->getLaserPointerManager().getPrevRayPickResult(uid); }

    Q_INVOKABLE void setPrecisionPicking(const QUuid& uid, bool precisionPicking) const { qApp->getLaserPointerManager().setPrecisionPicking(uid, precisionPicking); }
    Q_INVOKABLE void setLaserLength(const QUuid& uid, float laserLength) const { qApp->getLaserPointerManager().setLaserLength(uid, laserLength); }
    Q_INVOKABLE void setIgnoreItems(const QUuid& uid, const QScriptValue& ignoreEntities) const;
    Q_INVOKABLE void setIncludeItems(const QUuid& uid, const QScriptValue& includeEntities) const;

    Q_INVOKABLE void setLockEndUUID(const QUuid& uid, const QUuid& objectID, bool isOverlay) const { qApp->getLaserPointerManager().setLockEndUUID(uid, objectID, isOverlay); }

private:
    static RenderState buildRenderState(const QVariantMap& propMap);

};

#endif // hifi_LaserPointerScriptingInterface_h
