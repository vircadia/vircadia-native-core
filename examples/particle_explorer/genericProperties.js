//
//  genericProperties.js
//  
//
//  Created by James B. Pollack @imgntnon 9/26/2015
//  Copyright 2014 High Fidelity, Inc.
//
//  Interface side of the App.
// This is an example of a new, easy way to do two way bindings between dynamically created GUI and in-world entities.  
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  todo: folders, color pickers, animation settings, scale gui width with window resizing  
//

SettingsWindow = function() {
    var _this = this;
    this.webWindow = null;
    this.init = function() {
        _this.webWindow = new WebWindow('genericProperties', Script.resolvePath('index.html'), 400, 600, true);
        _this.webWindow.setVisible(true);
        _this.webWindow.eventBridge.webEventReceived.connect(_this.onWebEventReceived);

        var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
            x: 0,
            y: 0.5,
            z: 0
        }), Vec3.multiply(0.5, Quat.getFront(Camera.getOrientation())));


        _this.box = Entities.addEntity({
            type: 'Box',
            shapeType: 'box',
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

    };
    this.sendData = function(data) {
        print('sending data' + JSON.stringify(data));
        _this.webWindow.eventBridge.emitScriptEvent(JSON.stringify(data));
    };
    this.onWebEventReceived = function(data) {
        // print('DATA ' + data)
        var _data = JSON.parse(data)
        if (_data.type !== 'settings_update') {
            return;
        }
        print('GOT A SETTINGS UPDATE EVENT')
        editEntity(_data.updatedSettings)

    }


}

function sendInitialSettings() {


    var settings = {
        messageType: 'initialSettings',
        initialSettings: Entities.getEntityProperties(SettingsWindow.box)
    }
    settingsWindow.sendData(settings)



}

function editEntity(properties) {
    Entities.editEntity(SettingsWindow.box, properties);
    var currentProperties = Entities.getEntityProperties(SettingsWindow.box);
    settingsWindow.sendData({
        messageType: 'settingsUpdate',
        updatedSettings: currentProperties
    })
}


var settingsWindow = new SettingsWindow();
settingsWindow.init();
Script.setTimeout(function() {
    sendInitialSettings();
}, 1000)


function cleanup() {
    Entities.deleteEntity(testParticles);
    Entities.deleteEntity(box);
}
Script.scriptEnding.connect(cleanup);