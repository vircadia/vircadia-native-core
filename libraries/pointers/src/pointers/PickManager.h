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

    unsigned int addPick(PickQuery::PickType type, const std::shared_ptr<PickQuery> pick);
    void removePick(unsigned int uid);
    void enablePick(unsigned int uid) const;
    void disablePick(unsigned int uid) const;

    QVariantMap getPrevPickResult(unsigned int uid) const;

    void setPrecisionPicking(unsigned int uid, bool precisionPicking) const;
    void setIgnoreItems(unsigned int uid, const QVector<QUuid>& ignore) const;
    void setIncludeItems(unsigned int uid, const QVector<QUuid>& include) const;

    void setShouldPickHUDOperator(std::function<bool()> shouldPickHUDOperator) { _shouldPickHUDOperator = shouldPickHUDOperator; }
    void setCalculatePos2DFromHUDOperator(std::function<glm::vec2(const glm::vec3&)> calculatePos2DFromHUDOperator) { _calculatePos2DFromHUDOperator = calculatePos2DFromHUDOperator; }
    glm::vec2 calculatePos2DFromHUD(const glm::vec3& intersection) { return _calculatePos2DFromHUDOperator(intersection); }

protected:
    std::function<bool()> _shouldPickHUDOperator;
    std::function<glm::vec2(const glm::vec3&)> _calculatePos2DFromHUDOperator;

    std::shared_ptr<PickQuery> findPick(unsigned int uid) const;
    QHash<PickQuery::PickType, QHash<unsigned int, std::shared_ptr<PickQuery>>> _picks;
    QHash<unsigned int, PickQuery::PickType> _typeMap;
    // 0 = invalid
    const unsigned int FIRST_PICK_ID { 1 };
    unsigned int _nextPickID { FIRST_PICK_ID };

    PickCacheOptimizer<PickRay> _rayPickCacheOptimizer;
};

#endif // hifi_PickManager_h