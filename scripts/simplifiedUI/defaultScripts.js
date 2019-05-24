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
var DEFAULT_SCRIPTS_PATH_PREFIX = ScriptDiscoveryService.defaultScriptsPath + "/";


var DEFAULT_SCRIPTS_SEPARATE = [
    DEFAULT_SCRIPTS_PATH_PREFIX + "system/controllers/controllerScripts.js",
    Script.resolvePath("simplifiedUI.js")
];
function loadSeparateDefaults() {
    for (var i in DEFAULT_SCRIPTS_SEPARATE) {
        Script.load(DEFAULT_SCRIPTS_SEPARATE[i]);
    }
}


var DEFAULT_SCRIPTS_COMBINED = [
    DEFAULT_SCRIPTS_PATH_PREFIX + "system/request-service.js",
    DEFAULT_SCRIPTS_PATH_PREFIX + "system/progress.js",
    DEFAULT_SCRIPTS_PATH_PREFIX + "system/away.js"
];
function runDefaultsTogether() {
    for (var i in DEFAULT_SCRIPTS_COMBINED) {
        Script.include(DEFAULT_SCRIPTS_COMBINED[i]);
    }
    loadSeparateDefaults();
}


runDefaultsTogether();
