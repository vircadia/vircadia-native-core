//
//  EntityItemProperties.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include <QObject>

#include <ByteCountCoding.h>
#include <GLMHelpers.h>
#include <RegisteredMetaTypes.h>

#include "EntityItem.h"
#include "EntityItemProperties.h"
#include "ModelEntityItem.h"

EntityItemProperties::EntityItemProperties() :

    _id(UNKNOWN_ENTITY_ID),
    _idSet(false),
    _lastEdited(0),
    _created(UNKNOWN_CREATED_TIME),
    _type(EntityTypes::Unknown),

    _position(0),
    _dimensions(EntityItem::DEFAULT_DIMENSIONS),
    _rotation(EntityItem::DEFAULT_ROTATION),
    _mass(EntityItem::DEFAULT_MASS),
    _velocity(EntityItem::DEFAULT_VELOCITY),
    _gravity(EntityItem::DEFAULT_GRAVITY),
    _damping(EntityItem::DEFAULT_DAMPING),
    _lifetime(EntityItem::DEFAULT_LIFETIME),
    _script(EntityItem::DEFAULT_SCRIPT),

    _positionChanged(false),
    _dimensionsChanged(false),
    _rotationChanged(false),
    _massChanged(false),
    _velocityChanged(false),
    _gravityChanged(false),
    _dampingChanged(false),
    _lifetimeChanged(false),
    _scriptChanged(false),

    _color(),
    _modelURL(""),
    _animationURL(""),
    _animationIsPlaying(ModelEntityItem::DEFAULT_ANIMATION_IS_PLAYING),
    _animationFrameIndex(ModelEntityItem::DEFAULT_ANIMATION_FRAME_INDEX),
    _animationFPS(ModelEntityItem::DEFAULT_ANIMATION_FPS),
    _glowLevel(0.0f),

    _colorChanged(false),
    _modelURLChanged(false),
    _animationURLChanged(false),
    _animationIsPlayingChanged(false),
    _animationFrameIndexChanged(false),
    _animationFPSChanged(false),
    _glowLevelChanged(false),

    _defaultSettings(true)
{
}

void EntityItemProperties::debugDump() const {
    qDebug() << "EntityItemProperties...";
    qDebug() << "    _type=" << EntityTypes::getEntityTypeName(_type);
    qDebug() << "   _id=" << _id;
    qDebug() << "   _idSet=" << _idSet;
    qDebug() << "   _position=" << _position.x << "," << _position.y << "," << _position.z;
    qDebug() << "   _dimensions=" << getDimensions();
    qDebug() << "   _modelURL=" << _modelURL;
    qDebug() << "   changed properties...";
    EntityPropertyFlags props = getChangedProperties();
    props.debugDumpBits();
}

EntityPropertyFlags EntityItemProperties::getChangedProperties() const {
    EntityPropertyFlags changedProperties;
    if (_dimensionsChanged) {
        changedProperties += PROP_DIMENSIONS;
    }

    if (_positionChanged) {
        changedProperties += PROP_POSITION;
    }

    if (_rotationChanged) {
        changedProperties += PROP_ROTATION;
    }

    if (_massChanged) {
        changedProperties += PROP_MASS;
    }

    if (_velocityChanged) {
        changedProperties += PROP_VELOCITY;
    }

    if (_gravityChanged) {
        changedProperties += PROP_GRAVITY;
    }

    if (_dampingChanged) {
        changedProperties += PROP_DAMPING;
    }

    if (_lifetimeChanged) {
        changedProperties += PROP_LIFETIME;
    }

    if (_scriptChanged) {
        changedProperties += PROP_SCRIPT;
    }

    if (_colorChanged) {
        changedProperties += PROP_COLOR;
    }

    if (_modelURLChanged) {
        changedProperties += PROP_MODEL_URL;
    }

    if (_animationURLChanged) {
        changedProperties += PROP_ANIMATION_URL;
    }

    if (_animationIsPlayingChanged) {
        changedProperties += PROP_ANIMATION_PLAYING;
    }

    if (_animationFrameIndexChanged) {
        changedProperties += PROP_ANIMATION_FRAME_INDEX;
    }

    if (_animationFPSChanged) {
        changedProperties += PROP_ANIMATION_FPS;
    }

    return changedProperties;
}

