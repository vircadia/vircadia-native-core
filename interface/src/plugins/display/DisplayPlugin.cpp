//
//  DisplayPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "DisplayPlugin.h"

bool DisplayPlugin::isMouseOnScreen() const {
    glm::ivec2 mousePosition = getTrueMousePosition();
    return (glm::all(glm::greaterThanEqual(mousePosition, glm::ivec2(0))) &&
        glm::all(glm::lessThanEqual(mousePosition, glm::ivec2(getCanvasSize()))));
}