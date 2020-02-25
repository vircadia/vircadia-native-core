//
//    Simplified Nametag
//    Created by Milad Nazeri on 2019-02-16
//    Copyright 2019 High Fidelity, Inc.
//
//    Distributed under the Apache License, Version 2.0.
//    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//    Click on someone to get a nametag for them
//    
var PickRayController = Script.require('./resources/modules/pickRayController.js');
var NameTagListManager = Script.require('./resources/modules/nameTagListManager.js');
var pickRayController = new PickRayController();
var nameTagListManager = new NameTagListManager();
var altKeyPressed = false;

// Handles avatar being solo'd
pickRayController
    .registerEventHandler(selectAvatar)
    .setType("avatar")
    .setMapName("hifi_simplifiedNametag")
    .setShouldDoublePress(true)
    .create()
    .enable();


function selectAvatar(uuid, intersection) {
    if (!altKeyPressed) {
        nameTagListManager.handleSelect(uuid, intersection);
    }
}


// Handles reset of list if you change domains
function onDomainChange() {
    nameTagListManager.reset();
    nameTagListManager.handleAvatarNametagMode(avatarNametagMode);
}


// Handles removing an avatar from the list if they leave the domain
function onAvatarRemoved(uuid) {
    nameTagListManager.maybeRemove(uuid);
}


// Automatically add an avatar if they come into the domain.  Mainly used for alwaysOn mode.
function onAvatarAdded(uuid) {
    nameTagListManager.maybeAdd(uuid);
}

function blockedKeysPressed(event) {
    if (event.isAlt) {
        altKeyPressed = true;
    }
}

function blockedKeysReleased(event) {
    if (!event.isAlt) {
        altKeyPressed = false;
    }
}


// Create a new nametag list manager, connect signals, and return back a new Nametag object.
var avatarNametagMode;
function startup() {
    nameTagListManager.create();
    handleAvatarNametagMode(Settings.getValue("simplifiedNametag/avatarNametagMode", "on"));

    Script.scriptEnding.connect(unload);
    Window.domainChanged.connect(onDomainChange);
    AvatarManager.avatarRemovedEvent.connect(onAvatarRemoved);
    AvatarManager.avatarAddedEvent.connect(onAvatarAdded);
    Controller.keyPressEvent.connect(blockedKeysPressed);
    Controller.keyReleaseEvent.connect(blockedKeysReleased);

    function NameTag() {}
    
    NameTag.prototype = {
        handleAvatarNametagMode: handleAvatarNametagMode
    };

    return new NameTag();
}

// Called when the script is closing
function unload() {
    nameTagListManager.destroy();
    pickRayController.destroy();
    Window.domainChanged.disconnect(onDomainChange);
    AvatarManager.avatarRemovedEvent.disconnect(onAvatarRemoved);
    AvatarManager.avatarAddedEvent.disconnect(onAvatarAdded);
    Controller.keyPressEvent.disconnect(blockedsKeyPressed);
    Controller.keyReleaseEvent.disconnect(blockedKeysReleased);
}


// chose which mode you want the nametags in.  On, off, or alwaysOn.
function handleAvatarNametagMode(newAvatarNameTagMode) {
    avatarNametagMode = newAvatarNameTagMode;
    nameTagListManager.handleAvatarNametagMode(avatarNametagMode);
    Settings.setValue("simplifiedNametag/avatarNametagMode", avatarNametagMode);
}


var nameTag = startup();

try {
    module.exports = nameTag;
} catch (e) {
    // module doesn't exist when script run outside of simplified UI.
}
