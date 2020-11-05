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

Script.include('/~/system/libraries/utils.js');
var NyxSit = Script.require('./modules/sit.js');

var SETTING_NYX_PREFIX = 'nyx/';
var NYX_UI_CHANNEL = 'nyx-ui';

var nyxSettings = null;

///////////////// BEGIN ENTITY MENU OVERLAY
var enableEntityWebMenu = true;
var entityWebMenuOverlay;
var registeredEntityMenus = {};
var lastTriggeredEntityInfo = {};
var lastTriggeredPointerLocation = null;
var MENU_WEB_OVERLAY_SCALE = {
    x: 400,
    y: 500
};
var BOOTSTRAP_MENU_WEB_OVERLAY_TITLE = 'Nyx UI';
var BOOTSTRAP_MENU_WEB_OVERLAY_SCALE = {
    x: 1,
    y: 1
};
var BOOTSTRAP_MENU_WEB_OVERLAY_POSITION = {
    x: 1,
    y: 1
};
var BOOTSTRAP_MENU_WEB_OVERLAY_SOURCE;
var BOOTSTRAP_MENU_WEB_OVERLAY_DPI = 7;

function registerWithEntityMenu(messageData) {
    registeredEntityMenus[messageData.entityID] = messageData.menuItems;

    var dataToSend = {
        registeredEntityMenus: registeredEntityMenus
    };

    sendToWeb('script-to-web-registered-entity-menus', dataToSend);
}

function deregisterWithEntityMenu(messageData) {
    delete registeredEntityMenus[messageData.entityID];
    
    var dataToSend = {
        registeredEntityMenus: registeredEntityMenus
    };

    sendToWeb('script-to-web-registered-entity-menus', dataToSend);
}

function toggleEntityMenu(pressedEntityID) {
    if (!entityWebMenuOverlay.isVisible() && enableEntityWebMenu === true) {
        var triggeredEntityProperties = Entities.getEntityProperties(pressedEntityID);
        var lastEditedByName;

        if (AvatarManager.getPalData([triggeredEntityProperties.lastEditedBy]).data[0]) {
            lastEditedByName = AvatarManager.getPalData([triggeredEntityProperties.lastEditedBy]).data[0].sessionDisplayName
        } else {
            lastEditedByName = triggeredEntityProperties.lastEditedBy;
        }

        lastTriggeredEntityInfo = {
            id: pressedEntityID,
            name: triggeredEntityProperties.name,
            type: triggeredEntityProperties.type,
            lastEditedBy: triggeredEntityProperties.lastEditedBy,
            lastEditedByName: lastEditedByName,
            entityHostType: triggeredEntityProperties.entityHostType,
            description: triggeredEntityProperties.description,
            position: triggeredEntityProperties.position,
            rotation: triggeredEntityProperties.rotation
        };

        sendToWeb('script-to-web-triggered-entity-info', lastTriggeredEntityInfo);
        
        var newOverlayPosition = {
            x: (lastTriggeredPointerLocation.x - (MENU_WEB_OVERLAY_SCALE.x / 2)),
            y: (lastTriggeredPointerLocation.y - (MENU_WEB_OVERLAY_SCALE.y / 2))
        }
        
        entityWebMenuOverlay.setPosition(newOverlayPosition);
        entityWebMenuOverlay.setSize(MENU_WEB_OVERLAY_SCALE);
        entityWebMenuOverlay.setVisible(true);
    } else if (entityWebMenuOverlay.isVisible() && pressedEntityID !== entityWebMenuOverlay) {
        entityWebMenuOverlay.setVisible(false);
    }
}

///////////////// ENTITY MENU OVERLAY ---> IN-WORLD ENTITY MENU

function bootstrapEntityMenu() {
    entityWebMenuOverlay = new OverlayWebWindow({
        title: BOOTSTRAP_MENU_WEB_OVERLAY_TITLE,
        source: BOOTSTRAP_MENU_WEB_OVERLAY_SOURCE,
        width: BOOTSTRAP_MENU_WEB_OVERLAY_SCALE.x,
        height: BOOTSTRAP_MENU_WEB_OVERLAY_SCALE.y
    });
    
    entityWebMenuOverlay.setVisible(false);
    
    entityWebMenuOverlay.webEventReceived.connect(onOverlayWebEventReceived);
}

///////////////// ENTITY MENU OVERLAY ---> END IN-WORLD ENTITY MENU

///////////////// END ENTITY MENU OVERLAY

///////////////// NYX MESSAGE HANDLING

var NYX_SETTINGS_SETTINGS = 'Nyx Settings'; // lol

function sendToWeb(command, data) {
    var dataToSend = {
        'command': command,
        'data': data
    };

    entityWebMenuOverlay.emitScriptEvent(JSON.stringify(dataToSend));
}

function onOverlayWebEventReceived(event) {
    var eventJSON = JSON.parse(event);

    if (eventJSON.command === 'ready') {
        var dataToSend = {
            registeredEntityMenus: registeredEntityMenus,
            lastTriggeredEntityInfo: lastTriggeredEntityInfo
        };

        sendToWeb('script-to-web-registered-entity-menus', dataToSend);
        
        var settingsToSend = {
            settings: nyxSettings
        };

        sendToWeb('script-to-web-update-settings', settingsToSend);
    }
    
    if (eventJSON.command === 'menu-item-triggered') {
        var dataToSend = {
            command: eventJSON.command,
            entityID: eventJSON.data.triggeredEntityID,
            menuItem: eventJSON.data.menuItem
        };

        Messages.sendLocalMessage(NYX_UI_CHANNEL, JSON.stringify(dataToSend));
        
        if (entityWebMenuOverlay.isVisible()) {
            toggleEntityMenu(); // Close the menu if a menu item was pressed.
        }
    }
    
    if (eventJSON.command === 'sit-on-entity-triggered') {
        NyxSit.toggleSit();
    }
    
    if (eventJSON.command === 'close-entity-menu') {
        if (entityWebMenuOverlay.isVisible()) {
            toggleEntityMenu(); // Close the menu if it is active.
        }
    }
    
    if (eventJSON.command === 'web-to-script-settings-changed') {
        nyxSettings = eventJSON.data.settings;
        Settings.setValue(SETTING_NYX_PREFIX + NYX_SETTINGS_SETTINGS, eventJSON.data.settings);
    }
}

