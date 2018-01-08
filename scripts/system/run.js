"use strict";

/* global Script, Tablet, MyAvatar */

(function() { // BEGIN LOCAL_SCOPE
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: Script.resolvePath("assets/images/icon-particles.svg"),
        text: "Run",
        sortOrder: 15
    });

    function onClicked() {
        if (MyAvatar.walkSpeed < 4) {
            MyAvatar.walkSpeed = 4.5;
        } else {
            MyAvatar.walkSpeed = 3.0;
        }
    }

    function cleanup() {
        button.clicked.disconnect(onClicked);
        tablet.removeButton(button);
    }

    button.clicked.connect(onClicked);
}());
