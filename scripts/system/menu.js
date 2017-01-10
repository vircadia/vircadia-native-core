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

(function() {
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: "icons/tablet-icons/menu-i.svg",
        text: "Menu"
    });

  
    function onClicked() {
        tablet.gotoMenuScreen();
    }

    button.clicked.connect(onClicked);

    Script.scriptEnding.connect(function () {
        button.clicked.disconnect(onClicked);
        tablet.removeButton(button);
    })
}());