QScriptValue EntityItemProperties::copyToScriptValue(QScriptEngine* engine) const {
    QScriptValue properties = engine->newObject();

    if (_idSet) {
        properties.setProperty("id", _id.toString());
        bool isKnownID = (_id != UNKNOWN_ENTITY_ID);
        properties.setProperty("isKnownID", isKnownID);
    }

    properties.setProperty("type", EntityTypes::getEntityTypeName(_type));

    QScriptValue position = vec3toScriptValue(engine, _position);
    properties.setProperty("position", position);
    QScriptValue dimensions = vec3toScriptValue(engine, _dimensions);
    properties.setProperty("dimensions", dimensions);
    QScriptValue rotation = quatToScriptValue(engine, _rotation);
    properties.setProperty("rotation", rotation);
    properties.setProperty("mass", _mass);
    QScriptValue velocity = vec3toScriptValue(engine, _velocity);
    properties.setProperty("velocity", velocity);
    QScriptValue gravity = vec3toScriptValue(engine, _gravity);
    properties.setProperty("gravity", gravity);
    properties.setProperty("damping", _damping);
    properties.setProperty("lifetime", _lifetime);
    properties.setProperty("age", getAge()); // gettable, but not settable
    properties.setProperty("ageAsText", formatSecondsElapsed(getAge())); // gettable, but not settable
    properties.setProperty("script", _script);

    QScriptValue color = xColorToScriptValue(engine, _color);
    properties.setProperty("color", color);
    properties.setProperty("modelURL", _modelURL);
    
    properties.setProperty("animationURL", _animationURL);
    properties.setProperty("animationIsPlaying", _animationIsPlaying);
    properties.setProperty("animationFrameIndex", _animationFrameIndex);
    properties.setProperty("animationFPS", _animationFPS);
    properties.setProperty("glowLevel", _glowLevel);

    // Sitting properties support
    QScriptValue sittingPoints = engine->newObject();
    for (int i = 0; i < _sittingPoints.size(); ++i) {
        QScriptValue sittingPoint = engine->newObject();
        sittingPoint.setProperty("name", _sittingPoints[i].name);
        sittingPoint.setProperty("position", vec3toScriptValue(engine, _sittingPoints[i].position));
        sittingPoint.setProperty("rotation", quatToScriptValue(engine, _sittingPoints[i].rotation));
        sittingPoints.setProperty(i, sittingPoint);
    }
    sittingPoints.setProperty("length", _sittingPoints.size());
    properties.setProperty("sittingPoints", sittingPoints);

    return properties;
}

