//
//  EntitiesScriptEngineProvider.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on Sept. 18, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// TODO: How will we handle collision callbacks with Entities
//

#ifndef hifi_EntitiesScriptEngineProvider_h
#define hifi_EntitiesScriptEngineProvider_h

#include <QtCore/QString>
#include "EntityItemID.h"

class EntitiesScriptEngineProvider {
public:
    virtual void callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName, const QStringList& params = QStringList()) = 0;
};

#endif // hifi_EntitiesScriptEngineProvider_h