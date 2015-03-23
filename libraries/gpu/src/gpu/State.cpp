//
//  State.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "State.h"
#include <QDebug>

using namespace gpu;


State::~State()
{
}

void State::set(Field field, bool value) {
    auto found = _fields.at(field);
    found._integer = value;
}

void State::set(Field field, uint32 value) {
    auto found = _fields.at(field);
    found._unsigned_integer = value;
}

void State::set(Field field, int32 value) {
    auto found = _fields.at(field);
    found._integer = value;
}

void State::set(Field field, float value) {
    auto found = _fields.at(field);
    found._float = value;
}

State::Value State::get(Field field) const {
    auto found = _fields.find(field);
    if (found != _fields.end()) {
        return (*found).second;
    }
    return Value();
}