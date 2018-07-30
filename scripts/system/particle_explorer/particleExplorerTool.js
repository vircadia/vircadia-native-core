//
//  particleExplorerTool.js
//
//  Created by Eric Levin on 2/15/16
//  Copyright 2016 High Fidelity, Inc.
//  Adds particleExplorer tool to the edit panel when a user selects a particle entity from the edit tool window
//  This is an example of a new, easy way to do two way bindings between dynamically created GUI and in-world entities.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* global ParticleExplorerTool */


var PARTICLE_EXPLORER_HTML_URL = Script.resolvePath('particleExplorer.html');

ParticleExplorerTool = function(createToolsWindow) {
    var that = {};
    that.activeParticleEntity = 0;
    that.updatedActiveParticleProperties = {};

    that.createWebView = function() {
        that.webView = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        that.webView.setVisible = function(value) {};
        that.webView.webEventReceived.connect(that.webEventReceived);
        createToolsWindow.webEventReceived.addListener(this, that.webEventReceived);
    };

    function emitScriptEvent(data) {
        var messageData = JSON.stringify(data);
        that.webView.emitScriptEvent(messageData);
        createToolsWindow.emitScriptEvent(messageData);
    }

    that.destroyWebView = function() {
        if (!that.webView) {
            return;
        }
        that.activeParticleEntity = 0;
        that.updatedActiveParticleProperties = {};

        emitScriptEvent({
            messageType: "particle_close"
        });
    };

    function sendParticleProperties(properties) {
        emitScriptEvent({
            messageType: "particle_settings",
            currentProperties: properties
        });
    }

    function sendActiveParticleProperties() {
        var properties = Entities.getEntityProperties(that.activeParticleEntity);
        if (properties.emitOrientation) {
            properties.emitOrientation = Quat.safeEulerAngles(properties.emitOrientation);
        }
        // Update uninitialized variables
        if (isNaN(properties.alphaStart)) {
            properties.alphaStart = properties.alpha;
        }
        if (isNaN(properties.alphaFinish)) {
            properties.alphaFinish = properties.alpha;
        }
        if (isNaN(properties.radiusStart)) {
            properties.radiusStart = properties.particleRadius;
        }
        if (isNaN(properties.radiusFinish)) {
            properties.radiusFinish = properties.particleRadius;
        }
        if (isNaN(properties.colorStart.red)) {
            properties.colorStart = properties.color;
        }
        if (isNaN(properties.colorFinish.red)) {
            properties.colorFinish = properties.color;
        }
        if (isNaN(properties.spinStart)) {
            properties.spinStart = properties.particleSpin;
        }
        if (isNaN(properties.spinFinish)) {
            properties.spinFinish = properties.particleSpin;
        }
        sendParticleProperties(properties);
    }
    
    function sendUpdatedActiveParticleProperties() {
        sendParticleProperties(that.updatedActiveParticleProperties);
        that.updatedActiveParticleProperties = {};
    }

    that.webEventReceived = function(message) {
        var data = JSON.parse(message);
        if (data.messageType === "settings_update") {
            var updatedSettings = data.updatedSettings;

            var optionalProps = ["alphaStart", "alphaFinish", "radiusStart", "radiusFinish", "colorStart", "colorFinish", "spinStart", "spinFinish"];
            var fallbackProps = ["alpha", "particleRadius", "color", "particleSpin"];
            for (var i = 0; i < optionalProps.length; i++) {
                var fallbackProp = fallbackProps[Math.floor(i / 2)];
                var optionalValue = updatedSettings[optionalProps[i]];
                var fallbackValue = updatedSettings[fallbackProp];
                if (optionalValue && fallbackValue) {
                    delete updatedSettings[optionalProps[i]];
                }
            }
            
            if (updatedSettings.emitOrientation) {
                updatedSettings.emitOrientation = Quat.fromVec3Degrees(updatedSettings.emitOrientation);
            }
            
            Entities.editEntity(that.activeParticleEntity, updatedSettings);
                    
            var entityProps = Entities.getEntityProperties(that.activeParticleEntity, optionalProps);
            
            var needsUpdate = false;
            for (var i = 0; i < optionalProps.length; i++) {
                var fallbackProp = fallbackProps[Math.floor(i / 2)];
                var fallbackValue = updatedSettings[fallbackProp];
                if (fallbackValue) {
                    var optionalProp = optionalProps[i];
                    if ((fallbackProp !== "color" && isNaN(entityProps[optionalProp])) || (fallbackProp === "color" && isNaN(entityProps[optionalProp].red))) {
                        that.updatedActiveParticleProperties[optionalProp] = fallbackValue;
                        needsUpdate = true;
                    }
                }
            }

            if (needsUpdate) {
                sendUpdatedActiveParticleProperties();
            }
            
        } else if (data.messageType === "page_loaded") {
            sendActiveParticleProperties();
        }
    };

    that.setActiveParticleEntity = function(id) {
        that.activeParticleEntity = id;
        sendActiveParticleProperties();
    };
    
    return that;
};
