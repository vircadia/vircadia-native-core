"use strict";
(function() {

    var ERROR_MESSAGE_MAP = [
        "Oops! Protocol version mismatch.",
        "Oops! Not authorized to join domain.",
        "Oops! Connection timed out.",
        "Oops! Something went wrong."
    ];

    var PROTOCOL_VERSION_MISMATCH = 1;
    var NOT_AUTHORIZED = 3;
    var TIMEOUT = 5;
    var hardRefusalErrors = [PROTOCOL_VERSION_MISMATCH,
        NOT_AUTHORIZED, TIMEOUT];
    var error = -1;
    var timer = null;

    function getOopsText() {
        error = Window.getLastDomainConnectionError();
        var errorMessageMapIndex = hardRefusalErrors.indexOf(error);
        if (errorMessageMapIndex >= 0) {
            return ERROR_MESSAGE_MAP[errorMessageMapIndex];
        } else {
            // some other text.
            return ERROR_MESSAGE_MAP[4];
        }
    };

    var redirectOopsText = Overlays.addOverlay("text3d", {
        name: "oopsText",
        localPosition: {x: 0.5691902160644531, y: 0.6403706073760986, z: 6.68358039855957},
        text: getOopsText(),
        textAlpha: 1,
        backgroundAlpha: 0,
        lineHeight: 0.13,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        drawInFront: true,
        grabbable: false,
        orientation: {x: 0.0, y: 0.5, z: 0.0, w: 0.87},
        parentID: MyAvatar.SELF_ID
    });
    print("redirect oops text = " + redirectOopsText);

    var tryAgainImage = Overlays.addOverlay("image3d", {
        name: "tryAgainImage",
        localPosition: { x: -0.15, y: -0.4, z: 0.0},
        url: Script.resourcesPath() + "images/interstitialPage/button_tryAgain.png",
        alpha: 1,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        drawInFront: true,
        grabbable: false,
        orientation: {x: 0.0, y: 0.5, z: 0, w: 0.87},
        parentID: redirectOopsText
    });
    print("try again image = " + tryAgainImage);
    Window.copyToClipboard(redirectOopsText);

    var backImage = Overlays.addOverlay("image3d", {
        name: "backImage",
        localPosition: { x: 1.0, y: -0.4, z: 0.0},
        url: Script.resourcesPath() + "images/interstitialPage/button_back.png",
        alpha: 1,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        drawInFront: true,
        grabbable: false,
        orientation: {x: 0.0, y: 0.5, z: 0, w: 0.87},
        parentID: redirectOopsText
    });
    print("back image = " + backImage);

    var TARGET_UPDATE_HZ = 60;
    var BASIC_TIMER_INTERVAL_MS = 1000 / TARGET_UPDATE_HZ;

    function toggleOverlays() {
        var overlaysVisible = false;
        error = Window.getLastDomainConnectionError();
        var errorMessageMapIndex = hardRefusalErrors.indexOf(error);
        var oopsText = "";
        if (error === -1) {
            overlaysVisible = false;
        } else if (errorMessageMapIndex >= 0) {
            overlaysVisible = true;
            oopsText = ERROR_MESSAGE_MAP[errorMessageMapIndex];
        } else {
            overlaysVisible = true;
            oopsText = ERROR_MESSAGE_MAP[4];
        }
        var properties = {
            visible: overlaysVisible
        };
        var oopsTextProperties = {
            visible: overlaysVisible,
            text: oopsText
        };

        Overlays.editOverlay(redirectOopsText, oopsTextProperties);
        Overlays.editOverlay(tryAgainImage, properties);
        Overlays.editOverlay(backImage, properties);
    }

    function clickedOnOverlay(overlayID, event) {
        var properties = {
            visible: false
        };
        if (tryAgainImage === overlayID) {
            location.goToLastAddress();
        } else if (backImage === overlayID) {
            location.goBack();
        }
        Overlays.editOverlay(redirectOopsText, properties);
        Overlays.editOverlay(tryAgainImage, properties);
        Overlays.editOverlay(backImage, properties);
    }

    function cleanup() {
        timer = null;
        Overlays.deleteOverlay(redirectOopsText);
        Overlays.deleteOverlay(tryAgainImage);
        Overlays.deleteOverlay(backImage);
    }

    var whiteColor = {red: 255, green: 255, blue: 255};
    var greyColor = {red: 125, green: 125, blue: 125};
    Overlays.mouseReleaseOnOverlay.connect(clickedOnOverlay);
    Overlays.hoverEnterOverlay.connect(function(overlayID, event) {
        if (overlayID === backImage || overlayID === tryAgainImage) {
            Overlays.editOverlay(overlayID, { color: greyColor });
        }
    });

    Overlays.hoverLeaveOverlay.connect(function(overlayID, event) {
        if (overlayID === backImage || overlayID === tryAgainImage) {
            Overlays.editOverlay(overlayID, { color: whiteColor });
        }
    });

    // timer = Script.setTimeout(update, BASIC_TIMER_INTERVAL_MS);
    Script.update.connect(toggleOverlays);

    Script.scriptEnding.connect(cleanup);
}());
