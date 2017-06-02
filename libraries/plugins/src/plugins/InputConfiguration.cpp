//
//  Created by Dante Ruiz on 6/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "InputConfiguration.h"

#include "PluginManager.h"

InputConfiguration::InputConfiguration() {
        
}

void InputConfiguration::inputPlugins() {
    PluginManager* inputPlugin = PluginManager::getInstance();
}

void InputConfiguration::enabledInputPlugins() {
    qDebug() << "getting enabled plugins";
}


