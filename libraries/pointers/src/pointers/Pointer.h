//
//  Created by Sam Gondelman 10/17/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_Pointer_h
#define hifi_Pointer_h

#include <QtCore/QUuid>
#include <QVector>
#include <QVariant>

#include <shared/ReadWriteLockable.h>

class Pointer : protected ReadWriteLockable {
public:
    Pointer(const QUuid& uid) : _pickUID(uid) {}

    virtual ~Pointer();

    virtual void enable();
    virtual void disable();
    virtual const QVariantMap getPrevPickResult();

    virtual void setRenderState(const std::string& state) = 0;
    virtual void editRenderState(const std::string& state, const QVariant& startProps, const QVariant& pathProps, const QVariant& endProps) = 0;
    
    virtual void setPrecisionPicking(const bool precisionPicking);
    virtual void setIgnoreItems(const QVector<QUuid>& ignoreItems) const;
    virtual void setIncludeItems(const QVector<QUuid>& includeItems) const;

    // Pointers can choose to implement these
    virtual void setLength(const float length) {}
    virtual void setLockEndUUID(QUuid objectID, const bool isOverlay) {}

    virtual void update() = 0;

    QUuid getRayUID() { return _pickUID; }

protected:
    const QUuid _pickUID;
};

#endif // hifi_Pick_h
