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

var pal = new OverlayWindow({
    title: 'People Action List', 
    source: Script.resolvePath('../../resources/qml/hifi/Pal.qml'),
    width: 480,
    height: 640, 
    visible: false
});

function populateUserList() {
    var data = [
        {
            displayName: "Dummy Debugging User",
            userName: "foo.bar",
            sessionId: 'XXX'
        }
        ];
    var counter = 1;
    AvatarList.getAvatarIdentifiers().forEach(function (id) {
        var avatar = AvatarList.getAvatar(id);
        data.push({
            displayName: avatar.displayName || ('anonymous ' + counter++),
            userName: "fakeAcct" + (id || "Me"),
            sessionId: id
        });
    });
    pal.sendToQml(data);
}


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
