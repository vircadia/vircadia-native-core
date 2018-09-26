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
    var timer = null;

    function getOopsText() {
        var error = Window.getLastDomainConnectionError();
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
        localPosition: {x: 0.2691902160644531, y: 0.6403706073760986, z: 3.18358039855957},
        localRotation: Quat.fromPitchYawRollDegrees(0.0, 180.0, 0.0),
        text: getOopsText(),
        textAlpha: 1,
        backgroundAlpha: 0,
        color: {x: 255, y: 255, z: 255},
        lineHeight: 0.10,
        leftMargin: 0.538373570564886,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        dimensions: {x: 4.2, y: 1},
        grabbable: false,
        parentID: MyAvatar.SELF_ID,
        parentJointIndex: MyAvatar.getJointIndex("Head")
    });

    var tryAgainImage = Overlays.addOverlay("image3d", {
        name: "tryAgainImage",
        localPosition: {x: -0.6, y: -0.4, z: 0.0},
        url: Script.resourcesPath() + "images/interstitialPage/button_tryAgain.png",
        alpha: 1,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        grabbable: false,
        orientation: Overlays.getProperty(redirectOopsText, "orientation"),
        parentID: redirectOopsText
    });

    var backImage = Overlays.addOverlay("image3d", {
        name: "backImage",
        localPosition: {x: 0.6, y: -0.4, z: 0.0},
        url: Script.resourcesPath() + "images/interstitialPage/button_back.png",
        alpha: 1,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        grabbable: false,
        orientation: Overlays.getProperty(redirectOopsText, "orientation"),
        parentID: redirectOopsText
    });

    var TARGET_UPDATE_HZ = 60;
    var BASIC_TIMER_INTERVAL_MS = 1000 / TARGET_UPDATE_HZ;

    function toggleOverlays(isInErrorState) {
        if (!isInErrorState) {
            var properties = {
                visible: false
            };

            Overlays.editOverlay(redirectOopsText, properties);
            Overlays.editOverlay(tryAgainImage, properties);
            Overlays.editOverlay(backImage, properties);
            return;
        }
        var overlaysVisible = false;
        var error = Window.getLastDomainConnectionError();
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

        var textWidth = Overlays.textSize(redirectOopsText, oopsText).width;
        var textOverlayWidth = Overlays.getProperty(redirectOopsText, "dimensions").x;

        var oopsTextProperties = {
            visible: overlaysVisible,
            text: oopsText,
            leftMargin: (textOverlayWidth - textWidth) / 2
        };

        Overlays.editOverlay(redirectOopsText, oopsTextProperties);
        Overlays.editOverlay(tryAgainImage, properties);
        Overlays.editOverlay(backImage, properties);

    }

    function clickedOnOverlay(overlayID, event) {
        if (tryAgainImage === overlayID) {
            location.goToLastAddress();
        } else if (backImage === overlayID) {
            location.goBack();
        }
    }

    function cleanup() {
        Script.clearInterval(timer);
        timer = null;
        Overlays.deleteOverlay(redirectOopsText);
        Overlays.deleteOverlay(tryAgainImage);
        Overlays.deleteOverlay(backImage);
    }

    toggleOverlays(true);

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

    Window.redirectErrorStateChanged.connect(toggleOverlays);

    Script.scriptEnding.connect(cleanup);
}());
