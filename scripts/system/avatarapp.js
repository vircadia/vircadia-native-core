"use strict";
/*jslint vars:true, plusplus:true, forin:true*/
/*global Tablet, Settings, Script, AvatarList, Users, Entities, MyAvatar, Camera, Overlays, Vec3, Quat, HMD, Controller, Account, UserActivityLogger, Messages, Window, XMLHttpRequest, print, location, getControllerWorldLocation*/
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
// avatarapp.js
//
// Created by Alexander Ivash on April 30, 2018
// Copyright 2016 High Fidelity, Inc
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

var request = Script.require('request').request;
var AVATARAPP_QML_SOURCE = "hifi/AvatarApp.qml";
Script.include("/~/system/libraries/controllers.js");

// constants from AvatarBookmarks.h
var ENTRY_AVATAR_URL = "avatarUrl";
var ENTRY_AVATAR_ATTACHMENTS = "attachments";
var ENTRY_AVATAR_ENTITIES = "avatarEntites";
var ENTRY_AVATAR_SCALE = "avatarScale";
var ENTRY_VERSION = "version";

function getMyAvatarWearables() {
    var wearablesArray = MyAvatar.getAvatarEntitiesVariant();

    console.debug('avatarapp.js: getMyAvatarWearables(): wearables count: ', wearablesArray.length);
    for(var i = 0; i < wearablesArray.length; ++i) {
        var wearable = wearablesArray[i];
        console.debug('updating localRotationAngles for wearable: ',
                      wearable.properties.id,
                      wearable.properties.itemName,
                      wearable.properties.parentID,
                      wearable.properties.owningAvatarID,
                      isGrabbable(wearable.properties.id));

        var localRotation = wearable.properties.localRotation;
        wearable.properties.localRotationAngles = Quat.safeEulerAngles(localRotation)
    }

    return wearablesArray;

    /*
    var getAttachedModelEntities = function() {
        var resultEntities = [];
        Entities.findEntitiesByType('Model', MyAvatar.position, 100).forEach(function(entityID) {
            if (isEntityBeingWorn(entityID)) {
                resultEntities.push({properties : entityID});
            }
        });
        return resultEntities;
    };

    return getAttachedModelEntities();
    */
}

function getMyAvatar() {
    var avatar = {}
    avatar[ENTRY_AVATAR_URL] = MyAvatar.skeletonModelURL;
    avatar[ENTRY_AVATAR_SCALE] = MyAvatar.getAvatarScale();
    avatar[ENTRY_AVATAR_ATTACHMENTS] = MyAvatar.getAttachmentsVariant();
    avatar[ENTRY_AVATAR_ENTITIES] = getMyAvatarWearables();
    console.debug('getMyAvatar: ', JSON.stringify(avatar, null, 4));

    return avatar;
}

function updateAvatarWearables(avatar, wearables) {
    avatar[ENTRY_AVATAR_ENTITIES] = wearables;
}

var adjustWearables = {
    opened : false,
    cameraMode : '',
    setOpened : function(value) {
        if(this.opened !== value) {
            console.debug('avatarapp.js: adjustWearables.setOpened: ', value)
            if(value) {
                this.cameraMode = Camera.mode;
                console.debug('avatarapp.js: adjustWearables.setOpened: storing camera mode: ', this.cameraMode);

                if(!HMD.active) {
                    Camera.mode = 'mirror';
                }
            } else {
                Camera.mode = this.cameraMode;
            }

            this.opened = value;
        }
    }
}

var currentAvatar = getMyAvatar();
var selectedAvatarEntityGrabbable = false;
var selectedAvatarEntity = null;

