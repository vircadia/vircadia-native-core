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

var DopplegangerClass = Script.require('./doppleganger.js');

var tablet = Tablet.getTablet('com.highfidelity.interface.tablet.system'),
    button = tablet.addButton({
        icon: Script.resolvePath('./doppleganger-i.svg'),
        activeIcon: Script.resolvePath('./doppleganger-a.svg'),
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

// hide the doppleganger if this client script is unloaded
Script.scriptEnding.connect(doppleganger, 'stop');

// hide the doppleganger if the user switches domains (which might place them arbitrarily far away in world space)
function onDomainChanged() {
    if (doppleganger.active) {
        doppleganger.stop('domain_changed');
    }
}
Window.domainChanged.connect(onDomainChanged);
Window.domainConnectionRefused.connect(onDomainChanged);
Script.scriptEnding.connect(function() {
    Window.domainChanged.disconnect(onDomainChanged);
    Window.domainConnectionRefused.disconnect(onDomainChanged);
});

// toggle on/off via tablet button
button.clicked.connect(doppleganger, 'toggle');

// highlight tablet button based on current doppleganger state
doppleganger.activeChanged.connect(function(active, reason) {
    if (button) {
        button.editProperties({ isActive: active });
        print('doppleganger.activeChanged', active, reason);
    }
});

// alert the user if there was an error applying their skeletonModelURL
doppleganger.modelOverlayLoaded.connect(function(error, result) {
    if (doppleganger.active && error) {
        Window.alert('doppleganger | ' + error + '\n' + doppleganger.skeletonModelURL);
    }
});

// add debug indicators, but only if the user has configured the settings value
if (Settings.getValue('debug.doppleganger', false)) {
    DopplegangerClass.addDebugControls(doppleganger);
}

UserActivityLogger.logAction('doppleganger_app_load');
doppleganger.activeChanged.connect(function(active, reason) {
    if (active) {
        UserActivityLogger.logAction('doppleganger_enable');
    } else {
        if (reason === 'stop') {
            // user intentionally toggled the doppleganger
            UserActivityLogger.logAction('doppleganger_disable');
        } else {
            print('doppleganger stopped:', reason);
            UserActivityLogger.logAction('doppleganger_autodisable', { reason: reason });
        }
    }
});
