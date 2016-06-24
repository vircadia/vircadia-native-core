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
/*global window, alert, EventBridge, dat, listenForSettingsUpdates,createVec3Folder,createQuatFolder,writeVec3ToInterface,writeDataToInterface*/


var PARTICLE_EXPLORER_HTML_URL = Script.resolvePath('particleExplorer.html');

ParticleExplorerTool = function() {
    var that = {};

    that.createWebView = function() {
        var url = PARTICLE_EXPLORER_HTML_URL;
        that.webView = new OverlayWebWindow({
            title: 'Particle Explorer',
            source: url,
            toolWindow: true
        });

        that.webView.setVisible(true);
        that.webView.webEventReceived.connect(that.webEventReceived);        
    }


    that.destroyWebView = function() {
        if (!that.webView) {
            return;
        }

        that.webView.close();
        that.webView = null;
        that.activeParticleEntity = 0;
    }

    that.webEventReceived = function(data) {
        var data = JSON.parse(data);
        if (data.messageType === "settings_update") {
            Entities.editEntity(that.activeParticleEntity, data.updatedSettings);
        }
    }

    that.setActiveParticleEntity = function(id) {
        that.activeParticleEntity = id;
    }


    return that;


};