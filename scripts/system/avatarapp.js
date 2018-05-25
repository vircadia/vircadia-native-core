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

function executeLater(callback) {
    Script.setTimeout(callback, 300);
}

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

function getMyAvatarSettings() {
    return {
        dominantHand: MyAvatar.getDominantHand(),
        collisionsEnabled : MyAvatar.getCollisionsEnabled(),
        collisionSoundUrl : MyAvatar.collisionSoundURL,
        animGraphUrl : MyAvatar.getAnimGraphUrl(),
    }
}

function updateAvatarWearables(avatar, bookmarkAvatarName) {
    console.debug('avatarapp.js: scheduling wearablesUpdated notify for', bookmarkAvatarName);

    executeLater(function() {
        console.debug('avatarapp.js: executing wearablesUpdated notify for', bookmarkAvatarName);

        var wearables = getMyAvatarWearables();
        avatar[ENTRY_AVATAR_ENTITIES] = wearables;

        sendToQml({'method' : 'wearablesUpdated', 'wearables' : wearables, 'avatarName' : bookmarkAvatarName})
    });
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

var currentAvatarWearablesBackup = null;
var currentAvatar = null;
var currentAvatarSettings = getMyAvatarSettings();

var notifyScaleChanged = true;
function onTargetScaleChanged() {
    console.debug('onTargetScaleChanged: ', MyAvatar.getAvatarScale());
    if(currentAvatar.scale !== MyAvatar.getAvatarScale()) {
        currentAvatar.scale = MyAvatar.getAvatarScale();
        if(notifyScaleChanged) {
            sendToQml({'method' : 'scaleChanged', 'value' : currentAvatar.scale})
        }
    }
}

function onDominantHandChanged(dominantHand) {
    console.debug('onDominantHandChanged: ', dominantHand);
    if(currentAvatarSettings.dominantHand !== dominantHand) {
        currentAvatarSettings.dominantHand = dominantHand;
        sendToQml({'method' : 'settingChanged', 'name' : 'dominantHand', 'value' : dominantHand})
    }
}

function onCollisionsEnabledChanged(enabled) {
    console.debug('onCollisionsEnabledChanged: ', enabled);
    if(currentAvatarSettings.collisionsEnabled !== enabled) {
        currentAvatarSettings.collisionsEnabled = enabled;
        sendToQml({'method' : 'settingChanged', 'name' : 'collisionsEnabled', 'value' : enabled})
    }
}

function onNewCollisionSoundUrl(url) {
    console.debug('onNewCollisionSoundUrl: ', url);
    if(currentAvatarSettings.collisionSoundUrl !== url) {
        currentAvatarSettings.collisionSoundUrl = url;
        sendToQml({'method' : 'settingChanged', 'name' : 'collisionSoundUrl', 'value' : url})
    }
}

function onAnimGraphUrlChanged(url) {
    console.debug('onAnimGraphUrlChanged: ', url);
    if(currentAvatarSettings.animGraphUrl !== url) {
        currentAvatarSettings.animGraphUrl = url;
        sendToQml({'method' : 'settingChanged', 'name' : 'animGraphUrl', 'value' : url})
    }
}

var selectedAvatarEntityGrabbable = false;
var selectedAvatarEntityID = null;
var grabbedAvatarEntityChangeNotifier = null;

var MARKETPLACE_PURCHASES_QML_PATH = "hifi/commerce/purchases/Purchases.qml";
var MARKETPLACE_URL = Account.metaverseServerURL + "/marketplace";
var MARKETPLACES_INJECT_SCRIPT_URL = Script.resolvePath("../html/js/marketplacesInject.js");

function fromQml(message) { // messages are {method, params}, like json-rpc. See also sendToQml.
    console.debug('fromQml: message = ', JSON.stringify(message, null, '\t'))

    switch (message.method) {
    case 'getAvatars':
        currentAvatar = getMyAvatar();
        currentAvatarSettings = getMyAvatarSettings();

        message.data = {
            'bookmarks' : AvatarBookmarks.getBookmarks(),
            'displayName' : MyAvatar.displayName,
            'currentAvatar' : currentAvatar,
            'currentAvatarSettings' : currentAvatarSettings
        };

        console.debug('avatarapp.js: currentAvatarSettings: ', JSON.stringify(message.data.currentAvatarSettings, null, '\t'))
        console.debug('avatarapp.js: currentAvatar: ', JSON.stringify(message.data.currentAvatar, null, '\t'))
        sendToQml(message)
        break;
    case 'selectAvatar':
        console.debug('avatarapp.js: selecting avatar: ', message.name);
        AvatarBookmarks.loadBookmark(message.name);
        break;
    case 'deleteAvatar':
        console.debug('avatarapp.js: deleting avatar: ', message.name);
        AvatarBookmarks.removeBookmark(message.name);
        break;
    case 'addAvatar':
        console.debug('avatarapp.js: saving avatar: ', message.name);
        AvatarBookmarks.addBookmark(message.name);
        break;
    case 'adjustWearable':
        if(message.properties.localRotationAngles) {
            message.properties.localRotation = Quat.fromVec3Degrees(message.properties.localRotationAngles)
        }
        console.debug('Entities.editEntity(message.entityID, message.properties)'.replace('message.entityID', message.entityID).replace('message.properties', JSON.stringify(message.properties)));
        Entities.editEntity(message.entityID, message.properties);
        sendToQml({'method' : 'wearableUpdated', 'entityID' : message.entityID, wearableIndex : message.wearableIndex, properties : message.properties, updateUI : false})
        break;
    case 'adjustWearablesOpened':
        console.debug('avatarapp.js: adjustWearablesOpened');

        currentAvatarWearablesBackup = getMyAvatarWearables();
        adjustWearables.setOpened(true);

        Entities.mousePressOnEntity.connect(onSelectedEntity);
        Messages.subscribe('Hifi-Object-Manipulation');
        Messages.messageReceived.connect(handleWearableMessages);
        break;
    case 'adjustWearablesClosed':
        console.debug('avatarapp.js: adjustWearablesClosed');

        if(!message.save) {
            // revert changes using snapshot of wearables
            console.debug('reverting... ');
            if(currentAvatarWearablesBackup !== null) {
                AvatarBookmarks.updateAvatarEntities(currentAvatarWearablesBackup);
                updateAvatarWearables(currentAvatar, message.avatarName);
            }
        } else {
            console.debug('saving... ');
            sendToQml({'method' : 'updateAvatarInBookmarks'});
        }

        adjustWearables.setOpened(false);
        ensureWearableSelected(null);
        Entities.mousePressOnEntity.disconnect(onSelectedEntity);
        Messages.messageReceived.disconnect(handleWearableMessages);
        Messages.unsubscribe('Hifi-Object-Manipulation');
        break;
    case 'selectWearable':
        ensureWearableSelected(message.entityID);
        break;
    case 'deleteWearable':
        console.debug('avatarapp.js: deleteWearable: ', message.entityID, 'from avatar: ', message.avatarName);
        Entities.deleteEntity(message.entityID);
        updateAvatarWearables(currentAvatar, message.avatarName);
        break;
    case 'changeDisplayName':
        console.debug('avatarapp.js: changeDisplayName: ', message.displayName);
        if (MyAvatar.displayName !== message.displayName) {
            MyAvatar.displayName = message.displayName;
            UserActivityLogger.palAction("display_name_change", message.displayName);
        }
        break;
    case 'applyExternalAvatar':
        console.debug('avatarapp.js: applyExternalAvatar: ', message.avatarURL);
        var currentAvatarURL = MyAvatar.getFullAvatarURLFromPreferences();
        if(currentAvatarURL !== message.avatarURL) {
            MyAvatar.useFullAvatarURL(message.avatarURL);
            sendToQml({'method' : 'externalAvatarApplied', 'avatarURL' : message.avatarURL})
        }
        break;
    case 'navigate':
        console.debug('avatarapp.js: navigate: ', message.url);
        var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system")
        if(message.url.indexOf('app://') === 0) {
            if(message.url === 'app://marketplace') {
                tablet.gotoWebScreen(MARKETPLACE_URL, MARKETPLACES_INJECT_SCRIPT_URL);
            } else if(message.url === 'app://purchases') {
                tablet.pushOntoStack(MARKETPLACE_PURCHASES_QML_PATH);
            }

        } else if(message.url.indexOf('hifi://') === 0) {
            AddressManager.handleLookupString(message.url, false);
        } else if(message.url.indexOf('https://') === 0 || message.url.indexOf('http://') === 0) {
            tablet.gotoWebScreen(message.url);
        }

        break;
    case 'setScale':
        console.debug('avatarapp.js: setScale: ', message.avatarScale);
        notifyScaleChanged = false;
        MyAvatar.setAvatarScale(message.avatarScale);
        currentAvatar.avatarScale = message.avatarScale;
        notifyScaleChanged = true;
        break;
    case 'revertScale':
        console.debug('avatarapp.js: revertScale: ', message.avatarScale);
        MyAvatar.setAvatarScale(message.avatarScale);
        currentAvatar.avatarScale = message.avatarScale;
        break;
    case 'saveSettings':
        console.debug('avatarapp.js: saveSettings: ', JSON.stringify(message.settings, 0, 4));
        MyAvatar.setAvatarScale(message.avatarScale);
        currentAvatar.avatarScale = message.avatarScale;

        MyAvatar.setDominantHand(message.settings.dominantHand);
        MyAvatar.setCollisionsEnabled(message.settings.collisionsEnabled);
        MyAvatar.collisionSoundURL = message.settings.collisionSoundUrl;
        MyAvatar.setAnimGraphUrl(message.settings.animGraphUrl);

        settings = getMyAvatarSettings();
        console.debug('saveSettings: settings: ', JSON.stringify(settings, 0, 4));
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
    if(selectedAvatarEntityID !== entityID) {
        if(grabbedAvatarEntityChangeNotifier !== null) {
            Script.clearInterval(grabbedAvatarEntityChangeNotifier);
            grabbedAvatarEntityChangeNotifier = null;
        }

        if(selectedAvatarEntityID !== null) {
            setGrabbable(selectedAvatarEntityID, selectedAvatarEntityGrabbable);
        }

        selectedAvatarEntityID = entityID;
        console.debug('ensureWearableSelected: selectedAvatarEntity = ', selectedAvatarEntityID);

        selectedAvatarEntityGrabbable = isGrabbable(entityID);

        if(selectedAvatarEntityID !== null) {
            setGrabbable(selectedAvatarEntityID, true);
        }

        return true;
    }

    return false;
}

function isEntityBeingWorn(entityID) {
    return Entities.getEntityProperties(entityID, 'parentID').parentID === MyAvatar.sessionUUID;
};

function onSelectedEntity(entityID, pointerEvent) {
    console.debug('onSelectedEntity: clicked on wearable', entityID);

    if(selectedAvatarEntityID !== entityID && isEntityBeingWorn(entityID))
    {
        console.debug('onSelectedEntity: clicked on worn wearable', entityID);

        if(ensureWearableSelected(entityID)) {
            sendToQml({'method' : 'selectAvatarEntity', 'entityID' : selectedAvatarEntityID});
        }
    }
}

function handleWearableMessages(channel, message, sender) {
    if (channel !== 'Hifi-Object-Manipulation') {
        return;
    }

    var parsedMessage = null;

    try {
        parsedMessage = JSON.parse(message);
        console.debug('handleWearableMessages: ', JSON.stringify(parsedMessage, null, 4));
    } catch (e) {
        print('error parsing wearable message');
        return;
    }

    var entityID = parsedMessage.grabbedEntity;
    console.debug('item worn: ', isEntityBeingWorn(entityID))

    if(parsedMessage.action === 'grab') {
        console.debug('handleWearableMessages: grab: ', JSON.stringify(parsedMessage, null, 4));

        if(selectedAvatarEntityID !== entityID) {
            ensureWearableSelected(entityID);
            sendToQml({'method' : 'selectAvatarEntity', 'entityID' : selectedAvatarEntityID});
        }

        console.debug('creating grabbedAvatarEntityChangeNotifier: ', grabbedAvatarEntityChangeNotifier);

        grabbedAvatarEntityChangeNotifier = Script.setInterval(function() {
            console.debug('grabbedAvatarEntityChangeNotifier callback')

            // for some reasons Entities.getEntityProperties returns more than was asked..
            var propertyNames = ['localPosition', 'localRotation', 'dimensions', 'naturalDimensions'];
            var entityProperties = Entities.getEntityProperties(selectedAvatarEntityID, propertyNames);
            var properties = {}

            propertyNames.forEach(function(propertyName) {
                properties[propertyName] = entityProperties[propertyName];
            })

            properties.localRotationAngles = Quat.safeEulerAngles(properties.localRotation);
            sendToQml({'method' : 'wearableUpdated', 'entityID' : selectedAvatarEntityID, 'wearableIndex' : -1, 'properties' : properties, updateUI : true})

        }, 1000);
    } else if(parsedMessage.action === 'release') {
        console.debug('handleWearableMessages: release: ', JSON.stringify(parsedMessage, null, 4));

        if(grabbedAvatarEntityChangeNotifier !== null) {
            console.debug('clearing grabbedAvatarEntityChangeNotifier: ', grabbedAvatarEntityChangeNotifier);
            Script.clearInterval(grabbedAvatarEntityChangeNotifier);
            grabbedAvatarEntityChangeNotifier = null;
        }
    }
}

function sendToQml(message) {
    console.debug('avatarapp.js: sendToQml: ', JSON.stringify(message, 0, 4));
    tablet.sendToQml(message);
}

function onBookmarkLoaded(bookmarkName) {
    console.debug('avatarapp.js: scheduling onBookmarkLoaded: ', bookmarkName);

    executeLater(function() {
        currentAvatar = getMyAvatar();
        console.debug('avatarapp.js: executing onBookmarkLoaded: ', JSON.stringify(currentAvatar, 0, 4));
        sendToQml({'method' : 'bookmarkLoaded', 'data' : {'name' : bookmarkName, 'currentAvatar' : currentAvatar} });
    });
}

function onBookmarkDeleted(bookmarkName) {
    console.debug('avatarapp.js: onBookmarkDeleted: ', bookmarkName);
    sendToQml({'method' : 'bookmarkDeleted', 'name' : bookmarkName});
}

function onBookmarkAdded(bookmarkName) {
    console.debug('avatarapp.js: onBookmarkAdded: ', bookmarkName);
    sendToQml({ 'method': 'bookmarkAdded', 'bookmarkName': bookmarkName, 'bookmark': AvatarBookmarks.getBookmark(bookmarkName) });
}

//
// Manage the connection between the button and the window.
//
var button;
var buttonName = "Avatar";
var tablet = null;

function startup() {
    tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    button = tablet.addButton({
        text: buttonName,
        icon: "icons/tablet-icons/avatar-i.svg",
        activeIcon: "icons/tablet-icons/avatar-a.svg",
        sortOrder: 7
    });
    button.clicked.connect(onTabletButtonClicked);
    tablet.screenChanged.connect(onTabletScreenChanged);
}

startup();

var isWired = false;
function off() {
    console.debug('entering avatarapp.js: off');

    if (isWired) { // It is not ok to disconnect these twice, hence guard.
        tablet.tabletShownChanged.disconnect(tabletVisibilityChanged);
        isWired = false;
    }

    if(adjustWearables.opened) {
        adjustWearables.setOpened(false);
        ensureWearableSelected(null);
        Entities.mousePressOnEntity.disconnect(onSelectedEntity);

        Messages.messageReceived.disconnect(handleWearableMessages);
        Messages.unsubscribe('Hifi-Object-Manipulation');
    }

    AvatarBookmarks.bookmarkLoaded.disconnect(onBookmarkLoaded);
    AvatarBookmarks.bookmarkDeleted.disconnect(onBookmarkDeleted);
    AvatarBookmarks.bookmarkAdded.disconnect(onBookmarkAdded);

    MyAvatar.dominantHandChanged.disconnect(onDominantHandChanged);
    MyAvatar.collisionsEnabledChanged.disconnect(onCollisionsEnabledChanged);
    MyAvatar.newCollisionSoundURL.disconnect(onNewCollisionSoundUrl);
    MyAvatar.animGraphUrlChanged.disconnect(onAnimGraphUrlChanged);
    MyAvatar.targetScaleChanged.disconnect(onTargetScaleChanged);

    console.debug('exiting avatarapp.js: off');
}

function on() {
    console.debug('entering avatarapp.js: on');

    AvatarBookmarks.bookmarkLoaded.connect(onBookmarkLoaded);
    AvatarBookmarks.bookmarkDeleted.connect(onBookmarkDeleted);
    AvatarBookmarks.bookmarkAdded.connect(onBookmarkAdded);

    MyAvatar.dominantHandChanged.connect(onDominantHandChanged);
    MyAvatar.collisionsEnabledChanged.connect(onCollisionsEnabledChanged);
    MyAvatar.newCollisionSoundURL.connect(onNewCollisionSoundUrl);
    MyAvatar.animGraphUrlChanged.connect(onAnimGraphUrlChanged);
    MyAvatar.targetScaleChanged.connect(onTargetScaleChanged);

    console.debug('exiting avatarapp.js: on');
}

function tabletVisibilityChanged() {
    if (!tablet.tabletShown) {
        tablet.gotoHomeScreen();
    }
}

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

var onAvatarAppScreen = false;
function onTabletScreenChanged(type, url) {
    console.debug('avatarapp.js: onTabletScreenChanged: ', type, url);

    var onAvatarAppScreenNow = (type === "QML" && url === AVATARAPP_QML_SOURCE);
    wireEventBridge(onAvatarAppScreenNow);
    // for toolbar mode: change button to active when window is first openend, false otherwise.
    button.editProperties({isActive: onAvatarAppScreenNow});

    if (!onAvatarAppScreen && onAvatarAppScreenNow) {
        console.debug('entering avatarapp');
        on();
    } else if(onAvatarAppScreen && !onAvatarAppScreenNow) {
        console.debug('leaving avatarapp');
        off();
    }

    onAvatarAppScreen = onAvatarAppScreenNow;

    if(onAvatarAppScreenNow) {
        var message = {
            'method' : 'initialize',
            'data' : {
                'jointNames' : MyAvatar.getJointNames()
            }
        };

        sendToQml(message)
    }

    console.debug('onAvatarAppScreenNow: ', onAvatarAppScreenNow);
}

function shutdown() {
    console.debug('shutdown: adjustWearables.opened', adjustWearables.opened);

    if (onAvatarAppScreen) {
        tablet.gotoHomeScreen();
    }
    button.clicked.disconnect(onTabletButtonClicked);
    tablet.removeButton(button);
    tablet.screenChanged.disconnect(onTabletScreenChanged);

    off();
}

//
// Cleanup.
//
Script.scriptEnding.connect(shutdown);

}()); // END LOCAL_SCOPE
