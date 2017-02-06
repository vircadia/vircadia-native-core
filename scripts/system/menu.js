//
//  menu.js
//  scripts/system/
//
//  Created by Dante Ruiz  on 5 Jun 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var HOME_BUTTON_TEXTURE = "http://hifi-content.s3.amazonaws.com/alan/dev/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-root.png";
//var HOME_BUTTON_TEXTURE = Script.resourcesPath() + "meshes/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-root.png";

(function() {
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: "icons/tablet-icons/menu-i.svg",
        text: "MENU",
        sortOrder: 3
    });

    function onClicked() {
        var entity = HMD.tabletID;
        Entities.editEntity(entity, {textures: JSON.stringify({"tex.close": HOME_BUTTON_TEXTURE})});
        tablet.gotoMenuScreen();
    }

    button.clicked.connect(onClicked);

    Script.scriptEnding.connect(function () {
        button.clicked.disconnect(onClicked);
        tablet.removeButton(button);
    });
}());
