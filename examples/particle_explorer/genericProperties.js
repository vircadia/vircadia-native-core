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
}), Vec3.multiply(2, Quat.getFront(Camera.getOrientation())));



var box = Entities.addEntity({
    type: 'Box',
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
var lastProperties;
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
        var currentProperties = Entities.getEntityProperties(box);
        boxProperties = currentProperties;
        lastProperties = currentProperties;

        // Script.update.connect(detectChangesInObject);
        Script.update.connect(sendObjectUpdates);
        Script.update.disconnect(waitForObjectAuthorization);
    }

    // function detectChangesInObject() {
    //     //would be better if there were just one kind of _dirty =  true check we could do on the object to know it needs updating without trying to do some deep object equality.

    //     var currentProperties = Entities.getEntityProperties(box);

    //     currentProperties.age = 'ignore';
    //     lastProperties.age = 'ignore';

    //     var string1 = JSON.stringify(lastProperties);
    //     var string2 = JSON.stringify(currentProperties);

    //     print('STRING1   '+string1)
    //       print('STRING2   '+string2)
    //     var sameAsBefore = string1 !== string2 ? false : true;
    //     print('same as before?? '+sameAsBefore)
    //     if (sameAsBefore === false) {
    //        // print('OBJECT HAS CHANGED, SEND UPDATED SETTINGS FROM INTERFACE')
    //        sendUpdatedSettings(currentProperties);
    //     }
    // }

    function sendObjectUpdates() {
        var currentProperties = Entities.getEntityProperties(box);

        sendUpdatedObject(currentProperties);
    }
    this.sendData = function(data) {
        // print('sending data' + JSON.stringify(data));
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
            print('SETTINGS UPDATE FROM GUI '+JSON.stringify(_data.updatedSettings));
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

function sendUpdatedObject(properties) {
    // print('SENDING UPDATED OBJECT FROM INTERFACE');
    var settings = {
        messageType: 'object_update',
        objectSettings: properties
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
    Script.update.disconnect(sendObjectUpdates);

}

Script.scriptEnding.connect(cleanup);

var settingsWindow = new SettingsWindow();
settingsWindow.init();