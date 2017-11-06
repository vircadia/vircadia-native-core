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

#include "RegisteredMetaTypes.h"
#include <DependencyManager.h>

#include "PickScriptingInterface.h"

class RayPickScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    Q_PROPERTY(unsigned int PICK_NOTHING READ PICK_NOTHING CONSTANT)
    Q_PROPERTY(unsigned int PICK_ENTITIES READ PICK_ENTITIES CONSTANT)
    Q_PROPERTY(unsigned int PICK_OVERLAYS READ PICK_OVERLAYS CONSTANT)
    Q_PROPERTY(unsigned int PICK_AVATARS READ PICK_AVATARS CONSTANT)
    Q_PROPERTY(unsigned int PICK_HUD READ PICK_HUD CONSTANT)
    Q_PROPERTY(unsigned int PICK_COARSE READ PICK_COARSE CONSTANT)
    Q_PROPERTY(unsigned int PICK_INCLUDE_INVISIBLE READ PICK_INCLUDE_INVISIBLE CONSTANT)
    Q_PROPERTY(unsigned int PICK_INCLUDE_NONCOLLIDABLE READ PICK_INCLUDE_NONCOLLIDABLE CONSTANT)
    Q_PROPERTY(unsigned int PICK_ALL_INTERSECTIONS READ PICK_ALL_INTERSECTIONS CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_NONE READ INTERSECTED_NONE CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_ENTITY READ INTERSECTED_ENTITY CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_OVERLAY READ INTERSECTED_OVERLAY CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_AVATAR READ INTERSECTED_AVATAR CONSTANT)
    Q_PROPERTY(unsigned int INTERSECTED_HUD READ INTERSECTED_HUD CONSTANT)
    SINGLETON_DEPENDENCY

public:
    Q_INVOKABLE unsigned int createRayPick(const QVariant& properties);
    Q_INVOKABLE void enableRayPick(unsigned int uid);
    Q_INVOKABLE void disableRayPick(unsigned int uid);
    Q_INVOKABLE void removeRayPick(unsigned int uid);
    Q_INVOKABLE QVariantMap getPrevRayPickResult(unsigned int uid);

    Q_INVOKABLE void setPrecisionPicking(unsigned int uid, bool precisionPicking);
    Q_INVOKABLE void setIgnoreItems(unsigned int uid, const QScriptValue& ignoreEntities);
    Q_INVOKABLE void setIncludeItems(unsigned int uid, const QScriptValue& includeEntities);

    Q_INVOKABLE bool isLeftHand(unsigned int uid);
    Q_INVOKABLE bool isRightHand(unsigned int uid);
    Q_INVOKABLE bool isMouse(unsigned int uid);

public slots:
    static unsigned int PICK_NOTHING() { return PickScriptingInterface::PICK_NOTHING(); }
    static unsigned int PICK_ENTITIES() { return PickScriptingInterface::PICK_ENTITIES(); }
    static unsigned int PICK_OVERLAYS() { return PickScriptingInterface::PICK_OVERLAYS(); }
    static unsigned int PICK_AVATARS() { return PickScriptingInterface::PICK_AVATARS(); }
    static unsigned int PICK_HUD() { return PickScriptingInterface::PICK_HUD(); }
    static unsigned int PICK_COARSE() { return PickScriptingInterface::PICK_COARSE(); }
    static unsigned int PICK_INCLUDE_INVISIBLE() { return PickScriptingInterface::PICK_INCLUDE_INVISIBLE(); }
    static unsigned int PICK_INCLUDE_NONCOLLIDABLE() { return PickScriptingInterface::PICK_INCLUDE_NONCOLLIDABLE(); }
    static unsigned int PICK_ALL_INTERSECTIONS() { return PickScriptingInterface::PICK_ALL_INTERSECTIONS(); }
    static unsigned int INTERSECTED_NONE() { return PickScriptingInterface::INTERSECTED_NONE(); }
    static unsigned int INTERSECTED_ENTITY() { return PickScriptingInterface::INTERSECTED_ENTITY(); }
    static unsigned int INTERSECTED_OVERLAY() { return PickScriptingInterface::INTERSECTED_OVERLAY(); }
    static unsigned int INTERSECTED_AVATAR() { return PickScriptingInterface::INTERSECTED_AVATAR(); }
    static unsigned int INTERSECTED_HUD() { return PickScriptingInterface::INTERSECTED_HUD(); }
};

#endif // hifi_RayPickScriptingInterface_h
