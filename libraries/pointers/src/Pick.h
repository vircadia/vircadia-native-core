//
//  Created by Sam Gondelman 10/17/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_Pick_h
#define hifi_Pick_h

#include <memory>
#include <stdint.h>
#include <bitset>

#include <QtCore/QUuid>
#include <QVector>
#include <QVariant>

#include <shared/ReadWriteLockable.h>
#include <TransformNode.h>
#include <PickFilter.h>

enum IntersectionType {
    NONE = 0,
    ENTITY,
    LOCAL_ENTITY,
    AVATAR,
    HUD
};

class PickResult {
public:
    PickResult() {}
    PickResult(const QVariantMap& pickVariant) : pickVariant(pickVariant) {}
    virtual ~PickResult() = default;

    virtual QVariantMap toVariantMap() const {
        return pickVariant;
    }

    virtual bool doesIntersect() const = 0;

    // for example: if we want the closest result, compare based on distance
    // if we want all results, combine them
    // must return a new pointer
    virtual std::shared_ptr<PickResult> compareAndProcessNewResult(const std::shared_ptr<PickResult>& newRes) = 0;

    // returns true if this result contains any valid results with distance < maxDistance
    // can also filter out results with distance >= maxDistance
    virtual bool checkOrFilterAgainstMaxDistance(float maxDistance) = 0;

    QVariantMap pickVariant;
};

using PickResultPointer = std::shared_ptr<PickResult>;

class PickQuery : protected ReadWriteLockable {
    Q_GADGET
public:
    PickQuery(const PickFilter& filter, const float maxDistance, const bool enabled);
    virtual ~PickQuery() = default;

    /**jsdoc
     * Enum for different types of Picks and Pointers.
     *
     * @namespace PickType
     * @variation 0
     *
     * @hifi-interface
     * @hifi-client-entity
     *
     * @property {number} Ray Ray picks intersect a ray with the nearest object in front of them, along a given direction.
     * @property {number} Stylus Stylus picks provide "tapping" functionality on/into flat surfaces.
     * @property {number} Parabola Parabola picks intersect a parabola with the nearest object in front of them, with a given 
     *     initial velocity and acceleration.
     * @property {number} Collision Collision picks intersect a collision volume with avatars and entities that have collisions.
     */
    /**jsdoc
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>{@link PickType(0)|PickType.Ray}</code></td><td></td></tr>
     *     <tr><td><code>{@link PickType(0)|PickType.Stylus}</code></td><td></td></tr>
     *     <tr><td><code>{@link PickType(0)|PickType.Parabola}</code></td><td></td></tr>
     *     <tr><td><code>{@link PickType(0)|PickType.Collision}</code></td><td></td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} PickType
     */
    enum PickType {
        Ray = 0,
        Stylus,
        Parabola,
        Collision,
        NUM_PICK_TYPES
    };
    Q_ENUM(PickType)

    enum JointState {
        JOINT_STATE_NONE = 0,
        JOINT_STATE_LEFT_HAND,
        JOINT_STATE_RIGHT_HAND,
        JOINT_STATE_MOUSE
    };

    void enable(bool enabled = true);
    void disable() { enable(false); }

    PickFilter getFilter() const;
    float getMaxDistance() const;
    bool isEnabled() const;

    void setPrecisionPicking(bool precisionPicking);

    PickResultPointer getPrevPickResult() const;
    void setPickResult(const PickResultPointer& pickResult);

    QVector<QUuid> getIgnoreItems() const;
    QVector<QUuid> getIncludeItems() const;

    template <typename S>
    QVector<S> getIgnoreItemsAs() const {
        QVector<S> result;
        withReadLock([&] {
            for (const auto& uid : _ignoreItems) {
                result.push_back(uid);
            }
        });
        return result;
    }

    template <typename S>
    QVector<S> getIncludeItemsAs() const {
        QVector<S> result;
        withReadLock([&] {
            for (const auto& uid : _includeItems) {
                result.push_back(uid);
            }
        });
        return result;
    }

    void setIgnoreItems(const QVector<QUuid>& items);
    void setIncludeItems(const QVector<QUuid>& items);

    virtual bool isLeftHand() const { return _jointState == JOINT_STATE_LEFT_HAND; }
    virtual bool isRightHand() const { return _jointState == JOINT_STATE_RIGHT_HAND; }
    virtual bool isMouse() const { return _jointState == JOINT_STATE_MOUSE; }

    void setJointState(JointState jointState) { _jointState = jointState; }

    virtual Transform getResultTransform() const = 0;

    std::shared_ptr<TransformNode> parentTransform;

private:
    PickFilter _filter;
    const float _maxDistance;
    bool _enabled;
    PickResultPointer _prevResult;

    QVector<QUuid> _ignoreItems;
    QVector<QUuid> _includeItems;

    JointState _jointState { JOINT_STATE_NONE };
};
Q_DECLARE_METATYPE(PickQuery::PickType)

template<typename T>
class Pick : public PickQuery {
public:
    Pick(const T& mathPick, const PickFilter& filter, const float maxDistance, const bool enabled) : PickQuery(filter, maxDistance, enabled), _mathPick(mathPick) {}

    virtual T getMathematicalPick() const = 0;
    virtual PickResultPointer getDefaultResult(const QVariantMap& pickVariant) const = 0;
    virtual PickResultPointer getEntityIntersection(const T& pick) = 0;
    virtual PickResultPointer getAvatarIntersection(const T& pick) = 0;
    virtual PickResultPointer getHUDIntersection(const T& pick) = 0;

protected:
    T _mathPick;
};

namespace std {
    template <>
    struct hash<PickQuery::PickType> {
        size_t operator()(const PickQuery::PickType& a) const {
            return a;
        }
    };
}

#endif // hifi_Pick_h
