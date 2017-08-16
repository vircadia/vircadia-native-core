//
//  AvatarConstants.h
//  libraries/shared/src
//
//  Created by Anthony J. Thibault on Aug 16th 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarConstants_h
#define hifi_AvatarConstants_h

// 50th Percentile Man
const float DEFAULT_AVATAR_HEIGHT = 1.755f; // meters
const float DEFAULT_AVATAR_EYE_TO_TOP_OF_HEAD = 0.11f; // meters
const float DEFAULT_AVATAR_NECK_TO_TOP_OF_HEAD = 0.185f; // meters

// Used when avatar is missing joints... (avatar space)
static const glm::quat DEFAULT_AVATAR_MIDDLE_EYE_ROT { Quaternions::Y_180 };
static const glm::vec3 DEFAULT_AVATAR_MIDDLE_EYE_POS { 0.0f, 0.6f, 0.0f };
static const glm::vec3 DEFAULT_AVATAR_HEAD_POS { 0.0f, 0.53f, 0.0f };
static const glm::quat DEFAULT_AVATAR_HEAD_ROT { Quaternions::Y_180 };
static const glm::vec3 DEFAULT_AVATAR_RIGHTARM_POS { -0.134824f, 0.396348f, -0.0515777f };
static const glm::quat DEFAULT_AVATAR_RIGHTARM_ROT { -0.536241f, 0.536241f, -0.460918f, -0.460918f };
static const glm::vec3 DEFAULT_AVATAR_LEFTARM_POS { 0.134795f, 0.396349f, -0.0515881f };
static const glm::quat DEFAULT_AVATAR_LEFTARM_ROT { 0.536257f, 0.536258f, -0.460899f, 0.4609f };
static const glm::vec3 DEFAULT_AVATAR_RIGHTHAND_POS { -0.72768f, 0.396349f, -0.0515779f };
static const glm::quat DEFAULT_AVATAR_RIGHTHAND_ROT { 0.479184f, -0.520013f, 0.522537f, 0.476365f};
static const glm::vec3 DEFAULT_AVATAR_LEFTHAND_POS { 0.727588f, 0.39635f, -0.0515878f };
static const glm::quat DEFAULT_AVATAR_LEFTHAND_ROT { -0.479181f, -0.52001f, 0.52254f, -0.476369f };
static const glm::vec3 DEFAULT_AVATAR_NECK_POS { 0.0f, 0.445f, 0.025f };
static const glm::vec3 DEFAULT_AVATAR_SPINE2_POS { 0.0f, 0.32f, 0.02f };
static const glm::quat DEFAULT_AVATAR_SPINE2_ROT { Quaternions::Y_180 };
static const glm::vec3 DEFAULT_AVATAR_HIPS_POS { 0.0f, 0.0f, 0.0f };
static const glm::quat DEFAULT_AVATAR_HIPS_ROT { Quaternions::Y_180 };
static const glm::vec3 DEFAULT_AVATAR_LEFTFOOT_POS { -0.08f, -0.96f, 0.029f};
static const glm::quat DEFAULT_AVATAR_LEFTFOOT_ROT { -0.40167322754859924f, 0.9154590368270874f, -0.005437685176730156f, -0.023744143545627594f };
static const glm::vec3 DEFAULT_AVATAR_RIGHTFOOT_POS { 0.08f, -0.96f, 0.029f };
static const glm::quat DEFAULT_AVATAR_RIGHTFOOT_ROT { -0.4016716778278351f, 0.9154615998268127f, 0.0053307069465518f, 0.023696165531873703f };

#endif // hifi_AvatarConstants_h
