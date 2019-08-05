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

#include <NumericalConstants.h>

#include <QObject>

class PickManager : public QObject, public Dependency, protected ReadWriteLockable {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    PickManager();

    void update();

    unsigned int addPick(PickQuery::PickType type, const std::shared_ptr<PickQuery> pick);
    void removePick(unsigned int uid);
    void enablePick(unsigned int uid) const;
    void disablePick(unsigned int uid) const;
    bool isPickEnabled(unsigned int uid) const;
    QVector<unsigned int> getPicks() const;

    PickResultPointer getPrevPickResult(unsigned int uid) const;
    // The actual current properties of the pick
    QVariantMap getPickProperties(unsigned int uid) const;
    // The properties that were passed in to create the pick (may be empty if the pick was created by invoking the constructor)
    QVariantMap getPickScriptParameters(unsigned int uid) const;

    template <typename T>
    std::shared_ptr<T> getPrevPickResultTyped(unsigned int uid) const {
        return std::static_pointer_cast<T>(getPrevPickResult(uid));
    }

    void setPrecisionPicking(unsigned int uid, bool precisionPicking) const;
    void setIgnoreItems(unsigned int uid, const QVector<QUuid>& ignore) const;
    void setIncludeItems(unsigned int uid, const QVector<QUuid>& include) const;

    Transform getParentTransform(unsigned int uid) const;
    Transform getResultTransform(unsigned int uid) const;

    bool isLeftHand(unsigned int uid);
    bool isRightHand(unsigned int uid);
    bool isMouse(unsigned int uid);

    void setShouldPickHUDOperator(std::function<bool()> shouldPickHUDOperator) { _shouldPickHUDOperator = shouldPickHUDOperator; }
    void setCalculatePos2DFromHUDOperator(std::function<glm::vec2(const glm::vec3&)> calculatePos2DFromHUDOperator) { _calculatePos2DFromHUDOperator = calculatePos2DFromHUDOperator; }
    glm::vec2 calculatePos2DFromHUD(const glm::vec3& intersection) { return _calculatePos2DFromHUDOperator(intersection); }

    static const unsigned int INVALID_PICK_ID { 0 };

    unsigned int getPerFrameTimeBudget() const { return _perFrameTimeBudget; }
    void setPerFrameTimeBudget(unsigned int numUsecs) { _perFrameTimeBudget = numUsecs; }

    bool getForceCoarsePicking() { return _forceCoarsePicking; }

    const std::vector<QVector3D>& getUpdatedPickCounts() { return _updatedPickCounts; }
    const std::vector<int>& getTotalPickCounts() { return _totalPickCounts; }

public slots:
    void setForceCoarsePicking(bool forceCoarsePicking) { _forceCoarsePicking = forceCoarsePicking; }

protected:
    std::vector<QVector3D> _updatedPickCounts { PickQuery::NUM_PICK_TYPES };
    std::vector<int> _totalPickCounts { 0, 0, 0, 0 };

    bool _forceCoarsePicking { false };
    std::function<bool()> _shouldPickHUDOperator;
    std::function<glm::vec2(const glm::vec3&)> _calculatePos2DFromHUDOperator;

    std::shared_ptr<PickQuery> findPick(unsigned int uid) const;
    std::unordered_map<PickQuery::PickType, std::unordered_map<unsigned int, std::shared_ptr<PickQuery>>> _picks;
    unsigned int _nextPickToUpdate[PickQuery::NUM_PICK_TYPES] { 0, 0, 0, 0 };
    std::unordered_map<unsigned int, PickQuery::PickType> _typeMap;
    unsigned int _nextPickID { INVALID_PICK_ID + 1 };

    PickCacheOptimizer<PickRay> _rayPickCacheOptimizer;
    PickCacheOptimizer<StylusTip> _stylusPickCacheOptimizer;
    PickCacheOptimizer<PickParabola> _parabolaPickCacheOptimizer;
    PickCacheOptimizer<CollisionRegion> _collisionPickCacheOptimizer;

    static const unsigned int DEFAULT_PER_FRAME_TIME_BUDGET = 3 * USECS_PER_MSEC;
    unsigned int _perFrameTimeBudget { DEFAULT_PER_FRAME_TIME_BUDGET };
};

#endif // hifi_PickManager_h
