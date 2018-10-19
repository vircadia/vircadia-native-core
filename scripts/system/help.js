"use strict";
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
//  help.js
//  scripts/system/
//
//  Created by Howard Stearns on 2 Nov 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* globals Tablet, Script, HMD, Controller, Menu */

(function () { // BEGIN LOCAL_SCOPE
var AppUi = Script.require('appUi');

var HELP_URL = Script.resourcesPath() + "html/tabletHelp.html";
var HELP_BUTTON_NAME = "HELP";
var ui;
function startup() {
    ui = new AppUi({
        buttonName: HELP_BUTTON_NAME,
        sortOrder: 6,
        home: HELP_URL
    });
}
startup();
}()); // END LOCAL_SCOPE
