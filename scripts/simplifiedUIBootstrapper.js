"use strict";
/* jslint vars: true, plusplus: true */

//
//  defaultScripts.js
//
//  Authors: Zach Fox
//  Created: 2019-05-23
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var currentlyRunningScripts = ScriptDiscoveryService.getRunning();


var DEFAULT_SCRIPTS_SEPARATE = [
    "system/controllers/controllerScripts.js",
    "simplifiedUI/ui/simplifiedUI.js",
    "simplifiedUI/clickToZoom/clickToZoom.js"
];
function loadSeparateDefaults() {
    for (var i = 0; i < DEFAULT_SCRIPTS_SEPARATE.length; i++) {
        var shouldLoadCurrentDefaultScript = true;

        for (var j = 0; j < currentlyRunningScripts.length; j++) {
            var currentRunningScriptObject = currentlyRunningScripts[j];
            var currentDefaultScriptName = DEFAULT_SCRIPTS_SEPARATE[i].substr((DEFAULT_SCRIPTS_SEPARATE[i].lastIndexOf("/") + 1), DEFAULT_SCRIPTS_SEPARATE[i].length);
            if (currentDefaultScriptName === currentRunningScriptObject.name) {
                shouldLoadCurrentDefaultScript = false;
            }
        }

        if (shouldLoadCurrentDefaultScript) {
            Script.load(DEFAULT_SCRIPTS_SEPARATE[i]);
        }
    }
}


var DEFAULT_SCRIPTS_COMBINED = [
    "system/request-service.js",
    "system/progress.js",
    "system/away.js"
];
function runDefaultsTogether() {
    for (var i = 0; i < DEFAULT_SCRIPTS_COMBINED.length; i++) {
        var shouldIncludeCurrentDefaultScript = true;

        for (var j = 0; j < currentlyRunningScripts.length; j++) {
            var currentRunningScriptObject = currentlyRunningScripts[j];
            var currentDefaultScriptName = DEFAULT_SCRIPTS_COMBINED[i].substr((DEFAULT_SCRIPTS_COMBINED[i].lastIndexOf("/") + 1), DEFAULT_SCRIPTS_COMBINED[i].length);
            if (currentDefaultScriptName === currentRunningScriptObject.name) {
                shouldIncludeCurrentDefaultScript = false;
            }
        }

        if (shouldIncludeCurrentDefaultScript) {
            Script.include(DEFAULT_SCRIPTS_COMBINED[i]);
        }
    }
}



runDefaultsTogether();
loadSeparateDefaults();
