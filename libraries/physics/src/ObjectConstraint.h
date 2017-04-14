//
//  ObjectConstraint.h
//  libraries/physcis/src
//
//  Created by Seth Alves 2017-4-11
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  http://bulletphysics.org/Bullet/BulletFull/classbtConstraintInterface.html

#ifndef hifi_ObjectConstraint_h
#define hifi_ObjectConstraint_h

#include <QUuid>
#include <btBulletDynamicsCommon.h>
#include "ObjectDynamic.h"

class ObjectConstraint : public ObjectDynamic
{
public:
    ObjectConstraint(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectConstraint() {}

    virtual btTypedConstraint* getConstraint() = 0;

    virtual bool isConstraint() const override { return true; }
    virtual void invalidate() override;

protected:
    btTypedConstraint* _constraint { nullptr };
};

#endif // hifi_ObjectConstraint_h
