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

function getCurrentAvatar() {
    var currentAvatar = {}
    currentAvatar[ENTRY_AVATAR_URL] = MyAvatar.skeletonModelURL;
    currentAvatar[ENTRY_AVATAR_SCALE] = MyAvatar.getAvatarScale();
    currentAvatar[ENTRY_AVATAR_ATTACHMENTS] = MyAvatar.getAttachmentsVariant();
    currentAvatar[ENTRY_AVATAR_ENTITIES] = MyAvatar.getAvatarEntitiesVariant();

    for(var i = 0; i < currentAvatar[ENTRY_AVATAR_ENTITIES].length; ++i) {
        var wearable = currentAvatar[ENTRY_AVATAR_ENTITIES][i];
        console.debug('updating localRotationAngles for wearable: ', JSON.stringify(wearable, null, '\t'));
        var localRotation = wearable.properties.localRotation;
        wearable.properties.localRotationAngles = Quat.safeEulerAngles(localRotation)
    }

    return currentAvatar;
}

function fromQml(message) { // messages are {method, params}, like json-rpc. See also sendToQml.
    console.debug('fromQml: message = ', JSON.stringify(message, null, '\t'))

    switch (message.method) {
    case 'getAvatars':

        message.reply = {
            'bookmarks' : AvatarBookmarks.getBookmarks(),
            'currentAvatar' : getCurrentAvatar()
        };

        console.debug('avatarapp.js: currentAvatar: ', JSON.stringify(message.reply.currentAvatar, null, '\t'))
        sendToQml(message)
        break;
    case 'adjustWearable':
        if(message.properties.localRotationAngles) {
            message.properties.localRotation = Quat.fromVec3Degrees(message.properties.localRotationAngles)
            delete message.properties.localRotationAngles;
        }

        console.debug('Entities.editEntity(message.id, message.properties)'.replace('message.id', message.id).replace('message.properties', JSON.stringify(message.properties)));
        Entities.editEntity(message.id, message.properties);
        break;
    case 'selectAvatar':
        console.debug('avatarapp.js: selecting avatar: ', message.name);
        AvatarBookmarks.loadBookmark(message.name);
        break;

    default:
        print('Unrecognized message from AvatarApp.qml:', JSON.stringify(message));
    }
}

function sendToQml(message) {
    tablet.sendToQml(message);
}

function onBookmarkLoaded(bookmarkName) {
    var currentAvatar = getCurrentAvatar();
    console.debug('avatarapp.js: onBookmarkLoaded: ', JSON.stringify(currentAvatar, 0, 4));
    sendToQml({'method' : 'bookmarkLoaded', 'reply' : {'name' : bookmarkName, 'currentAvatar' : getCurrentAvatar()} });
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
