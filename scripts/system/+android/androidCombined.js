"use strict";
//
//  androidCombined.js
//  scripts/system/
//
//  Created by Gabriel Calero & Cristian Duarte on Mar 20, 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

var modesInterface = Script.require('./modes.js');
var bottombarInterface = Script.require('./bottombar.js');

function init() {
    modesInterface.isRadarModeValidTouch = bottombarInterface.isRadarModeValidTouch;// set new function
}

init();

}()); // END LOCAL_SCOPE