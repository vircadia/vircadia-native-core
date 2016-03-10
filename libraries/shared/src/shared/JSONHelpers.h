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
QJsonValue toJsonValue(const vec3& v);
QJsonValue toJsonValue(const vec4& v);
QJsonValue toJsonValue(const QObject& o);

quat quatFromJsonValue(const QJsonValue& q);
vec3 vec3FromJsonValue(const QJsonValue& v);
vec4 vec4FromJsonValue(const QJsonValue& v);
void qObjectFromJsonValue(const QJsonValue& j, QObject& o);

#endif
