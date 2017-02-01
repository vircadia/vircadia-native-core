//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <vector>
#include <memory>

enum class PluginType {
    DISPLAY_PLUGIN,
    INPUT_PLUGIN,
    CODEC_PLUGIN,
};

class DisplayPlugin;
class InputPlugin;
class CodecPlugin;
class SteamClientPlugin;
class Plugin;
class PluginContainer;
class PluginManager;

using DisplayPluginPointer = std::shared_ptr<DisplayPlugin>;
using DisplayPluginList = std::vector<DisplayPluginPointer>;
using InputPluginPointer = std::shared_ptr<InputPlugin>;
using InputPluginList = std::vector<InputPluginPointer>;
using CodecPluginPointer = std::shared_ptr<CodecPlugin>;
using CodecPluginList = std::vector<CodecPluginPointer>;
using SteamClientPluginPointer = std::shared_ptr<SteamClientPlugin>;
