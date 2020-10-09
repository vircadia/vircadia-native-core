'use strict';

//
//  nyx.js
//
//  Created by Kalila L. on Oct 6 2020.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// BEGIN ENTITY MENU OVERLAY

var entityWebMenu;
var entityWebMenuActive = false;
var registeredEntityMenus = {};
var lastTriggeredEntityInfo = {};
var MENU_WEB_ENTITY_SCALE = {
    x: 1,
    y: 1.5,
    z: 0.01
};
var BOOTSTRAP_MENU_WEB_ENTITY_SCALE = {
    x: 0.01,
    y: 0.01,
    z: 0.01
};
var BOOTSTRAP_MENU_WEB_ENTITY_POSITION = Vec3.ZERO;
var BOOTSTRAP_MENU_WEB_ENTITY_SOURCE = Script.resolvePath("./index.html");
var BOOTSTRAP_MENU_WEB_ENTITY_DPI = 7;
var NYX_UI_CHANNEL = "nyx-ui";

function registerWithEntityMenu(messageData) {
    registeredEntityMenus[messageData.entityID] = messageData.menuItems;

    var dataToSend = {
        registeredEntityMenus: registeredEntityMenus
    };

    sendToWeb(entityWebMenu, 'script-to-web-registered-entity-menus', dataToSend);
}

function deregisterWithEntityMenu(messageData) {
    delete registeredEntityMenus[messageData.entityID];
    
    var dataToSend = {
        registeredEntityMenus: registeredEntityMenus
    };

    sendToWeb(entityWebMenu, 'script-to-web-registered-entity-menus', dataToSend);
}

function toggleEntityMenu(pressedEntityID) {
    if (!entityWebMenuActive) {
        var triggeredEntityProperties = Entities.getEntityProperties(pressedEntityID);
        
        lastTriggeredEntityInfo = {
            id: pressedEntityID,
            name: triggeredEntityProperties.name,
            lastEditedBy: triggeredEntityProperties.lastEditedBy,
            lastEditedByName: AvatarManager.getPalData([triggeredEntityProperties.lastEditedBy]).data[0].sessionDisplayName
        };

        sendToWeb(entityWebMenu, 'script-to-web-triggered-entity-info', lastTriggeredEntityInfo);

        Entities.editEntity(entityWebMenu, {
            position: Entities.getEntityProperties(pressedEntityID, ['position']).position,
            dimensions: MENU_WEB_ENTITY_SCALE,
            visible: true
        });
        
        entityWebMenuActive = true;
    } else if (entityWebMenuActive && pressedEntityID !== entityWebMenu) {
        Entities.editEntity(entityWebMenu, {
            position: BOOTSTRAP_MENU_WEB_ENTITY_POSITION,
            dimensions: BOOTSTRAP_MENU_WEB_ENTITY_SCALE
        });
        
        entityWebMenuActive = false;
    }
}

function bootstrapEntityMenu() {
    entityWebMenu = Entities.addEntity({
        type: "Web",
        billboardMode: 'full',
        renderLayer: 'front',
        visible: false,
        grab: {
            'grabbable': false
        },
        sourceUrl: BOOTSTRAP_MENU_WEB_ENTITY_SOURCE,
        position: BOOTSTRAP_MENU_WEB_ENTITY_POSITION,
        dimensions: BOOTSTRAP_MENU_WEB_ENTITY_SCALE,
        dpi: BOOTSTRAP_MENU_WEB_ENTITY_DPI
    }, 'local');
}

// END ENTITY MENU OVERLAY

function sendToWeb(sendToEntity, command, data) {
    var dataToSend = {
        "command": command,
        "data": data
    };

    Entities.emitScriptEvent(sendToEntity, JSON.stringify(dataToSend));
}

function onWebEventReceived(sendingEntityID, event) {
    if (sendingEntityID === entityWebMenu) {
        var eventJSON = JSON.parse(event);

        if (eventJSON.command === "ready") {
            var dataToSend = {
                registeredEntityMenus: registeredEntityMenus,
                lastTriggeredEntityInfo: lastTriggeredEntityInfo
            };

            sendToWeb(entityWebMenu, 'script-to-web-registered-entity-menus', dataToSend);
        }
        
        if (eventJSON.command === "menu-item-triggered") {
            var dataToSend = {
                entityID: eventJSON.data.triggeredEntityID,
                itemPressed: eventJSON.data.itemPressed
            };

            Messages.sendLocalMessage(NYX_UI_CHANNEL, JSON.stringify(dataToSend));
        }
        
    }
}

function onMessageReceived(channel, message, senderID, localOnly) {
    print("NYX UI Message received:");
    print("- channel: " + channel);
    print("- message: " + message);
    print("- sender: " + senderID);
    print("- localOnly: " + localOnly);

    if (channel === NYX_UI_CHANNEL && MyAvatar.sessionUUID === senderID) {
        messageData = JSON.parse(message);
        
        if (messageData.command === 'register-with-entity-menu') {
            registerWithEntityMenu(messageData);
        }
        
        if (messageData.command === 'deregister-with-entity-menu') {
            deregisterWithEntityMenu(messageData);
        }
    }
}

function onMousePressOnEntity (pressedEntityID, event) {
    if (event.isPrimaryButton) {
        toggleEntityMenu(pressedEntityID);
    }
}

// BOOTSTRAPPING

function startup() {
    Messages.messageReceived.connect(onMessageReceived);
    Entities.mousePressOnEntity.connect(onMousePressOnEntity);
    Entities.webEventReceived.connect(onWebEventReceived);
    
    bootstrapEntityMenu();
}

startup();

Script.scriptEnding.connect(function () {
    Messages.messageReceived.disconnect(onMessageReceived);
    Entities.mousePressOnEntity.disconnect(onMousePressOnEntity);
    Entities.webEventReceived.disconnect(onWebEventReceived);

    Entities.deleteEntity(entityWebMenu);
    entityWebMenu = null;
});

// BOOTSTRAPPING TESTING CODE

var messageToSend = {
    'command': 'register-with-entity-menu',
    'entityID': '{768542d0-e962-49e3-94fb-85651d56f5ae}',
    'menuItems': ['This', 'Is', 'Nice']
};

Messages.sendLocalMessage(NYX_UI_CHANNEL, JSON.stringify(messageToSend));