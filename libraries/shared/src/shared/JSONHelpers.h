//
//  Created by Bradley Austin Davis on 2015/11/09
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Shared_JSONHelpers_h
#define hifi_Shared_JSONHelpers_h

#include "../GLMHelpers.h"

QJsonValue toJsonValue(const quat& q);

QJsonValue toJsonValue(const vec3& q);

quat quatFromJsonValue(const QJsonValue& q);

vec3 vec3FromJsonValue(const QJsonValue& q);

#endif
