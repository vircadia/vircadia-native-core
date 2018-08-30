//
//  nearTabletHighlight.js
//
//  Highlight the tablet if a hand is near enough to grab it.
//
//  Created by David Rowe on 28 Aug 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global LEFT_HAND, RIGHT_HAND, makeDispatcherModuleParameters, makeRunningValues, enableDispatcherModule, 
 * disableDispatcherModule */

Script.include("/~/system/libraries/controllerDispatcherUtils.js");

(function () {

    "use strict";

    function NearTabletHighlight(hand) {
        this.hand = hand;

        this.parameters = makeDispatcherModuleParameters(
            95,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100
        );

        this.isNearTablet = function (controllerData) {
            return HMD.tabletID && controllerData.nearbyOverlayIDs[this.hand].indexOf(HMD.tabletID) !== -1;
        };

        this.isReady = function (controllerData) {
            if (this.isNearTablet(controllerData)) {
                return makeRunningValues(true, [], []);
            }
            return makeRunningValues(false, [], []);
        };

        this.run = function (controllerData) {
            if (!this.isNearTablet(controllerData)) {
                return makeRunningValues(false, [], []);
            }

            if (controllerData.triggerClicks[this.hand]) {
                return makeRunningValues(false, [], []);
            }

            return makeRunningValues(true, [], []);
        };
    }

    var leftNearTabletHighlight = new NearTabletHighlight(LEFT_HAND);
    var rightNearTabletHighlight = new NearTabletHighlight(RIGHT_HAND);
    enableDispatcherModule("LeftNearTabletHighlight", leftNearTabletHighlight);
    enableDispatcherModule("RightNearTabletHighlight", rightNearTabletHighlight);

    function cleanUp() {
        disableDispatcherModule("LeftNearTabletHighlight");
        disableDispatcherModule("RightNearTabletHighlight");
    }
    Script.scriptEnding.connect(cleanUp);

}());