void EntityItemProperties::copyFromScriptValue(const QScriptValue& object) {
    QScriptValue typeScriptValue = object.property("type");
    if (typeScriptValue.isValid()) {
        QString typeName;
        typeName = typeScriptValue.toVariant().toString();
        _type = EntityTypes::getEntityTypeFromName(typeName);
    }

    QScriptValue position = object.property("position");
    if (position.isValid()) {
        QScriptValue x = position.property("x");
        QScriptValue y = position.property("y");
        QScriptValue z = position.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newPosition;
            newPosition.x = x.toVariant().toFloat();
            newPosition.y = y.toVariant().toFloat();
            newPosition.z = z.toVariant().toFloat();
            if (_defaultSettings || newPosition != _position) {
                setPosition(newPosition); // gives us automatic clamping
            }
        }
    }

    QScriptValue dimensions = object.property("dimensions");
    if (dimensions.isValid()) {
        QScriptValue x = dimensions.property("x");
        QScriptValue y = dimensions.property("y");
        QScriptValue z = dimensions.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newDimensions;
            newDimensions.x = x.toVariant().toFloat();
            newDimensions.y = y.toVariant().toFloat();
            newDimensions.z = z.toVariant().toFloat();
            if (_defaultSettings || newDimensions != getDimensions()) {
                setDimensions(newDimensions);
            }
        }
    }

    QScriptValue rotation = object.property("rotation");
    if (rotation.isValid()) {
        QScriptValue x = rotation.property("x");
        QScriptValue y = rotation.property("y");
        QScriptValue z = rotation.property("z");
        QScriptValue w = rotation.property("w");
        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            glm::quat newRotation;
            newRotation.x = x.toVariant().toFloat();
            newRotation.y = y.toVariant().toFloat();
            newRotation.z = z.toVariant().toFloat();
            newRotation.w = w.toVariant().toFloat();
            if (_defaultSettings || newRotation != _rotation) {
                _rotation = newRotation;
                _rotationChanged = true;
            }
        }
    }

    QScriptValue mass = object.property("mass");
    if (mass.isValid()) {
        float newValue;
        newValue = mass.toVariant().toFloat();
        if (_defaultSettings || newValue != _mass) {
            _mass = newValue;
            _massChanged = true;
        }
    }
    
    QScriptValue velocity = object.property("velocity");
    if (velocity.isValid()) {
        QScriptValue x = velocity.property("x");
        QScriptValue y = velocity.property("y");
        QScriptValue z = velocity.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newValue;
            newValue.x = x.toVariant().toFloat();
            newValue.y = y.toVariant().toFloat();
            newValue.z = z.toVariant().toFloat();
            if (_defaultSettings || newValue != _velocity) {
                _velocity = newValue;
                _velocityChanged = true;
            }
        }
    }

    QScriptValue gravity = object.property("gravity");
    if (gravity.isValid()) {
        QScriptValue x = gravity.property("x");
        QScriptValue y = gravity.property("y");
        QScriptValue z = gravity.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newValue;
            newValue.x = x.toVariant().toFloat();
            newValue.y = y.toVariant().toFloat();
            newValue.z = z.toVariant().toFloat();
            if (_defaultSettings || newValue != _gravity) {
                _gravity = newValue;
                _gravityChanged = true;
            }
        }
    }

    QScriptValue damping = object.property("damping");
    if (damping.isValid()) {
        float newValue;
        newValue = damping.toVariant().toFloat();
        if (_defaultSettings || newValue != _damping) {
            _damping = newValue;
            _dampingChanged = true;
        }
    }

    QScriptValue lifetime = object.property("lifetime");
    if (lifetime.isValid()) {
        float newValue;
        newValue = lifetime.toVariant().toFloat();
        if (_defaultSettings || newValue != _lifetime) {
            _lifetime = newValue;
            _lifetimeChanged = true;
        }
    }

    QScriptValue script = object.property("script");
    if (script.isValid()) {
        QString newValue;
        newValue = script.toVariant().toString();
        if (_defaultSettings || newValue != _script) {
            _script = newValue;
            _scriptChanged = true;
        }
    }

    QScriptValue color = object.property("color");
    if (color.isValid()) {
        QScriptValue red = color.property("red");
        QScriptValue green = color.property("green");
        QScriptValue blue = color.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            xColor newColor;
            newColor.red = red.toVariant().toInt();
            newColor.green = green.toVariant().toInt();
            newColor.blue = blue.toVariant().toInt();
            if (_defaultSettings || (newColor.red != _color.red ||
                newColor.green != _color.green ||
                newColor.blue != _color.blue)) {
                _color = newColor;
                _colorChanged = true;
            }
        }
    }

    QScriptValue modelURL = object.property("modelURL");
    if (modelURL.isValid()) {
        QString newModelURL;
        newModelURL = modelURL.toVariant().toString();
        if (_defaultSettings || newModelURL != _modelURL) {
            _modelURL = newModelURL;
            _modelURLChanged = true;
        }
    }
    
    QScriptValue animationURL = object.property("animationURL");
    if (animationURL.isValid()) {
        QString newAnimationURL;
        newAnimationURL = animationURL.toVariant().toString();
        if (_defaultSettings || newAnimationURL != _animationURL) {
            _animationURL = newAnimationURL;
            _animationURLChanged = true;
        }
    }

    QScriptValue animationIsPlaying = object.property("animationIsPlaying");
    if (animationIsPlaying.isValid()) {
        bool newIsAnimationPlaying;
        newIsAnimationPlaying = animationIsPlaying.toVariant().toBool();
        if (_defaultSettings || newIsAnimationPlaying != _animationIsPlaying) {
            _animationIsPlaying = newIsAnimationPlaying;
            _animationIsPlayingChanged = true;
        }
    }
    
    QScriptValue animationFrameIndex = object.property("animationFrameIndex");
    if (animationFrameIndex.isValid()) {
        float newFrameIndex;
        newFrameIndex = animationFrameIndex.toVariant().toFloat();
        if (_defaultSettings || newFrameIndex != _animationFrameIndex) {
            _animationFrameIndex = newFrameIndex;
            _animationFrameIndexChanged = true;
        }
    }
    
    QScriptValue animationFPS = object.property("animationFPS");
    if (animationFPS.isValid()) {
        float newFPS;
        newFPS = animationFPS.toVariant().toFloat();
        if (_defaultSettings || newFPS != _animationFPS) {
            _animationFPS = newFPS;
            _animationFPSChanged = true;
        }
    }
    
    QScriptValue glowLevel = object.property("glowLevel");
    if (glowLevel.isValid()) {
        float newGlowLevel;
        newGlowLevel = glowLevel.toVariant().toFloat();
        if (_defaultSettings || newGlowLevel != _glowLevel) {
            _glowLevel = newGlowLevel;
            _glowLevelChanged = true;
        }
    }
    _lastEdited = usecTimestampNow();
}

QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties) {
    return properties.copyToScriptValue(engine);
}

void EntityItemPropertiesFromScriptValue(const QScriptValue &object, EntityItemProperties& properties) {
    properties.copyFromScriptValue(object);
}

// TODO: Implement support for edit packets that can span an MTU sized buffer. We need to implement a mechanism for the 
//       encodeEntityEditPacket() method to communicate the the caller which properties couldn't fit in the buffer. Similar 
//       to how we handle this in the Octree streaming case.
//
// TODO: Right now, all possible properties for all subclasses are handled here. Ideally we'd prefer
//       to handle this in a more generic way. Allowing subclasses of EntityItem to register their properties
//
// TODO: There's a lot of repeated patterns in the code below to handle each property. It would be nice if the property
//       registration mechanism allowed us to collapse these repeated sections of code into a single implementation that
//       utilized the registration table to shorten up and simplify this code.
//
// TODO: Implement support for paged properties, spanning MTU, and custom properties
//
// TODO: Implement support for script and visible properties.
//
bool EntityItemProperties::encodeEntityEditPacket(PacketType command, EntityItemID id, const EntityItemProperties& properties,
        unsigned char* bufferOut, int sizeIn, int& sizeOut) {
    OctreePacketData ourDataPacket(false, sizeIn); // create a packetData object to add out packet details too.
    OctreePacketData* packetData = &ourDataPacket; // we want a pointer to this so we can use our APPEND_ENTITY_PROPERTY macro

    bool success = true; // assume the best
    OctreeElement::AppendState appendState = OctreeElement::COMPLETED; // assume the best
    sizeOut = 0;

    // TODO: We need to review how jurisdictions should be handled for entities. (The old Models and Particles code
    // didn't do anything special for jurisdictions, so we're keeping that same behavior here.)
    //
    // Always include the root octcode. This is only because the OctreeEditPacketSender will check these octcodes
    // to determine which server to send the changes to in the case of multiple jurisdictions. The root will be sent
    // to all servers.
    glm::vec3 rootPosition(0);
    float rootScale = 0.5f;
    unsigned char* octcode = pointToOctalCode(rootPosition.x, rootPosition.y, rootPosition.z, rootScale);

    success = packetData->startSubTree(octcode);
    delete[] octcode;
    
    // assuming we have rome to fit our octalCode, proceed...
    if (success) {

        // Now add our edit content details...
        bool isNewEntityItem = (id.id == NEW_ENTITY);

        // id
        // encode our ID as a byte count coded byte stream
        QByteArray encodedID = id.id.toRfc4122(); // NUM_BYTES_RFC4122_UUID

        // encode our ID as a byte count coded byte stream
        ByteCountCoded<quint32> tokenCoder;
        QByteArray encodedToken;

        // special case for handling "new" modelItems
        if (isNewEntityItem) {
            // encode our creator token as a byte count coded byte stream
            tokenCoder = id.creatorTokenID;
            encodedToken = tokenCoder;
        }

        // encode our type as a byte count coded byte stream
        ByteCountCoded<quint32> typeCoder = (quint32)properties.getType();
        QByteArray encodedType = typeCoder;

        quint64 updateDelta = 0; // this is an edit so by definition, it's update is in sync
        ByteCountCoded<quint64> updateDeltaCoder = updateDelta;
        QByteArray encodedUpdateDelta = updateDeltaCoder;

        EntityPropertyFlags propertyFlags(PROP_LAST_ITEM);
        EntityPropertyFlags requestedProperties = properties.getChangedProperties();
        EntityPropertyFlags propertiesDidntFit = requestedProperties;

        // TODO: we need to handle the multi-pass form of this, similar to how we handle entity data
        //
        // If we are being called for a subsequent pass at appendEntityData() that failed to completely encode this item,
        // then our modelTreeElementExtraEncodeData should include data about which properties we need to append.
        //if (modelTreeElementExtraEncodeData && modelTreeElementExtraEncodeData->includedItems.contains(getEntityItemID())) {
        //    requestedProperties = modelTreeElementExtraEncodeData->includedItems.value(getEntityItemID());
        //}

        LevelDetails entityLevel = packetData->startLevel();

        // Last Edited quint64 always first, before any other details, which allows us easy access to adjusting this
        // timestamp for clock skew
        quint64 lastEdited = properties.getLastEdited();
        bool successLastEditedFits = packetData->appendValue(lastEdited);

        bool successIDFits = packetData->appendValue(encodedID);
        if (isNewEntityItem && successIDFits) {
            successIDFits = packetData->appendValue(encodedToken);
        }
        bool successTypeFits = packetData->appendValue(encodedType);

        // NOTE: We intentionally do not send "created" times in edit messages. This is because:
        //   1) if the edit is to an existing entity, the created time can not be changed
        //   2) if the edit is to a new entity, the created time is the last edited time

        // TODO: Should we get rid of this in this in edit packets, since this has to always be 0?
        bool successLastUpdatedFits = packetData->appendValue(encodedUpdateDelta);
    
        int propertyFlagsOffset = packetData->getUncompressedByteOffset();
        QByteArray encodedPropertyFlags = propertyFlags;
        int oldPropertyFlagsLength = encodedPropertyFlags.length();
        bool successPropertyFlagsFits = packetData->appendValue(encodedPropertyFlags);
        int propertyCount = 0;

        bool headerFits = successIDFits && successTypeFits && successLastEditedFits
                                  && successLastUpdatedFits && successPropertyFlagsFits;

        int startOfEntityItemData = packetData->getUncompressedByteOffset();

        if (headerFits) {
            bool successPropertyFits;
            propertyFlags -= PROP_LAST_ITEM; // clear the last item for now, we may or may not set it as the actual item

            // These items would go here once supported....
            //      PROP_PAGED_PROPERTY,
            //      PROP_CUSTOM_PROPERTIES_INCLUDED,
            //      PROP_VISIBLE,

            APPEND_ENTITY_PROPERTY(PROP_POSITION, appendPosition, properties.getPosition());
            APPEND_ENTITY_PROPERTY(PROP_DIMENSIONS, appendValue, properties.getDimensions()); // NOTE: PROP_RADIUS obsolete
            APPEND_ENTITY_PROPERTY(PROP_ROTATION, appendValue, properties.getRotation());
            APPEND_ENTITY_PROPERTY(PROP_MASS, appendValue, properties.getMass());
            APPEND_ENTITY_PROPERTY(PROP_VELOCITY, appendValue, properties.getVelocity());
            APPEND_ENTITY_PROPERTY(PROP_GRAVITY, appendValue, properties.getGravity());
            APPEND_ENTITY_PROPERTY(PROP_DAMPING, appendValue, properties.getDamping());
            APPEND_ENTITY_PROPERTY(PROP_LIFETIME, appendValue, properties.getLifetime());
            //APPEND_ENTITY_PROPERTY(PROP_SCRIPT, appendValue, properties.getScript());  // not supported by edit messages
            APPEND_ENTITY_PROPERTY(PROP_COLOR, appendColor, properties.getColor());
            APPEND_ENTITY_PROPERTY(PROP_MODEL_URL, appendValue, properties.getModelURL());
            APPEND_ENTITY_PROPERTY(PROP_ANIMATION_URL, appendValue, properties.getAnimationURL());
            APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, appendValue, properties.getAnimationFPS());
            APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, appendValue, properties.getAnimationFrameIndex());
            APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, appendValue, properties.getAnimationIsPlaying());
        }
        if (propertyCount > 0) {
            int endOfEntityItemData = packetData->getUncompressedByteOffset();
        
            encodedPropertyFlags = propertyFlags;
            int newPropertyFlagsLength = encodedPropertyFlags.length();
            packetData->updatePriorBytes(propertyFlagsOffset, 
                    (const unsigned char*)encodedPropertyFlags.constData(), encodedPropertyFlags.length());
        
            // if the size of the PropertyFlags shrunk, we need to shift everything down to front of packet.
            if (newPropertyFlagsLength < oldPropertyFlagsLength) {
                int oldSize = packetData->getUncompressedSize();

                const unsigned char* modelItemData = packetData->getUncompressedData(propertyFlagsOffset + oldPropertyFlagsLength);
                int modelItemDataLength = endOfEntityItemData - startOfEntityItemData;
                int newEntityItemDataStart = propertyFlagsOffset + newPropertyFlagsLength;
                packetData->updatePriorBytes(newEntityItemDataStart, modelItemData, modelItemDataLength);

                int newSize = oldSize - (oldPropertyFlagsLength - newPropertyFlagsLength);
                packetData->setUncompressedSize(newSize);

            } else {
                assert(newPropertyFlagsLength == oldPropertyFlagsLength); // should not have grown
            }
       
            packetData->endLevel(entityLevel);
        } else {
            packetData->discardLevel(entityLevel);
            appendState = OctreeElement::NONE; // if we got here, then we didn't include the item
        }
    
        // If any part of the model items didn't fit, then the element is considered partial
        if (appendState != OctreeElement::COMPLETED) {

            // TODO: handle mechanism for handling partial fitting data!
            // add this item into our list for the next appendElementData() pass
            //modelTreeElementExtraEncodeData->includedItems.insert(getEntityItemID(), propertiesDidntFit);

            // for now, if it's not complete, it's not successful
            success = false;
        }
    }
    
    if (success) {
        packetData->endSubTree();
        const unsigned char* finalizedData = packetData->getFinalizedData();
        int  finalizedSize = packetData->getFinalizedSize();
        if (finalizedSize <= sizeIn) {
            memcpy(bufferOut, finalizedData, finalizedSize);
            sizeOut = finalizedSize;
        } else {
            qDebug() << "ERROR - encoded edit message doesn't fit in output buffer.";
            sizeOut = 0;
            success = false;
        }
    } else {
        packetData->discardSubTree();
        sizeOut = 0;
    }
    return success;
}

