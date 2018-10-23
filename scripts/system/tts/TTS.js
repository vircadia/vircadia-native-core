"use strict";
/*jslint vars:true, plusplus:true, forin:true*/
/*global Tablet, Script,  */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
// TTS.js
//
// Created by Zach Fox on 2018-10-10
// Copyright 2018 High Fidelity, Inc
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () { // BEGIN LOCAL_SCOPE
var AppUi = Script.require('appUi');

var ui;
function startup() {
    ui = new AppUi({
        buttonName: "TTS",
        //home: Script.resolvePath("TTS.qml")
        home: "hifi/tts/TTS.qml",
        graphicsDirectory: Script.resolvePath("./") // speech by Danil Polshin from the Noun Project
    });
}
startup();
}()); // END LOCAL_SCOPE
