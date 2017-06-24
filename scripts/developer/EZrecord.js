"use strict";

//
//  EZrecord.js
//
//  Created by David Rowe on 24 Jun 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    var APP_NAME = "EZRECORD",
        APP_ICON_INACTIVE = "icons/tablet-icons/avatar-record-i.svg",
        APP_ICON_ACTIVE = "icons/tablet-icons/avatar-record-a.svg",
        tablet,
        button;

    function onButtonClicked() {
        // TODO
        print("Clicked");
    }

    function setUp() {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        if (!tablet) {
            return;
        }

        // Tablet/toolbar button.
        button = tablet.addButton({
            icon: APP_ICON_INACTIVE,
            activeIcon: APP_ICON_ACTIVE,
            text: APP_NAME,
            isActive: false
        });
        if (button) {
            button.clicked.connect(onButtonClicked);
        }
    }

    function tearDown() {
        if (!tablet) {
            return;
        }

        if (button) {
            button.clicked.disconnect(onButtonClicked);
            tablet.removeButton(button);
            button = null;
        }

        tablet = null;
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
