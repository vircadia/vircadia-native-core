//
//  RegisteredMetaTypes.h
//  hifi
//
//  Created by Stephen Birarda on 10/3/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  Used to register meta-types with Qt so that they can be used as properties for objects exposed to our
//  Agent scripting.

#ifndef hifi_RegisteredMetaTypes_h
#define hifi_RegisteredMetaTypes_h

#include <glm/glm.hpp>

#include <QtScript/QScriptEngine>

#include "SharedUtil.h"

Q_DECLARE_METATYPE(glm::vec3)
Q_DECLARE_METATYPE(glm::vec2)
Q_DECLARE_METATYPE(xColor)

void registerMetaTypes(QScriptEngine* engine);

QScriptValue vec3toScriptValue(QScriptEngine* engine, const glm::vec3 &vec3);
void vec3FromScriptValue(const QScriptValue &object, glm::vec3 &vec3);

QScriptValue vec2toScriptValue(QScriptEngine* engine, const glm::vec2 &vec2);
void vec2FromScriptValue(const QScriptValue &object, glm::vec2 &vec2);

QScriptValue xColorToScriptValue(QScriptEngine* engine, const xColor& color);
void xColorFromScriptValue(const QScriptValue &object, xColor& color);

#endif
