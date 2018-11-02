//
//  nearTabletHighlight.js
//
//  Highlight the tablet if a hand is near enough to grab it and it isn't grabbed.
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

    var TABLET_GRABBABLE_SELECTION_NAME = "tabletGrabbableSelection";
    var TABLET_GRABBABLE_SELECTION_STYLE = {
        outlineUnoccludedColor: { red: 0, green: 180, blue: 239 }, // #00b4ef
        outlineUnoccludedAlpha: 1,
        outlineOccludedColor: { red: 0, green: 0, blue: 0 },
        outlineOccludedAlpha: 0,
        fillUnoccludedColor: { red: 0, green: 0, blue: 0 },
        fillUnoccludedAlpha: 0,
        fillOccludedColor: { red: 0, green: 0, blue: 0 },
        fillOccludedAlpha: 0,
        outlineWidth: 4,
        isOutlineSmooth: false
    };

    var isTabletNearGrabbable = [false, false];
    var isTabletHighlighted = false;

    function setTabletNearGrabbable(hand, enabled) {
        if (enabled === isTabletNearGrabbable[hand]) {
            return;
        }

        isTabletNearGrabbable[hand] = enabled;

        if (isTabletNearGrabbable[LEFT_HAND] || isTabletNearGrabbable[RIGHT_HAND]) {
            if (!isTabletHighlighted) {
                Selection.addToSelectedItemsList(TABLET_GRABBABLE_SELECTION_NAME, "overlay", HMD.tabletID);
                isTabletHighlighted = true;
            }
        } else {
            if (isTabletHighlighted) {
                Selection.removeFromSelectedItemsList(TABLET_GRABBABLE_SELECTION_NAME, "overlay", HMD.tabletID);
                isTabletHighlighted = false;
            }
        }
    }

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
            setTabletNearGrabbable(this.hand, false);
            return makeRunningValues(false, [], []);
        };

        this.run = function (controllerData) {
            if (!this.isNearTablet(controllerData)) {
                setTabletNearGrabbable(this.hand, false);
                return makeRunningValues(false, [], []);
            }

            if (controllerData.triggerClicks[this.hand] || controllerData.secondaryValues[this.hand]) {
                setTabletNearGrabbable(this.hand, false);
                return makeRunningValues(false, [], []);
            }

            setTabletNearGrabbable(this.hand, true);
            return makeRunningValues(true, [], []);
        };
    }

    var leftNearTabletHighlight = new NearTabletHighlight(LEFT_HAND);
    var rightNearTabletHighlight = new NearTabletHighlight(RIGHT_HAND);
    enableDispatcherModule("LeftNearTabletHighlight", leftNearTabletHighlight);
    enableDispatcherModule("RightNearTabletHighlight", rightNearTabletHighlight);

    function onDisplayModeChanged() {
        if (HMD.active) {
            Selection.enableListHighlight(TABLET_GRABBABLE_SELECTION_NAME, TABLET_GRABBABLE_SELECTION_STYLE);
        } else {
            Selection.disableListHighlight(TABLET_GRABBABLE_SELECTION_NAME);
            Selection.clearSelectedItemsList(TABLET_GRABBABLE_SELECTION_NAME);
        }
    }
    HMD.displayModeChanged.connect(onDisplayModeChanged);
    HMD.mountedChanged.connect(onDisplayModeChanged);
    onDisplayModeChanged();

    function cleanUp() {
        disableDispatcherModule("LeftNearTabletHighlight");
        disableDispatcherModule("RightNearTabletHighlight");
        Selection.disableListHighlight(TABLET_GRABBABLE_SELECTION_NAME);
    }
    Script.scriptEnding.connect(cleanUp);

}());
