//
//  GrabPropertyGroup.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2018-8-8.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GrabPropertyGroup_h
#define hifi_GrabPropertyGroup_h

#include <stdint.h>

#include <glm/glm.hpp>

#include <QtScript/QScriptEngine>

#include "PropertyGroup.h"
#include "EntityItemPropertiesMacros.h"

class EntityItemProperties;
class EncodeBitstreamParams;
class OctreePacketData;
class ReadBitstreamToTreeParams;

static const bool INITIAL_GRABBABLE { true };
static const bool INITIAL_KINEMATIC { true };
static const bool INITIAL_FOLLOWS_CONTROLLER { true };
static const bool INITIAL_TRIGGERABLE { false };
static const bool INITIAL_EQUIPPABLE { false };
static const glm::vec3 INITIAL_LEFT_EQUIPPABLE_POSITION { glm::vec3(0.0f) };
static const glm::quat INITIAL_LEFT_EQUIPPABLE_ROTATION { glm::quat() };
static const glm::vec3 INITIAL_RIGHT_EQUIPPABLE_POSITION { glm::vec3(0.0f) };
static const glm::quat INITIAL_RIGHT_EQUIPPABLE_ROTATION { glm::quat() };
static const glm::vec3 INITIAL_EQUIPPABLE_INDICATOR_SCALE { glm::vec3(1.0f) };
static const glm::vec3 INITIAL_EQUIPPABLE_INDICATOR_OFFSET { glm::vec3(0.0f) };


/**jsdoc
 * Grab is defined by the following properties.
 * @typedef {object} Entities.Grab
 *
 * @property {boolean} grabbable=true - If <code>true</code> the entity can be grabbed.
 * @property {boolean} grabKinematic=true - If <code>true</code> the entity is updated in a kinematic manner.
 *     If <code>false</code> it will be grabbed using a tractor action.  A kinematic grab will make the item appear more
 *     tightly held, but will cause it to behave poorly when interacting with dynamic entities.
 * @property {boolean} grabFollowsController=true - If <code>true</code> the entity will follow the motions of the
 *     hand-controller even if the avatar's hand can't get to the implied position.  This should be <code>true</code>
 *     for tools, pens, etc and false for things meant to decorate the hand.
 *
 * @property {boolean} triggerable=false - If <code>true</code> the entity will receive calls to trigger
 *     {@link Controller|Controller entity methods}.
 *
 * @property {boolean} equippable=true - If <code>true</code> the entity can be equipped.
 * @property {Vec3} equippableLeftPosition=0,0,0 - Positional offset from the left hand, when equipped.
 * @property {Quat} equippableLeftRotation=0,0,0,1 - Rotational offset from the left hand, when equipped.
 * @property {Vec3} equippableRightPosition=0,0,0 - Positional offset from the right hand, when equipped.
 * @property {Quat} equippableRightRotation=0,0,0,1 - Rotational offset from the right hand, when equipped.
 *
 * @property {string} equippableIndicatorURL="" - If non-empty, this model will be used to indicate that an
 *     entity is equippable, rather than the default.
 * @property {Vec3} equippableIndicatorScale=1,1,1 - If equippableIndicatorURL is non-empty, this controls the
       scale of the displayed overlay.
 * @property {Vec3} equippableIndicatorOffset=0,0,0 - If equippableIndicatorURL is non-empty, this controls the
       relative offset of the displayed overlay from the equippable entity.
 */


class GrabPropertyGroup : public PropertyGroup {
public:
    // EntityItemProperty related helpers
    virtual void copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties,
                                   QScriptEngine* engine, bool skipDefaults,
                                   EntityItemProperties& defaultEntityProperties) const override;
    virtual void copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) override;

    void merge(const GrabPropertyGroup& other);

    virtual void debugDump() const override;
    virtual void listChangedProperties(QList<QString>& out) override;

    virtual bool appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual bool decodeFromEditPacket(EntityPropertyFlags& propertyFlags,
                                      const unsigned char*& dataAt, int& processedBytes) override;
    virtual void markAllChanged() override;
    virtual EntityPropertyFlags getChangedProperties() const override;

    // EntityItem related helpers
    // methods for getting/setting all properties of an entity
    virtual void getProperties(EntityItemProperties& propertiesOut) const override;

    // returns true if something changed
    virtual bool setProperties(const EntityItemProperties& properties) override;

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) override;

    // grab properties
    DEFINE_PROPERTY(PROP_GRAB_GRABBABLE, Grabbable, grabbable, bool, INITIAL_GRABBABLE);
    DEFINE_PROPERTY(PROP_GRAB_KINEMATIC, GrabKinematic, grabKinematic, bool, INITIAL_KINEMATIC);
    DEFINE_PROPERTY(PROP_GRAB_FOLLOWS_CONTROLLER, GrabFollowsController, grabFollowsController, bool,
                    INITIAL_FOLLOWS_CONTROLLER);
    DEFINE_PROPERTY(PROP_GRAB_TRIGGERABLE, Triggerable, triggerable, bool, INITIAL_TRIGGERABLE);
    DEFINE_PROPERTY(PROP_GRAB_EQUIPPABLE, Equippable, equippable, bool, INITIAL_EQUIPPABLE);
    DEFINE_PROPERTY_REF(PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET, EquippableLeftPosition, equippableLeftPosition,
                        glm::vec3, INITIAL_LEFT_EQUIPPABLE_POSITION);
    DEFINE_PROPERTY_REF(PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET, EquippableLeftRotation, equippableLeftRotation,
                        glm::quat, INITIAL_LEFT_EQUIPPABLE_ROTATION);
    DEFINE_PROPERTY_REF(PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET, EquippableRightPosition, equippableRightPosition,
                        glm::vec3, INITIAL_RIGHT_EQUIPPABLE_POSITION);
    DEFINE_PROPERTY_REF(PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET, EquippableRightRotation, equippableRightRotation,
                        glm::quat, INITIAL_RIGHT_EQUIPPABLE_ROTATION);
    DEFINE_PROPERTY_REF(PROP_GRAB_EQUIPPABLE_INDICATOR_URL, EquippableIndicatorURL, equippableIndicatorURL, QString, "");
    DEFINE_PROPERTY_REF(PROP_GRAB_EQUIPPABLE_INDICATOR_SCALE, EquippableIndicatorScale, equippableIndicatorScale,
                        glm::vec3, INITIAL_EQUIPPABLE_INDICATOR_SCALE);
    DEFINE_PROPERTY_REF(PROP_GRAB_EQUIPPABLE_INDICATOR_OFFSET, EquippableIndicatorOffset, equippableIndicatorOffset,
                        glm::vec3, INITIAL_EQUIPPABLE_INDICATOR_OFFSET);
};

#endif // hifi_GrabPropertyGroup_h
