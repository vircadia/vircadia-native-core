//
//  Created by Sam Gondelman 10/20/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PickScriptingInterface_h
#define hifi_PickScriptingInterface_h

#include <QtCore/QObject>

#include <RegisteredMetaTypes.h>
#include <DependencyManager.h>
#include <pointers/Pick.h>

class PickScriptingInterface : public QObject, public Dependency {
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
    QUuid createRayPick(const QVariant& properties);

public slots:
    Q_INVOKABLE QUuid createPick(const PickQuery::PickType type, const QVariant& properties);
    Q_INVOKABLE void enablePick(const QUuid& uid);
    Q_INVOKABLE void disablePick(const QUuid& uid);
    Q_INVOKABLE void removePick(const QUuid& uid);
    Q_INVOKABLE QVariantMap getPrevPickResult(const QUuid& uid);

    Q_INVOKABLE void setPrecisionPicking(const QUuid& uid, const bool precisionPicking);
    Q_INVOKABLE void setIgnoreItems(const QUuid& uid, const QScriptValue& ignoreEntities);
    Q_INVOKABLE void setIncludeItems(const QUuid& uid, const QScriptValue& includeEntities);

    static constexpr unsigned int PICK_NOTHING() { return 0; }
    static constexpr unsigned int PICK_ENTITIES() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_ENTITIES); }
    static constexpr unsigned int PICK_OVERLAYS() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_OVERLAYS); }
    static constexpr unsigned int PICK_AVATARS() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_AVATARS); }
    static constexpr unsigned int PICK_HUD() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_HUD); }
    static constexpr unsigned int PICK_COARSE() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_COARSE); }
    static constexpr unsigned int PICK_INCLUDE_INVISIBLE() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_INCLUDE_INVISIBLE); }
    static constexpr unsigned int PICK_INCLUDE_NONCOLLIDABLE() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_INCLUDE_NONCOLLIDABLE); }
    static constexpr unsigned int PICK_ALL_INTERSECTIONS() { return PickFilter::getBitMask(PickFilter::FlagBit::PICK_ALL_INTERSECTIONS); }
    static constexpr unsigned int INTERSECTED_NONE() { return IntersectionType::NONE; }
    static constexpr unsigned int INTERSECTED_ENTITY() { return IntersectionType::ENTITY; }
    static constexpr unsigned int INTERSECTED_OVERLAY() { return IntersectionType::OVERLAY; }
    static constexpr unsigned int INTERSECTED_AVATAR() { return IntersectionType::AVATAR; }
    static constexpr unsigned int INTERSECTED_HUD() { return IntersectionType::HUD; }
};

#endif // hifi_PickScriptingInterface_h