// TODO: 
//   how to handle lastEdited?
//   how to handle lastUpdated?
//   consider handling case where no properties are included... we should just ignore this packet...
//
// TODO: Right now, all possible properties for all subclasses are handled here. Ideally we'd prefer
//       to handle this in a more generic way. Allowing subclasses of EntityItem to register their properties
//
// TODO: There's a lot of repeated patterns in the code below to handle each property. It would be nice if the property
//       registration mechanism allowed us to collapse these repeated sections of code into a single implementation that
//       utilized the registration table to shorten up and simplify this code.
//
// TODO: Implement support for paged properties, spanning MTU, and custom properties
//
// TODO: Implement support for script and visible properties.
//
bool EntityItemProperties::decodeEntityEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes,
                        EntityItemID& entityID, EntityItemProperties& properties) {
    bool valid = false;

    const unsigned char* dataAt = data;
    processedBytes = 0;

    // the first part of the data is an octcode, this is a required element of the edit packet format, but we don't
    // actually use it, we do need to skip it and read to the actual data we care about.
    int octets = numberOfThreeBitSectionsInCode(data);
    int bytesToReadOfOctcode = bytesRequiredForCodeLength(octets);

    // we don't actually do anything with this octcode...
    dataAt += bytesToReadOfOctcode;
    processedBytes += bytesToReadOfOctcode;
    
    // Edit packets have a last edited time stamp immediately following the octcode.
    // NOTE: the edit times have been set by the editor to match out clock, so we don't need to adjust
    // these times for clock skew at this point.
    quint64 lastEdited;
    memcpy(&lastEdited, dataAt, sizeof(lastEdited));
    dataAt += sizeof(lastEdited);
    processedBytes += sizeof(lastEdited);
    properties.setLastEdited(lastEdited);

    // NOTE: We intentionally do not send "created" times in edit messages. This is because:
    //   1) if the edit is to an existing entity, the created time can not be changed
    //   2) if the edit is to a new entity, the created time is the last edited time

    // encoded id
    QByteArray encodedID((const char*)dataAt, NUM_BYTES_RFC4122_UUID); // maximum possible size
    QUuid editID = QUuid::fromRfc4122(encodedID);
    dataAt += encodedID.size();
    processedBytes += encodedID.size();

    bool isNewEntityItem = (editID == NEW_ENTITY);

    if (isNewEntityItem) {
        // If this is a NEW_ENTITY, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id

        QByteArray encodedToken((const char*)dataAt, (bytesToRead - processedBytes));
        ByteCountCoded<quint32> tokenCoder = encodedToken;
        quint32 creatorTokenID = tokenCoder;
        encodedToken = tokenCoder; // determine true bytesToRead
        dataAt += encodedToken.size();
        processedBytes += encodedToken.size();

        //newEntityItem.setCreatorTokenID(creatorTokenID);
        //newEntityItem._newlyCreated = true;
        
        entityID.id = NEW_ENTITY;
        entityID.creatorTokenID = creatorTokenID;
        entityID.isKnownID = false;

        valid = true;

        // created time is lastEdited time
        properties.setCreated(lastEdited);
    } else {
        entityID.id = editID;
        entityID.creatorTokenID = UNKNOWN_ENTITY_TOKEN;
        entityID.isKnownID = true;
        valid = true;

        // created time is lastEdited time
        properties.setCreated(USE_EXISTING_CREATED_TIME);
    }

    // Entity Type...
    QByteArray encodedType((const char*)dataAt, (bytesToRead - processedBytes));
    ByteCountCoded<quint32> typeCoder = encodedType;
    quint32 entityTypeCode = typeCoder;
    properties.setType((EntityTypes::EntityType)entityTypeCode);
    encodedType = typeCoder; // determine true bytesToRead
    dataAt += encodedType.size();
    processedBytes += encodedType.size();

    // Update Delta - when was this item updated relative to last edit... this really should be 0
    // TODO: Should we get rid of this in this in edit packets, since this has to always be 0?
    // TODO: do properties need to handle lastupdated???

    // last updated is stored as ByteCountCoded delta from lastEdited
    QByteArray encodedUpdateDelta((const char*)dataAt, (bytesToRead - processedBytes));
    ByteCountCoded<quint64> updateDeltaCoder = encodedUpdateDelta;
    encodedUpdateDelta = updateDeltaCoder; // determine true bytesToRead
    dataAt += encodedUpdateDelta.size();
    processedBytes += encodedUpdateDelta.size();

    // TODO: Do we need this lastUpdated?? We don't seem to use it.
    //quint64 updateDelta = updateDeltaCoder;
    //quint64 lastUpdated = lastEdited + updateDelta; // don't adjust for clock skew since we already did that for lastEdited
    
    // Property Flags...
    QByteArray encodedPropertyFlags((const char*)dataAt, (bytesToRead - processedBytes));
    EntityPropertyFlags propertyFlags = encodedPropertyFlags;
    dataAt += propertyFlags.getEncodedLength();
    processedBytes += propertyFlags.getEncodedLength();

    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_POSITION, glm::vec3, setPosition);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DIMENSIONS, glm::vec3, setDimensions);  // NOTE: PROP_RADIUS obsolete
    READ_ENTITY_PROPERTY_QUAT_TO_PROPERTIES(PROP_ROTATION, setRotation);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MASS, float, setMass);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VELOCITY, glm::vec3, setVelocity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_GRAVITY, glm::vec3, setGravity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DAMPING, float, setDamping);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LIFETIME, float, setLifetime);
    //READ_ENTITY_PROPERTY_STRING(PROP_SCRIPT,setScript); // not yet supported by edit messages...
    READ_ENTITY_PROPERTY_COLOR_TO_PROPERTIES(PROP_COLOR, setColor);
    READ_ENTITY_PROPERTY_STRING_TO_PROPERTIES(PROP_MODEL_URL, setModelURL);
    READ_ENTITY_PROPERTY_STRING_TO_PROPERTIES(PROP_ANIMATION_URL, setAnimationURL);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_FPS, float, setAnimationFPS);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_FRAME_INDEX, float, setAnimationFrameIndex);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_PLAYING, bool, setAnimationIsPlaying);
    // TODO: add PROP_REGISTRATION_POINT,
    // TODO: add PROP_ROTATIONAL_VELOCITY,
    // TODO: add PROP_VISIBLE,

    return valid;
}


