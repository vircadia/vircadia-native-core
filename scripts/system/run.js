"use strict";

/* global Script, Tablet, MyAvatar */

(function() { // BEGIN LOCAL_SCOPE
    var WALK_SPEED = 2.6;
    var RUN_SPEED = 4.5;
    var MIDDLE_SPEED = (WALK_SPEED + RUN_SPEED) / 2.0;

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: Script.resolvePath("assets/images/run.svg"),
        text: "RUN"
    });

    function onClicked() {
        if (MyAvatar.walkSpeed < MIDDLE_SPEED) {
            button.editProperties({isActive: true});
            MyAvatar.walkSpeed = RUN_SPEED;
        } else {
            button.editProperties({isActive: false});
            MyAvatar.walkSpeed = WALK_SPEED;
        }
    }

    function cleanup() {
        button.clicked.disconnect(onClicked);
        tablet.removeButton(button);
    }

    button.clicked.connect(onClicked);
    if (MyAvatar.walkSpeed < MIDDLE_SPEED) {
        button.editProperties({isActive: false});
    } else {
        button.editProperties({isActive: true});
    }

    Script.scriptEnding.connect(cleanup);
}());
