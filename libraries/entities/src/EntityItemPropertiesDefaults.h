//
//  EntityItemPropertiesDefaults.h
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2015.01.12
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityItemPropertiesDefaults_h
#define hifi_EntityItemPropertiesDefaults_h

#include <stdint.h>

#include <glm/glm.hpp>

#include <NumericalConstants.h>

// There is a minor performance gain when comparing/copying an existing glm::vec3 rather than
// creating a new one on the stack so we declare the ZERO_VEC3 constant as an optimization.
const glm::vec3 ENTITY_ITEM_ZERO_VEC3 = glm::vec3(0.0f);
const glm::vec3 ENTITY_ITEM_ONE_VEC3 = glm::vec3(1.0f);
const glm::vec3 ENTITY_ITEM_HALF_VEC3 = glm::vec3(0.5f);

const bool ENTITY_ITEM_DEFAULT_LOCKED = false;
const QString ENTITY_ITEM_DEFAULT_USER_DATA = QString("");
const QString ENTITY_ITEM_DEFAULT_MARKETPLACE_ID = QString("");
const QUuid ENTITY_ITEM_DEFAULT_SIMULATOR_ID = QUuid();

const float ENTITY_ITEM_DEFAULT_ALPHA = 1.0f;
const float ENTITY_ITEM_DEFAULT_LOCAL_RENDER_ALPHA = 1.0f;
const bool ENTITY_ITEM_DEFAULT_VISIBLE = true;

const QString ENTITY_ITEM_DEFAULT_SCRIPT = QString("");
const quint64 ENTITY_ITEM_DEFAULT_SCRIPT_TIMESTAMP = 0;
const QString ENTITY_ITEM_DEFAULT_COLLISION_SOUND_URL = QString("");
const glm::vec3 ENTITY_ITEM_DEFAULT_REGISTRATION_POINT = ENTITY_ITEM_HALF_VEC3; // center

const float ENTITY_ITEM_IMMORTAL_LIFETIME = -1.0f; /// special lifetime which means the entity lives for ever
const float ENTITY_ITEM_DEFAULT_LIFETIME = ENTITY_ITEM_IMMORTAL_LIFETIME;

const glm::vec3 ENTITY_ITEM_DEFAULT_POSITION = ENTITY_ITEM_ZERO_VEC3;
const glm::quat ENTITY_ITEM_DEFAULT_ROTATION;
const float ENTITY_ITEM_DEFAULT_WIDTH = 0.1f;
const glm::vec3 ENTITY_ITEM_DEFAULT_DIMENSIONS = glm::vec3(ENTITY_ITEM_DEFAULT_WIDTH);
const float ENTITY_ITEM_DEFAULT_VOLUME = ENTITY_ITEM_DEFAULT_WIDTH * ENTITY_ITEM_DEFAULT_WIDTH * ENTITY_ITEM_DEFAULT_WIDTH;

const float ENTITY_ITEM_MAX_DENSITY = 10000.0f; // kg/m^3 density of silver
const float ENTITY_ITEM_MIN_DENSITY = 100.0f; // kg/m^3 density of balsa wood
const float ENTITY_ITEM_DEFAULT_DENSITY = 1000.0f;   // density of water
const float ENTITY_ITEM_DEFAULT_MASS = ENTITY_ITEM_DEFAULT_DENSITY * ENTITY_ITEM_DEFAULT_VOLUME;

const glm::vec3 ENTITY_ITEM_DEFAULT_VELOCITY = ENTITY_ITEM_ZERO_VEC3;
const glm::vec3 ENTITY_ITEM_DEFAULT_ANGULAR_VELOCITY = ENTITY_ITEM_ZERO_VEC3;
const glm::vec3 ENTITY_ITEM_DEFAULT_GRAVITY = ENTITY_ITEM_ZERO_VEC3;
const glm::vec3 ENTITY_ITEM_DEFAULT_ACCELERATION = ENTITY_ITEM_ZERO_VEC3;
const float ENTITY_ITEM_DEFAULT_DAMPING = 0.39347f;  // approx timescale = 2.0 sec (see damping timescale formula in header)
const float ENTITY_ITEM_DEFAULT_ANGULAR_DAMPING = 0.39347f;  // approx timescale = 2.0 sec (see damping timescale formula in header)

const float ENTITY_ITEM_MIN_RESTITUTION = 0.0f;
const float ENTITY_ITEM_MAX_RESTITUTION = 0.99f;
const float ENTITY_ITEM_DEFAULT_RESTITUTION = 0.5f;

const float ENTITY_ITEM_MIN_FRICTION = 0.0f;
const float ENTITY_ITEM_MAX_FRICTION = 10.0f;
const float ENTITY_ITEM_DEFAULT_FRICTION = 0.5f;

const bool ENTITY_ITEM_DEFAULT_COLLISIONLESS = false;
const bool ENTITY_ITEM_DEFAULT_DYNAMIC = false;
const bool ENTITY_ITEM_DEFAULT_BILLBOARDED = false;

const QString ENTITY_ITEM_DEFAULT_NAME = QString("");

const uint16_t ENTITY_ITEM_DEFAULT_DPI = 30;

#endif // hifi_EntityItemPropertiesDefaults_h
