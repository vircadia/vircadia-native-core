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
#include "oculus/OculusDisplayPlugin.h"
#include "oculus/OculusDebugDisplayPlugin.h"
#include "oculus/OculusLegacyDisplayPlugin.h"

const QString& DisplayPlugin::MENU_PATH() {
    static const QString value = "Display";
    return value;
}

// TODO migrate to a DLL model where plugins are discovered and loaded at runtime by the PluginManager class
DisplayPluginList getDisplayPlugins() {
    DisplayPlugin* PLUGIN_POOL[] = {
        new Basic2DWindowOpenGLDisplayPlugin(),
#ifdef DEBUG
        new NullDisplayPlugin(),
#endif

        // Stereo modes

        // SBS left/right
        new SideBySideStereoDisplayPlugin(),
        // Interleaved left/right
        new InterleavedStereoDisplayPlugin(),

        // HMDs

        // Windows Oculus SDK
        new OculusDisplayPlugin(),
        // Windows Oculus Simulator... uses head tracking and the same rendering 
        // as the connected hardware, but without using the SDK to display to the 
        // Rift.  Useful for debugging Rift performance with nSight.
        new OculusDebugDisplayPlugin(),
        // Mac/Linux Oculus SDK (0.5)
        new OculusLegacyDisplayPlugin(),
#ifdef Q_OS_WIN
        // SteamVR SDK
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
