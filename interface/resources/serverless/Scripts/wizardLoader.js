'use strict';

//
//  wizardLoader.js
//
//  Created by Kalila L. on Feb 19 2021.
//  Copyright 2021 Vircadia contributors.
//
//  This script is used to load the onboarding wizard at the location of the entity it's on.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var CONFIG_WIZARD_WEB_URL = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Tutorial/Apps/configWizard/dist/index.html';

    var loaderEntityID;
    var configWizardEntityID;

    function onWebAppEventReceived(sendingEntityID, event) {
        if (sendingEntityID === configWizardEntityID) {
            var eventJSON = JSON.parse(event);

            if (eventJSON.command === 'first-run-wizard-ready') {
                var objectToSend = {
                    command: 'script-to-web-initialize',
                    data: {
                        performancePreset: Performance.getPerformancePreset(),
                        refreshRateProfile: Performance.getRefreshRateProfile(),
                        displayName: MyAvatar.displayName
                    }
                };
                
                Entities.emitScriptEvent(configWizardEntityID, JSON.stringify(objectToSend));
            }

            if (eventJSON.command === 'complete-wizard') {
                Performance.setPerformancePreset(eventJSON.data.performancePreset);
                Performance.setRefreshRateProfile(eventJSON.data.refreshRateProfile);
                MyAvatar.displayName = eventJSON.data.displayName;

                Entities.deleteEntity(configWizardEntityID);
                Entities.webEventReceived.disconnect(onWebAppEventReceived);
            }
        }
    }

    this.preload = function (entityID) {
        loaderEntityID = entityID;
        var loaderEntityProps = Entities.getEntityProperties(loaderEntityID, ['position', 'rotation']);

        configWizardEntityID = Entities.addEntity({
            type: 'Web',
            sourceUrl: CONFIG_WIZARD_WEB_URL,
            maxFPS: 60,
            dpi: 20,
            useBackground: false,
            grab: {
                grabbable: false
            },
            position: loaderEntityProps.position,
            rotation: loaderEntityProps.rotation,
            dimensions: { x: 1.4, y: 0.75, z: 0 }
        }, 'local');

        Entities.webEventReceived.connect(onWebAppEventReceived);
    }

    this.unload = function () {
        if (configWizardEntityID) {
            Entities.deleteEntity(configWizardEntityID);
            Entities.webEventReceived.disconnect(onWebAppEventReceived);
        }
    }

})
