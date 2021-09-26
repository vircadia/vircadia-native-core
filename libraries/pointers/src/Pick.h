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

/*@jsdoc
 * <p>The type of an intersection.</p>
 * <table>
 *   <thead>
 *     <tr><th>Name</th><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td>INTERSECTED_NONE</td><td><code>0</code></td><td>Intersected nothing.</td></tr>
 *     <tr><td>INTERSECTED_ENTITY</td><td><code>1</code></td><td>Intersected an entity.</td></tr>
 *     <tr><td>INTERSECTED_LOCAL_ENTITY</td><td><code>2</code></td><td>Intersected a local entity.</td></tr>
 *     <tr><td>INTERSECTED_AVATAR</td><td><code>3</code></td><td>Intersected an avatar.</td></tr>
 *     <tr><td>INTERSECTED_HUD</td><td><code>4</code></td><td>Intersected the HUD surface.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {number} IntersectionType
 */
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

    /*@jsdoc
     * The <code>PickType</code> API provides constant numeric values that represent different types of picks.
     *
     * @namespace PickType
     * @variation 0
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-avatar
     *
     * @property {number} Ray - Ray picks intersect a ray with objects in front of them, along their direction.
     * @property {number} Parabola - Parabola picks intersect a parabola with objects in front of them, along their arc.
     * @property {number} Stylus - Stylus picks provide "tapping" functionality on or into flat surfaces.
     * @property {number} Collision - Collision picks intersect a collision volume with avatars and entities that have 
     *     collisions.
     */

    /*@jsdoc
     * <p>A type of pick.</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>{@link PickType(0)|PickType.Ray}</code></td><td>Ray picks intersect a ray with objects in front of 
     *       them, along their direction.</td></tr>
     *     <tr><td><code>{@link PickType(0)|PickType.Parabola}</code></td><td>Parabola picks intersect a parabola with objects
     *       in front of them, along their arc.</td></tr>
     *     <tr><td><code>{@link PickType(0)|PickType.Stylus}</code></td><td>Stylus picks provide "tapping" functionality on or
     *       into flat surfaces.</td></tr>
     *     <tr><td><code>{@link PickType(0)|PickType.Collision}</code></td><td>Collision picks intersect a collision volume 
     *       with avatars and entities that have collisions.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} PickType
     */
    enum PickType {
        Ray = 0,
        Stylus,
        Parabola,
        Collision,
        NUM_PICK_TYPES,
        INVALID_PICK_TYPE = -1
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
    virtual PickType getType() const = 0;

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

    virtual QVariantMap toVariantMap() const {
        QVariantMap properties;

        properties["pickType"] = (int)getType();
        properties["enabled"] = isEnabled();
        properties["filter"] = (unsigned int)getFilter()._flags.to_ulong();
        properties["maxDistance"] = getMaxDistance();

        if (parentTransform) {
            auto transformNodeProperties = parentTransform->toVariantMap();
            for (auto it = transformNodeProperties.cbegin(); it != transformNodeProperties.cend(); ++it) {
                properties[it.key()] = it.value();
            }
        }

        return properties;
    }

    void setScriptParameters(const QVariantMap& parameters);
    QVariantMap getScriptParameters() const;

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

    // The parameters used to create this pick when created through a script
    QVariantMap _scriptParameters;

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

    QVariantMap toVariantMap() const override {
        QVariantMap properties = PickQuery::toVariantMap();

        const QVariantMap mathPickProperties = _mathPick.toVariantMap();
        for (auto it = mathPickProperties.cbegin(); it != mathPickProperties.cend(); ++it) {
            properties[it.key()] = it.value();
        }

        return properties;
    }
    
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
