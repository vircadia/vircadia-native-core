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

var PickRayController = Script.require('./resources/modules/pickRayController.js?' + Date.now());
var NameTagListManager = Script.require('./resources/modules/nameTagListManager.js?' + Date.now());
var pickRayController = new PickRayController();
var nameTagListManager = new NameTagListManager();

// Handles avatar being solo'd
pickRayController
    .registerEventHandler(selectAvatar)
    .setType("avatar")
    .setMapName("hifi_simplifiedNametag")
    .setShouldDoublePress(true)
    .create()
    .enable();


function selectAvatar(uuid, intersection) {
    nameTagListManager.handleSelect(uuid, intersection);
}


// Handles reset of list if you change domains
function onDomainChange() {
    nameTagListManager.reset();
}


// Handles removing an avatar from the list if they leave the domain
function onAvatarRemoved(uuid) {
    nameTagListManager.maybeRemove(uuid);
}


// Automatically add an avatar if they come into the domain.  Mainly used for alwaysOn mode.
function onAvatarAdded(uuid) {
    nameTagListManager.maybeAdd(uuid);
}


// Called on init
var avatarNametagMode;
function create() {
    nameTagListManager.create();
    handleAvatarNametagMode(Settings.getValue("simplifiedNametag/avatarNametagMode", "on"));

    Window.domainChanged.connect(onDomainChange);
    AvatarManager.avatarRemovedEvent.connect(onAvatarRemoved);
    AvatarManager.avatarAddedEvent.connect(onAvatarAdded);
}


// Called when the script is closing
function destroy() {
    nameTagListManager.destroy();
    pickRayController.destroy();
    Window.domainChanged.disconnect(onDomainChange);
    AvatarManager.avatarRemovedEvent.disconnect(onAvatarRemoved);
    AvatarManager.avatarAddedEvent.disconnect(onAvatarAdded);
}


// chose which mode you want the nametags in.  On, off, or alwaysOn.
function handleAvatarNametagMode(newAvatarNameTagMode) {
    avatarNametagMode = newAvatarNameTagMode;
    nameTagListManager.handleAvatarNametagMode(avatarNametagMode);
    Settings.setValue("simplifiedNametag/avatarNametagMode", avatarNametagMode);
}


// *************************************
// START api
// *************************************
// #region api


module.exports = {
    create: create,
    destroy: destroy,
    handleAvatarNametagMode: handleAvatarNametagMode
};


// #endregion
// *************************************
// END api
// *************************************