function fromQml(message) { // messages are {method, params}, like json-rpc. See also sendToQml.
    console.debug('fromQml: message = ', JSON.stringify(message, null, '\t'))

    switch (message.method) {
    case 'getAvatars':

        message.reply = {
            'bookmarks' : AvatarBookmarks.getBookmarks(),
            'currentAvatar' : currentAvatar
        };

        console.debug('avatarapp.js: currentAvatar: ', JSON.stringify(message.reply.currentAvatar, null, '\t'))
        sendToQml(message)
        break;
    case 'adjustWearable':
        if(message.properties.localRotationAngles) {
            message.properties.localRotation = Quat.fromVec3Degrees(message.properties.localRotationAngles)
        }

        console.debug('Entities.editEntity(message.entityID, message.properties)'.replace('message.entityID', message.entityID).replace('message.properties', JSON.stringify(message.properties)));
        Entities.editEntity(message.entityID, message.properties);
        sendToQml({'method' : 'wearableUpdated', 'wearable' : message.entityID, wearableIndex : message.wearableIndex, properties : message.properties})
        break;
    case 'selectAvatar':
        console.debug('avatarapp.js: selecting avatar: ', message.name);
        AvatarBookmarks.loadBookmark(message.name);
        break;
    case 'adjustWearablesOpened':
        adjustWearables.setOpened(true);
        Entities.mousePressOnEntity.connect(onSelectedEntity);
        break;
    case 'adjustWearablesClosed':
        adjustWearables.setOpened(false);
        ensureWearableSelected(null);
        Entities.mousePressOnEntity.disconnect(onSelectedEntity);
        break;
    case 'selectWearable':
        ensureWearableSelected(message.entityID);
        break;
    case 'deleteWearable':
        console.debug('before deleting: wearables.length: ', getMyAvatarWearables().length);
        console.debug(JSON.stringify(Entities.getEntityProperties(message.entityID, ['parentID'])));

        Entities.editEntity(message.entityID, { parentID : null }) // 2do: remove this hack when backend will be fixed
        console.debug(JSON.stringify(Entities.getEntityProperties(message.entityID, ['parentID'])));

        Entities.deleteEntity(message.entityID);
        var wearables = getMyAvatarWearables();
        console.debug('after deleting: wearables.length: ', wearables.length);

        updateAvatarWearables(currentAvatar, wearables);
        sendToQml({'method' : 'wearablesUpdated', 'wearables' : wearables, 'avatarName' : message.avatarName})
        break;
    default:
        print('Unrecognized message from AvatarApp.qml:', JSON.stringify(message));
    }
}

function isGrabbable(entityID) {
    if(entityID === null) {
        return false;
    }

    var properties = Entities.getEntityProperties(entityID, ['clientOnly', 'userData']);
    if (properties.clientOnly) {
        var userData;
        try {
            userData = JSON.parse(properties.userData);
        } catch (e) {
            userData = {};
        }

        return userData.grabbableKey && userData.grabbableKey.grabbable;
    }

    return false;
}

function setGrabbable(entityID, grabbable) {
    console.debug('making entity', entityID, grabbable ? 'grabbable' : 'not grabbable');

    var properties = Entities.getEntityProperties(entityID, ['clientOnly', 'userData']);
    if (properties.clientOnly) {
        var userData;
        try {
            userData = JSON.parse(properties.userData);
        } catch (e) {
            userData = {};
        }

        if (userData.grabbableKey === undefined) {
            userData.grabbableKey = {};
        }
        userData.grabbableKey.grabbable = grabbable;
        Entities.editEntity(entityID, {userData: JSON.stringify(userData)});
    }
}

function ensureWearableSelected(entityID) {
    if(selectedAvatarEntity !== entityID) {

        if(selectedAvatarEntity !== null) {
            setGrabbable(selectedAvatarEntity, selectedAvatarEntityGrabbable);
        }

        selectedAvatarEntity = entityID;
        selectedAvatarEntityGrabbable = isGrabbable(entityID);

        if(selectedAvatarEntity !== null) {
            setGrabbable(selectedAvatarEntity, true);
        }

        return true;
    }

    return false;
}

function isEntityBeingWorn(entityID) {
    return Entities.getEntityProperties(entityID, 'parentID').parentID === MyAvatar.sessionUUID;
};

function onSelectedEntity(entityID, pointerEvent) {
    if (isEntityBeingWorn(entityID)) {
        console.debug('onSelectedEntity: clicked on wearable', entityID);

        if(ensureWearableSelected(entityID)) {
            sendToQml({'method' : 'selectAvatarEntity', 'entityID' : selectedAvatarEntity});
        }
    }
}

function sendToQml(message) {
    console.debug('avatarapp.js: sendToQml: ', JSON.stringify(message, 0, 4));
    tablet.sendToQml(message);
}

function onBookmarkLoaded(bookmarkName) {
    currentAvatar = getMyAvatar();
    console.debug('avatarapp.js: onBookmarkLoaded: ', JSON.stringify(currentAvatar, 0, 4));
    sendToQml({'method' : 'bookmarkLoaded', 'reply' : {'name' : bookmarkName, 'currentAvatar' : currentAvatar} });
}

//
// Manage the connection between the button and the window.
//
var button;
var buttonName = "AvatarApp";
var tablet = null;

