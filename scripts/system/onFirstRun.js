'use strict';

//
//  onFirstRun.js
//
//  Created by Kalila L. on Oct 5 2020.
//  Copyright 2020 Vircadia contributors.
//
//  This script triggers on first run to perform bootstrapping actions.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE
    // Check to see if we should run this script or bail...
    var SETTING_TO_CHECK = 'firstRun';
    
    if (!Settings.getValue(SETTING_TO_CHECK, false)) {
        return;
    }
    
    // If this is our first run then proceed...

    var configWizardEntityID;
    var DEFAULT_DISPLAY_NAME = '';
    
    if (!PlatformInfo.has3DHTML()) {
        if (MyAvatar.displayName === '') {
            var selectedDisplayName = Window.prompt('Enter a display name.', MyAvatar.displayName);
            setDisplayName(selectedDisplayName);
        }
    } else {
        configWizardEntityID = Entities.addEntity({
            type: 'Web',
            sourceUrl: Script.resolvePath("./configWizard/dist/index.html"),
            maxFPS: 60,
            dpi: 20,
            useBackground: false,
            grab: {
                grabbable: false
            },
            position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -2.25 })),
            rotation: MyAvatar.orientation,
            dimensions: { x: 1.4, y: 0.75, z: 0 }
        }, 'local');

        Entities.webEventReceived.connect(onWebAppEventReceived);
    }
    
    function setDisplayName(displayName) {
        if (displayName === '') {
            MyAvatar.displayName = DEFAULT_DISPLAY_NAME;
        } else {
            MyAvatar.displayName = displayName;
        }
    }
    
    function onWebAppEventReceived(sendingEntityID, event) {
        if (sendingEntityID === configWizardEntityID) {
            var eventJSON = JSON.parse(event);
            if (eventJSON.command === "complete-wizard") {
                Performance.setPerformancePreset(eventJSON.data.performancePreset);
                Performance.setRefreshRateProfile(eventJSON.data.refreshRateProfile);
                setDisplayName(eventJSON.data.displayName);
                Entities.deleteEntity(configWizardEntityID);
                Entities.webEventReceived.disconnect(onWebAppEventReceived);
            }
        }
    }
    
    Script.scriptEnding.connect(function () {
        if (configWizardEntityID) {
            Entities.deleteEntity(configWizardEntityID);
            Entities.webEventReceived.disconnect(onWebAppEventReceived);
        }
    });
}());
