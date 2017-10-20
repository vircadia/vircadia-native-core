//
//  Created by Sam Gondelman 10/16/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PickManager_h
#define hifi_PickManager_h

#include <DependencyManager.h>
#include "RegisteredMetaTypes.h"

#include "Pick.h"
#include "PickCacheOptimizer.h"

class PickManager : public Dependency, protected ReadWriteLockable {
    SINGLETON_DEPENDENCY

public:
    PickManager();

    void update();

    QUuid addPick(PickQuery::PickType type, const std::shared_ptr<PickQuery> pick);
    void removePick(const QUuid& uid);
    void enablePick(const QUuid& uid) const;
    void disablePick(const QUuid& uid) const;

    QVariantMap getPrevPickResult(const QUuid& uid) const;

    void setPrecisionPicking(const QUuid& uid, bool precisionPicking) const;
    void setIgnoreItems(const QUuid& uid, const QVector<QUuid>& ignore) const;
    void setIncludeItems(const QUuid& uid, const QVector<QUuid>& include) const;

    void setShouldPickHUDOperator(std::function<bool()> shouldPickHUDOperator) { _shouldPickHUDOperator = shouldPickHUDOperator; }

protected:
    std::function<bool()> _shouldPickHUDOperator;

    std::shared_ptr<PickQuery> findPick(const QUuid& uid) const;
    QHash<PickQuery::PickType, QHash<QUuid, std::shared_ptr<PickQuery>>> _picks;
    QHash<QUuid, PickQuery::PickType> _typeMap;

    PickCacheOptimizer<PickRay> _rayPickCacheOptimizer;
};

#endif // hifi_PickManager_h