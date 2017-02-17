//
//  fingerPaint.js
//
//  Created by David Rowe on 15 Feb 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {
    var tablet,
        button,
        BUTTON_NAME = "PAINT",
        isFingerPainting = false;

    function onButtonClicked() {
        isFingerPainting = !isFingerPainting;
        button.editProperties({ isActive: isFingerPainting });
    }

    function setUp() {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        if (tablet) {
            button = tablet.addButton({
                icon: "icons/tablet-icons/bubble-i.svg",
                activeIcon: "icons/tablet-icons/bubble-a.svg",
                text: BUTTON_NAME,
                isActive: isFingerPainting
            });
            button.clicked.connect(onButtonClicked);
        }
    }

    function tearDown() {
        if (tablet) {
            button.clicked.disconnect(onButtonClicked);
            tablet.removeButton(button);
        }
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());