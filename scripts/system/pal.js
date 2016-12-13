"use strict";
/*jslint vars: true, plusplus: true, forin: true*/
/*globals Script, AvatarList, Camera, Overlays, OverlayWindow, Toolbars, Vec3, Controller, print */
//
// pal.js
//
// Created by Howard Stearns on December 9, 2016
// Copyright 2016 High Fidelity, Inc
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// FIXME: (function() { // BEGIN LOCAL_SCOPE

// Overlays
var overlays = {}; // Keeps track of all our extended overlay data objects, keyed by target identifier.
function ExtendedOverlay(key, type, properties) { // A wrapper around overlays to store the key it is associated with.
    overlays[key] = this;
    this.key = key;
    this.unselectedOverlay = Overlays.addOverlay(type, properties);
}
// Instance methods:
ExtendedOverlay.prototype.deleteOverlay = function () { // remove display and data of this overlay
    Overlays.deleteOverlay(this.unselectedOverlay);
    if (this.selected) {
        Overlays.deleteOverlay(this.selectedOverlay);
    }
    delete overlays[this.key];
};
ExtendedOverlay.prototype.editOverlay = function (properties) { // change display of this overlay
    Overlays.editOverlay(this.selected ? this.selectedOverlay : this.unselectedOverlay, properties);
};
ExtendedOverlay.prototype.select = function (selected, type, initialProperties) {
    if (selected && !this.selectedOverlay) {
        this.selectedOverlay = Overlays.addOverlay(type, initialProperties);
    }
    this.selected = selected;
    Overlays.editOverlay(this.unselectedOverlay, {visible: !selected});
    Overlays.editOverlay(this.selectedOverlay, {visible: selected});
};
// Class methods:
ExtendedOverlay.get = function (key) { // answer the extended overlay data object associated with the given avatar identifier
    return overlays[key];
};
ExtendedOverlay.some = function (iterator) { // Bails early as soon as iterator returns truthy.
    var key;
    for (key in overlays) {
        if (iterator(ExtendedOverlay.get(key))) {
            return;
        }
    }
};
ExtendedOverlay.applyPickRay = function (pickRay, cb) { // cb(overlay) on the one overlay intersected by pickRay, if any.
    var pickedOverlay = Overlays.findRayIntersection(pickRay); // Depends on nearer coverOverlays to extend closer to us than farther ones.
    if (!pickedOverlay.intersects) {
        return;
    }
    ExtendedOverlay.some(function (overlay) { // See if pickedOverlay is one of ours.
        if ((overlay.selected ? overlay.selectedOverlay : overlay.unselectedOverlay) === pickedOverlay.overlayID) {
            cb(overlay);
            return true;
        }
    });
};


var pal = new OverlayWindow({
    title: 'People Action List',
    source: Script.resolvePath('../../qml/hifi/Pal.qml'),
    width: 480,
    height: 640,
    visible: false
});

var AVATAR_OVERLAY = Script.resolvePath("assets/images/grabsprite-3.png");
function populateUserList() {
    var data = [];
    /*data.push({ // Create an extra user for debugging.
        displayName: "Dummy Debugging User",
        userName: "foo.bar",
        sessionId: 'XXX'
    })*/
    var counter = 1;
    AvatarList.getAvatarIdentifiers().forEach(function (id) {
        var avatar = AvatarList.getAvatar(id);
        data.push({
            displayName: avatar.displayName || ('anonymous ' + counter++),
            userName: "fakeAcct" + (id || "Me"),
            sessionId: id
        });
        if (id) { // No overlay for us
            new ExtendedOverlay(id, "image3d", { // 3d so we don't go cross-eyed looking at it, but on top of everything
                imageURL: AVATAR_OVERLAY,
                drawInFront: true
            });
        }
    });
    pal.sendToQml(data);
}
function updateOverlays() {
    //var eye = Camera.position;
    AvatarList.getAvatarIdentifiers().forEach(function (id) {
        if (!id) {
            return; // don't update ourself
        }
        var avatar = AvatarList.getAvatar(id);
        var target = avatar.position;
        //var distance = Vec3.distance(target, eye);
        ExtendedOverlay.get(id).editOverlay({
            position: target
        });
    });
}
function removeOverlays() {
    ExtendedOverlay.some(function (overlay) { overlay.deleteOverlay(); });
}

// Manage the connection between the button and the window.
var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
var buttonName = "pal";
var button = toolBar.addButton({
    objectName: buttonName,
    imageURL: Script.resolvePath("assets/images/tools/ignore.svg"),
    visible: true,
    hoverState: 2,
    defaultState: 1,
    buttonState: 1,
    alpha: 0.9
});
function onClicked() {
    if (!pal.visible) {
        populateUserList();
        pal.raise();
        Script.update.connect(updateOverlays);
    } else {
        Script.update.disconnect(updateOverlays);
        removeOverlays();
    }
    pal.setVisible(!pal.visible);
}
function onVisibileChanged() {
    button.writeProperty('buttonState', pal.visible ? 0 : 1);
    button.writeProperty('defaultState', pal.visible ? 0 : 1);
    button.writeProperty('hoverState', pal.visible ? 2 : 3);
}
button.clicked.connect(onClicked);
pal.visibleChanged.connect(onVisibileChanged);

Script.scriptEnding.connect(function () {
    toolBar.removeButton(buttonName);
    pal.visibleChanged.disconnect(onVisibileChanged);
    button.clicked.disconnect(onClicked);
});


// FIXME: }()); // END LOCAL_SCOPE
