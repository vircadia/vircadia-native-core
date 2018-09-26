//
//  EntityItemWeakPointerWithUrlAncestry.h
//  libraries/shared/src/
//
//  Created by Kerry Ivan Kurian 10/15/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef  hifi_EntityItemWeakPointerWithUrlAncestry_h
#define  hifi_EntityItemWeakPointerWithUrlAncestry_h

#include "EntityTypes.h"
#include "QUrlAncestry.h"


struct EntityItemWeakPointerWithUrlAncestry {
    EntityItemWeakPointerWithUrlAncestry(
        const EntityItemWeakPointer& a,
        const QUrlAncestry& b
    ) : entityItemWeakPointer(a), urlAncestry(b) {}

    EntityItemWeakPointer entityItemWeakPointer;
    QUrlAncestry urlAncestry;
};


#endif  // hifi_EntityItemWeakPointerWithUrlAncestry_h

