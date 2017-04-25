"use strict";

//  doppleganger-app.js
//
//  Created by Timothy Dedischew on 04/21/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  This Client script creates can instance of a Doppleganger that can be toggled on/off via tablet button.
//  (for more info see doppleganger.js)
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global */

var DopplegangerClass = Script.require('./doppleganger.js');
// uncomment the next line to sync via Script.update (instead of Script.setInterval)
// DopplegangerClass.USE_SCRIPT_UPDATE = true;

var tablet = Tablet.getTablet('com.highfidelity.interface.tablet.system'),
    button = tablet.addButton({
        icon: "icons/tablet-icons/doppleganger-i.svg",
        activeIcon: "icons/tablet-icons/doppleganger-a.svg",
        text: 'MIRROR'
    });

Script.scriptEnding.connect(function() {
    tablet.removeButton(button);
});

var doppleganger = new DopplegangerClass({
    avatar: MyAvatar,
    mirrored: true,
    eyeToEye: true,
    autoUpdate: true
});
Script.scriptEnding.connect(doppleganger, 'cleanup');

if (Settings.getValue('debug.doppleganger', false)) {
    DopplegangerClass.addDebugControls(doppleganger);
}

button.clicked.connect(function() {
    doppleganger.toggle();
    button.editProperties({ isActive: doppleganger.active });
});

