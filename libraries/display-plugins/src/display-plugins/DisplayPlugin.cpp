//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "DisplayPlugin.h"

#include <plugins/PluginManager.h>

#include "NullDisplayPlugin.h"
#include "stereo/SideBySideStereoDisplayPlugin.h"
#include "stereo/InterleavedStereoDisplayPlugin.h"
#include "Basic2DWindowOpenGLDisplayPlugin.h"

#include "openvr/OpenVrDisplayPlugin.h"
#include "oculus/Oculus_0_5_DisplayPlugin.h"
#include "oculus/Oculus_0_6_DisplayPlugin.h"

// TODO migrate to a DLL model where plugins are discovered and loaded at runtime by the PluginManager class
DisplayPluginList getDisplayPlugins() {
    DisplayPlugin* PLUGIN_POOL[] = {
        new Basic2DWindowOpenGLDisplayPlugin(),
#ifdef DEBUG
        new NullDisplayPlugin(),
#endif

        // Stereo modes
        // FIXME fix stereo display plugins
        new SideBySideStereoDisplayPlugin(),
        //new InterleavedStereoDisplayPlugin(),

        // HMDs
        new Oculus_0_5_DisplayPlugin(),
        new Oculus_0_6_DisplayPlugin(),
#ifdef Q_OS_WIN
        new OpenVrDisplayPlugin(),
#endif
        nullptr
    };

    DisplayPluginList result;
    for (int i = 0; PLUGIN_POOL[i]; ++i) {
        DisplayPlugin * plugin = PLUGIN_POOL[i];
        if (plugin->isSupported()) {
            plugin->init();
            result.push_back(DisplayPluginPointer(plugin));
        }
    }
    return result;
}