function startup() {
    tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    button = tablet.addButton({
        text: buttonName,
        icon: "icons/tablet-icons/people-i.svg",
        activeIcon: "icons/tablet-icons/people-a.svg",
        sortOrder: 7
    });
    button.clicked.connect(onTabletButtonClicked);
    tablet.screenChanged.connect(onTabletScreenChanged);
    AvatarBookmarks.bookmarkLoaded.connect(onBookmarkLoaded);

//    Window.domainChanged.connect(clearLocalQMLDataAndClosePAL);
//    Window.domainConnectionRefused.connect(clearLocalQMLDataAndClosePAL);
//    Users.avatarDisconnected.connect(avatarDisconnected);
//    AvatarList.avatarAddedEvent.connect(avatarAdded);
//    AvatarList.avatarRemovedEvent.connect(avatarRemoved);
//    AvatarList.avatarSessionChangedEvent.connect(avatarSessionChanged);
}

startup();

var isWired = false;
function off() {
    if (isWired) { // It is not ok to disconnect these twice, hence guard.
        //Controller.mousePressEvent.disconnect(handleMouseEvent);
        //Controller.mouseMoveEvent.disconnect(handleMouseMoveEvent);
        tablet.tabletShownChanged.disconnect(tabletVisibilityChanged);
        isWired = false;
    }

    if(adjustWearables.opened) {
        adjustWearables.setOpened(false);
        ensureWearableSelected(null);
        Entities.mousePressOnEntity.disconnect(onSelectedEntity);
    }
}

function tabletVisibilityChanged() {
    if (!tablet.tabletShown) {
        tablet.gotoHomeScreen();
    }
}

var onAvatarAppScreen = false;

function onTabletButtonClicked() {
    if (onAvatarAppScreen) {
        // for toolbar-mode: go back to home screen, this will close the window.
        tablet.gotoHomeScreen();
    } else {
        ContextOverlay.enabled = false;
        tablet.loadQMLSource(AVATARAPP_QML_SOURCE);
        tablet.tabletShownChanged.connect(tabletVisibilityChanged);
        isWired = true;
    }
}
var hasEventBridge = false;
function wireEventBridge(on) {
    if (on) {
        if (!hasEventBridge) {
            console.debug('tablet.fromQml.connect')
            tablet.fromQml.connect(fromQml);
            hasEventBridge = true;
        }
    } else {
        if (hasEventBridge) {
            console.debug('tablet.fromQml.disconnect')
            tablet.fromQml.disconnect(fromQml);
            hasEventBridge = false;
        }
    }
}

function onTabletScreenChanged(type, url) {
    console.debug('avatarapp.js: onTabletScreenChanged: ', type, url);

    onAvatarAppScreen = (type === "QML" && url === AVATARAPP_QML_SOURCE);
    wireEventBridge(onAvatarAppScreen);
    // for toolbar mode: change button to active when window is first openend, false otherwise.
    button.editProperties({isActive: onAvatarAppScreen});

    if (onAvatarAppScreen) {

        var message = {
            'method' : 'initialize',
            'reply' : {
                'jointNames' : MyAvatar.getJointNames()
            }
        };

        sendToQml(message)
    }

    console.debug('onAvatarAppScreen: ', onAvatarAppScreen);

    // disable sphere overlays when not on avatarapp screen.
    if (!onAvatarAppScreen) {
        off();
    }
}

function shutdown() {
    console.debug('shutdown: adjustWearables.opened', adjustWearables.opened);

    if (onAvatarAppScreen) {
        tablet.gotoHomeScreen();
    }
    button.clicked.disconnect(onTabletButtonClicked);
    tablet.removeButton(button);
    tablet.screenChanged.disconnect(onTabletScreenChanged);
    AvatarBookmarks.bookmarkLoaded.disconnect(onBookmarkLoaded);

//    Window.domainChanged.disconnect(clearLocalQMLDataAndClosePAL);
//    Window.domainConnectionRefused.disconnect(clearLocalQMLDataAndClosePAL);
//    AvatarList.avatarAddedEvent.disconnect(avatarAdded);
//    AvatarList.avatarRemovedEvent.disconnect(avatarRemoved);
//    AvatarList.avatarSessionChangedEvent.disconnect(avatarSessionChanged);
    off();
}

//
// Cleanup.
//
Script.scriptEnding.connect(shutdown);

}()); // END LOCAL_SCOPE