function onMessageReceived(channel, message, senderID, localOnly) {
    // print('NYX UI Message received:');
    // print('- channel: ' + channel);
    // print('- message: ' + message);
    // print('- sender: ' + senderID);
    // print('- localOnly: ' + localOnly);

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

function processMouseEvent (event) {
    if (!nyxSettings.entityMenu.selectedMouseTriggers) {
        return false;
    }

    for (var i = 0; i < nyxSettings.entityMenu.selectedMouseTriggers.length; i++) {
        var buttonToCheck = 'is' + nyxSettings.entityMenu.selectedMouseTriggers[i] + 'Held';

        if (event[buttonToCheck] !== true) {
            return false;
        }
    }

    return true;
}

function onMousePressOnEntity (pressedEntityID, event) {
    var currentCursorPosition = Reticle.getPosition();
    lastTriggeredPointerLocation = {
        x: currentCursorPosition.x,
        y: currentCursorPosition.y
    };

    NyxSit.capturePickPosition();
    
    if (!nyxSettings || !nyxSettings.entityMenu.useMouseTriggers || nyxSettings.entityMenu.selectedMouseTriggers.length === 0) {
        if (event.isPrimaryHeld && event.isSecondaryHeld && !isInEditMode()) {
            toggleEntityMenu(pressedEntityID);
        }
    } else if (processMouseEvent(event) === true) {
        toggleEntityMenu(pressedEntityID);
    }
}

///////////////// END NYX MESSAGE HANDLING

///////////////// BEGIN NYX MENU HANDLING

var NYX_MAIN_MENU = 'Settings > Nyx';
var NYX_ENTITY_MENU_ENABLED = 'Enable Entity Menu';
var NYX_ENTITY_MENU_RESET = 'Reset Entity Menu';

function handleMenuEvent(menuItem) {
    if (menuItem === NYX_ENTITY_MENU_ENABLED) {
        enableEntityWebMenu = Menu.isOptionChecked(NYX_ENTITY_MENU_ENABLED);
        Settings.setValue(SETTING_NYX_PREFIX + NYX_ENTITY_MENU_ENABLED, enableEntityWebMenu);
    }
    
    if (menuItem === NYX_ENTITY_MENU_RESET) {
        nyxSettings = null;
        Settings.setValue(SETTING_NYX_PREFIX + NYX_SETTINGS_SETTINGS, '');
    }
}

function bootstrapNyxMenu() {
    if (!Menu.menuExists(NYX_MAIN_MENU)) {
        Menu.addMenu(NYX_MAIN_MENU);
        Menu.addMenuItem({
            menuName: NYX_MAIN_MENU,
            menuItemName: NYX_ENTITY_MENU_ENABLED,
            isCheckable: true,
            isChecked: Settings.getValue(SETTING_NYX_PREFIX + NYX_ENTITY_MENU_ENABLED, true)
        });
        Menu.addMenuItem({
            menuName: NYX_MAIN_MENU,
            menuItemName: NYX_ENTITY_MENU_RESET,
            isCheckable: false
        });
    }

    enableEntityWebMenu = Settings.getValue(SETTING_NYX_PREFIX + NYX_ENTITY_MENU_ENABLED, true);
}

function unloadNyxMenu() {
    Settings.setValue(SETTING_NYX_PREFIX + NYX_ENTITY_MENU_ENABLED, enableEntityWebMenu);
    
    Menu.removeMenuItem(NYX_MAIN_MENU, NYX_ENTITY_MENU_ENABLED);
    Menu.removeMenuItem(NYX_MAIN_MENU, NYX_ENTITY_MENU_RESET);
    Menu.removeMenu(NYX_MAIN_MENU);
}

///////////////// END NYX MENU HANDLING

///////////////// BOOTSTRAPPING

function startup() {
    Messages.messageReceived.connect(onMessageReceived);
    Entities.mousePressOnEntity.connect(onMousePressOnEntity);
    Menu.menuItemEvent.connect(handleMenuEvent);
    
    nyxSettings = Settings.getValue(SETTING_NYX_PREFIX + NYX_SETTINGS_SETTINGS, '');
    
    BOOTSTRAP_MENU_WEB_OVERLAY_SOURCE = Script.resolvePath('./index.html');
    bootstrapNyxMenu();
    bootstrapEntityMenu();
}

startup();

Script.scriptEnding.connect(function () {
    Messages.messageReceived.disconnect(onMessageReceived);
    Entities.mousePressOnEntity.disconnect(onMousePressOnEntity);
    Menu.menuItemEvent.disconnect(handleMenuEvent);
    
    entityWebMenuOverlay.webEventReceived.disconnect(onOverlayWebEventReceived);

    unloadNyxMenu();

    entityWebMenuOverlay = null;
});

///////////////// BOOTSTRAPPING TESTING CODE

// var messageToSend = {
//     'command': 'register-with-entity-menu',
//     'entityID': '{768542d0-e962-49e3-94fb-85651d56f5ae}',
//     'menuItems': ['This', 'Is', 'Nice']
// };
// 
// Messages.sendLocalMessage(NYX_UI_CHANNEL, JSON.stringify(messageToSend));