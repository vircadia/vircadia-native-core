"use strict";

//  doppleganger-app.js
//
//  Created by Timothy Dedischew on 04/21/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  This tablet app creates a mirrored projection of your avatar (ie: a "doppleganger") that you can walk around
//  and inspect.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global */

var TABLET_APP_ICON = Script.resolvePath('Spiegel-lineart-black.svg');
var TABLET_APP_NAME = 'mirror';

var EYE_TO_EYE = false; // whether to maintain the doppleganger's relative vertical positioning
var DEBUG = true;
var MIRRORED = true; // whether to mirror joints or simply transfer them as-is

var tablet = Tablet.getTablet('com.highfidelity.interface.tablet.system');
var button = tablet.addButton({
    icon: TABLET_APP_ICON,
    text: TABLET_APP_NAME
});

var DopplegangerClass = Script.require('./doppleganger.js#'+ new Date().getTime().toString(36));

var doppleganger = new DopplegangerClass({
    avatar: MyAvatar,
    mirrored: MIRRORED,
    debug: DEBUG,
    eyeToEye: EYE_TO_EYE,
});

button.clicked.connect(function() {
    print('click', doppleganger.active);
    doppleganger.toggle();
    button.editProperties({ isActive: doppleganger.active });
});

Script.scriptEnding.connect(function() {
    try {
        doppleganger.shutdown();
    } finally {
        // we want to remove the button even if an error is thrown during shutdown
        tablet.removeButton(button);
    }
});
