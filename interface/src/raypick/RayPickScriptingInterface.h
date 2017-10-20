//
//  RayPickScriptingInterface.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 8/15/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RayPickScriptingInterface_h
#define hifi_RayPickScriptingInterface_h

#include <QtCore/QObject>

#include <RegisteredMetaTypes.h>
#include <DependencyManager.h>
#include "RayPick.h"

#include "PickScriptingInterface.h"

class RayPickScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public slots:
    Q_INVOKABLE QUuid createRayPick(const QVariant& properties);
    Q_INVOKABLE void enableRayPick(const QUuid& uid);
    Q_INVOKABLE void disableRayPick(const QUuid& uid);
    Q_INVOKABLE void removeRayPick(const QUuid& uid);
    Q_INVOKABLE QVariantMap getPrevRayPickResult(const QUuid& uid);

    Q_INVOKABLE void setPrecisionPicking(const QUuid& uid, const bool precisionPicking);
    Q_INVOKABLE void setIgnoreItems(const QUuid& uid, const QScriptValue& ignoreEntities);
    Q_INVOKABLE void setIncludeItems(const QUuid& uid, const QScriptValue& includeEntities);
};

#endif // hifi_RayPickScriptingInterface_h
