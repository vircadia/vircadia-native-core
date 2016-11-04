//  PluginUtils.h
//  input-plugins/src/input-plugins
//
//  Created by Ryan Huffman on 9/22/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <QString>

class PluginUtils {
public:
    static bool isHMDAvailable(const QString& pluginName = "");
    static bool isHandControllerAvailable();
    static bool isViveControllerAvailable();
    static bool isXboxControllerAvailable();
};
