//
//  steam.js
//  scripts/system/
//
//  Created by Clement on 7/28/16
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");


var steamInviteButton = toolBar.addButton({
    objectName: "steamInvite",
    imageURL: Script.resolvePath("assets/images/tools/steam-invite.svg"),
    visible: Steam.isRunning,
    buttonState: 1,
    defaultState: 1,
    hoverState: 3,
    alpha: 0.9
});

steamInviteButton.clicked.connect(function(){
    Steam.openInviteOverlay();
});

Script.scriptEnding.connect(function () {
    toolBar.removeButton("steamInvite");
});