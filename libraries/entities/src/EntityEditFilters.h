//
//  EntityEditFilters.h
//  libraries/entities/src
//
//  Created by David Kelly on 2/7/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_EntityEditFilters_h
#define hifi_EntityEditFilters_h

#include <QObject>
#include <QMap>
#include <QScriptValue>
#include <QScriptEngine>
#include <glm/glm.hpp>

#include <functional>

#include "EntityItemID.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"

typedef QPair<QScriptValue, std::function<bool()>> FilterFunctionPair;

class EntityEditFilters : public QObject, public Dependency {
    Q_OBJECT
public:
    EntityEditFilters() {};
    EntityEditFilters(EntityTreePointer tree ): _tree(tree) {};

    void addFilter(EntityItemID& entityID, QString filterURL);
    void removeFilter(EntityItemID& entityID);

    bool filter(glm::vec3& position, EntityItemProperties& propertiesIn, EntityItemProperties& propertiesOut, bool& wasChanged, EntityTree::FilterType filterType);
    void rejectAll(bool state) {_rejectAll = state; }

signals:
    void filterAdded(EntityItemID id, bool success);

private slots:
    void scriptRequestFinished(EntityItemID entityID);
    
private:
    QList<EntityItemID> getZonesByPosition(glm::vec3& position);

    EntityTreePointer _tree {};
    bool _rejectAll {false};
    QScriptValue _nullObjectForFilter{};
    
    QReadWriteLock _lock;
    QMap<EntityItemID, FilterFunctionPair*> _filterFunctionMap;
    QMap<EntityItemID, QScriptEngine*> _filterScriptEngineMap;
};

#endif //hifi_EntityEditFilters_h
