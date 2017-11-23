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
#include <pointers/rays/RayPick.h>

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

public slots:
    Q_INVOKABLE QUuid createRayPick(const QVariant& properties);
    Q_INVOKABLE void enableRayPick(const QUuid& uid);
    Q_INVOKABLE void disableRayPick(const QUuid& uid);
    Q_INVOKABLE void removeRayPick(const QUuid& uid);
    Q_INVOKABLE RayPickResult getPrevRayPickResult(const QUuid& uid);

    Q_INVOKABLE void setPrecisionPicking(const QUuid& uid, const bool precisionPicking);
    Q_INVOKABLE void setIgnoreItems(const QUuid& uid, const QScriptValue& ignoreEntities);
    Q_INVOKABLE void setIncludeItems(const QUuid& uid, const QScriptValue& includeEntities);

    unsigned int PICK_NOTHING() { return 0; }
    unsigned int PICK_ENTITIES() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_ENTITIES); }
    unsigned int PICK_OVERLAYS() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_OVERLAYS); }
    unsigned int PICK_AVATARS() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_AVATARS); }
    unsigned int PICK_HUD() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_HUD); }
    unsigned int PICK_COARSE() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_COARSE); }
    unsigned int PICK_INCLUDE_INVISIBLE() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_INCLUDE_INVISIBLE); }
    unsigned int PICK_INCLUDE_NONCOLLIDABLE() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_INCLUDE_NONCOLLIDABLE); }
    unsigned int PICK_ALL_INTERSECTIONS() { return RayPickFilter::getBitMask(RayPickFilter::FlagBit::PICK_ALL_INTERSECTIONS); }
    unsigned int INTERSECTED_NONE() { return IntersectionType::NONE; }
    unsigned int INTERSECTED_ENTITY() { return IntersectionType::ENTITY; }
    unsigned int INTERSECTED_OVERLAY() { return IntersectionType::OVERLAY; }
    unsigned int INTERSECTED_AVATAR() { return IntersectionType::AVATAR; }
    unsigned int INTERSECTED_HUD() { return IntersectionType::HUD; }
};

#endif // hifi_RayPickScriptingInterface_h
