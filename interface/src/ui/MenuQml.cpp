//
//  MenuQml.cpp
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "MenuQml.h"

QML_DIALOG_DEF(MenuQml)

MenuQml::MenuQml(QQuickItem *parent) : QQuickItem(parent) {
    auto menu = Menu::getInstance();
   
}