// NOTE: This version will only encode the portion of the edit message immediately following the
// header it does not include the send times and sequence number because that is handled by the 
// edit packet sender...
bool EntityItemProperties::encodeEraseEntityMessage(const EntityItemID& entityItemID, 
                                            unsigned char* outputBuffer, size_t maxLength, size_t& outputLength) {

    unsigned char* copyAt = outputBuffer;
    uint16_t numberOfIds = 1; // only one entity ID in this message

    if (maxLength < sizeof(numberOfIds) + NUM_BYTES_RFC4122_UUID) {
        qDebug() << "ERROR - encodeEraseEntityMessage() called with buffer that is too small!";
        outputLength = 0;
        return false;
    }
    memcpy(copyAt, &numberOfIds, sizeof(numberOfIds));
    copyAt += sizeof(numberOfIds);
    outputLength = sizeof(numberOfIds);

    QUuid entityID = entityItemID.id;                
    QByteArray encodedEntityID = entityID.toRfc4122();

    memcpy(copyAt, encodedEntityID.constData(), NUM_BYTES_RFC4122_UUID);
    copyAt += NUM_BYTES_RFC4122_UUID;
    outputLength += NUM_BYTES_RFC4122_UUID;

    return true;
}


void EntityItemProperties::markAllChanged() {
    _positionChanged = true;
    _rotationChanged = true;
    _massChanged = true;
    _velocityChanged = true;
    _gravityChanged = true;
    _dampingChanged = true;
    _lifetimeChanged = true;
    _scriptChanged = true;

    _colorChanged = true;
    _modelURLChanged = true;
    _animationURLChanged = true;
    _animationIsPlayingChanged = true;
    _animationFrameIndexChanged = true;
    _animationFPSChanged = true;
    _glowLevelChanged = true;

}


// TODO: Add support for registration point
glm::vec3 EntityItemProperties::getMinimumPointMeters() const { 
    // This assumes the registration point is in the center, we need to update this when we really support
    // registration point
    return _position - (_dimensions / 2.0f); 
}

// TODO: Add support for registration point
glm::vec3 EntityItemProperties::getMaximumPointMeters() const { 
    // This assumes the registration point is in the center, we need to update this when we really support
    // registration point
    return _position + (_dimensions / 2.0f); 
}
