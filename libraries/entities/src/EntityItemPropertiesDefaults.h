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

// There is a minor performance gain when comparing/copying an existing glm::vec3 rather than 
// creating a new one on the stack so we declare the ZERO_VEC3 constant as an optimization.
const glm::vec3 ENTITY_ITEM_ZERO_VEC3(0.0f);

const glm::vec3 REGULAR_GRAVITY = glm::vec3(0, -9.8f / (float)TREE_SCALE, 0);

const bool ENTITY_ITEM_DEFAULT_LOCKED = false;
const QString ENTITY_ITEM_DEFAULT_USER_DATA = QString("");

const float ENTITY_ITEM_DEFAULT_LOCAL_RENDER_ALPHA = 1.0f;
const float ENTITY_ITEM_DEFAULT_GLOW_LEVEL = 0.0f;
const bool ENTITY_ITEM_DEFAULT_VISIBLE = true;

const QString ENTITY_ITEM_DEFAULT_SCRIPT = QString("");
const glm::vec3 ENTITY_ITEM_DEFAULT_REGISTRATION_POINT = glm::vec3(0.5f, 0.5f, 0.5f); // center

const float ENTITY_ITEM_IMMORTAL_LIFETIME = -1.0f; /// special lifetime which means the entity lives for ever
const float ENTITY_ITEM_DEFAULT_LIFETIME = ENTITY_ITEM_IMMORTAL_LIFETIME;

const glm::quat ENTITY_ITEM_DEFAULT_ROTATION;
const float ENTITY_ITEM_DEFAULT_WIDTH = 0.1f;
const glm::vec3 ENTITY_ITEM_DEFAULT_DIMENSIONS = glm::vec3(ENTITY_ITEM_DEFAULT_WIDTH) / (float)TREE_SCALE;
const float ENTITY_ITEM_DEFAULT_VOLUME = ENTITY_ITEM_DEFAULT_WIDTH * ENTITY_ITEM_DEFAULT_WIDTH * ENTITY_ITEM_DEFAULT_WIDTH;

const float ENTITY_ITEM_MAX_DENSITY = 10000.0f; // kg/m^3 density of silver
const float ENTITY_ITEM_MIN_DENSITY = 100.0f; // kg/m^3 density of balsa wood
const float ENTITY_ITEM_DEFAULT_DENSITY = 1000.0f;   // density of water
const float ENTITY_ITEM_DEFAULT_MASS = ENTITY_ITEM_DEFAULT_DENSITY * ENTITY_ITEM_DEFAULT_VOLUME;

const glm::vec3 ENTITY_ITEM_DEFAULT_VELOCITY = ENTITY_ITEM_ZERO_VEC3;
const glm::vec3 ENTITY_ITEM_DEFAULT_ANGULAR_VELOCITY = ENTITY_ITEM_ZERO_VEC3;
const glm::vec3 ENTITY_ITEM_DEFAULT_GRAVITY = ENTITY_ITEM_ZERO_VEC3;
const float ENTITY_ITEM_DEFAULT_DAMPING = 0.39347f;  // approx timescale = 2.0 sec (see damping timescale formula in header)
const float ENTITY_ITEM_DEFAULT_ANGULAR_DAMPING = 0.39347f;  // approx timescale = 2.0 sec (see damping timescale formula in header)

const bool ENTITY_ITEM_DEFAULT_IGNORE_FOR_COLLISIONS = false;
const bool ENTITY_ITEM_DEFAULT_COLLISIONS_WILL_MOVE = false;

#endif // hifi_EntityItemPropertiesDefaults_h
