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
/* global window, alert, ParticleExplorerTool, EventBridge, dat, listenForSettingsUpdates,createVec3Folder,createQuatFolder,writeVec3ToInterface,writeDataToInterface*/


var PARTICLE_EXPLORER_HTML_URL = Script.resolvePath('particleExplorer.html');

ParticleExplorerTool = function() {
    var that = {};
    that.createWebView = function() {
        that.webView = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        that.webView.setVisible = function(value) {};
        that.webView.webEventReceived.connect(that.webEventReceived);
    }

    that.destroyWebView = function() {
        if (!that.webView) {
            return;
        }
        that.activeParticleEntity = 0;

        var messageData = {
            messageType: "particle_close"
        };
        that.webView.emitScriptEvent(JSON.stringify(messageData));
    }

    that.webEventReceived = function(data) {
        var data = JSON.parse(data);
        if (data.messageType === "settings_update") {
            if (data.updatedSettings.emitOrientation) {
                data.updatedSettings.emitOrientation = Quat.fromVec3Degrees(data.updatedSettings.emitOrientation);
            }
            Entities.editEntity(that.activeParticleEntity, data.updatedSettings);
        }
    }

    that.setActiveParticleEntity = function(id) {
        that.activeParticleEntity = id;
    }

    return that;
};
