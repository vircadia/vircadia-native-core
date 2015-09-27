//
//  genericProperties.js
//  
//
//  Created by James B. Pollack @imgntnon 9/26/2015
//  Copyright 2014 High Fidelity, Inc.
//
//  Interface side of the App.
//  This is an example of a way to do two way bindings between dynamically created GUI and in-world entities.  
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  todo: folders, color pickers, animation settings, scale gui width with window resizing  
//
var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(0.5, Quat.getFront(Camera.getOrientation())));


var box = Entities.addEntity({
    type: 'Sphere',
    visible: true,
    collisionsWillMove: true,
    color: {
        red: 0,
        green: 255,
        blue: 0

    },
    dimensions: {
        x: 1,
        y: 1,
        z: 1,
    },
    position: center
});

var boxProperties;

SettingsWindow = function() {
    var _this = this;

    this.webWindow = null;

    this.init = function() {
        _this.webWindow = new WebWindow('genericProperties', Script.resolvePath('index.html'), 400, 600, true);
        _this.webWindow.setVisible(true);
        _this.webWindow.eventBridge.webEventReceived.connect(_this.onWebEventReceived);
        Script.update.connect(waitForObjectAuthorization)
    };

    function waitForObjectAuthorization() {
        var properties = Entities.getEntityProperties(box, "isKnownID");

        var isKnownID = properties.isKnownID
        while (isKnownID === false || _this.pageLoaded === false) {
            return;
        }
        boxProperties = properties;
        Script.update.disconnect(waitForObjectAuthorization);
    }

    this.sendData = function(data) {
        print('sending data' + JSON.stringify(data));
        _this.webWindow.eventBridge.emitScriptEvent(JSON.stringify(data));
    };
    this.onWebEventReceived = function(data) {
        // print('DATA ' + data)
        var _data = JSON.parse(data)
        if (_data.messageType === 'page_loaded') {
            print('PAGE LOADED UPDATE FROM GUI');
            _this.pageLoaded = true;
            sendInitialSettings(boxProperties);
        }
        if (_data.messageType === 'settings_update') {
            print('SETTINGS UPDATE FROM GUI');
            editEntity(_data.updatedSettings);
            return;
        }

    }
}

function sendInitialSettings(properties) {
    print('SENDING INITIAL INTERFACE SETTINGS');
    var settings = {
        messageType: 'initial_settings',
        initialSettings: properties
    };
    settingsWindow.sendData(settings)

}

function editEntity(properties) {
    Entities.editEntity(box, properties);
    var currentProperties = Entities.getEntityProperties(box);
    settingsWindow.sendData({
        messageType: 'settings_update',
        updatedSettings: currentProperties
    })
}


function cleanup() {
    Entities.deleteEntity(box);
}

Script.scriptEnding.connect(cleanup);

var settingsWindow = new SettingsWindow();
settingsWindow.init();