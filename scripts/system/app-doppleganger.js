"use strict";

//  doppleganger-app.js
//
//  Created by Timothy Dedischew on 04/21/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  This Client script creates an instance of a Doppleganger that can be toggled on/off via tablet button.
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
    button = null;
});

var doppleganger = new DopplegangerClass({
    avatar: MyAvatar,
    mirrored: true,
    autoUpdate: true
});
Script.scriptEnding.connect(doppleganger, 'stop');

doppleganger.activeChanged.connect(function(active) {
    if (button) {
        button.editProperties({ isActive: active });
    }
});

doppleganger.modelOverlayLoaded.connect(function(error, result) {
    if (doppleganger.active && error) {
        Window.alert('doppleganger | ' + error + '\n' + doppleganger.skeletonModelURL);
    }
});

button.clicked.connect(doppleganger, 'toggle');

if (Settings.getValue('debug.doppleganger', false)) {
    DopplegangerClass.addDebugControls(doppleganger);
}
