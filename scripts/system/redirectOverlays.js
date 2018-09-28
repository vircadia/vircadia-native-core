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
        if (error === -1) {
            // not an error.
            return "";
        } else if (errorMessageMapIndex >= 0) {
            return ERROR_MESSAGE_MAP[errorMessageMapIndex];
        } else {
            // some other text.
            return ERROR_MESSAGE_MAP[4];
        }
    };

    var oopsDimensions = {x: 4.2, y: 0.8};

    var redirectOopsText = Overlays.addOverlay("text3d", {
        name: "oopsText",
        position: {x: 0, y: 1.6763916015625, z: 1.45927095413208},
        rotation: {x: -4.57763671875e-05, y: 0.4957197904586792, z: -7.62939453125e-05, w: 0.8684672117233276},
        text: getOopsText(),
        textAlpha: 1,
        backgroundColor: {x: 0, y: 0, z:0},
        backgroundAlpha: 0,
        lineHeight: 0.10,
        leftMargin: 0.538373570564886,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        dimensions: oopsDimensions,
        grabbable: false,
    });

    var tryAgainImage = Overlays.addOverlay("image3d", {
        name: "tryAgainImage",
        localPosition: {x: -0.6, y: -0.6, z: 0.0},
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
        localPosition: {x: 0.6, y: -0.6, z: 0.0},
        url: Script.resourcesPath() + "images/interstitialPage/button_back.png",
        alpha: 1,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        grabbable: false,
        orientation: Overlays.getProperty(redirectOopsText, "orientation"),
        parentID: redirectOopsText
    });

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
        var oopsText = getOopsText();
        // if oopsText === "", it was a success.
        var overlaysVisible = (oopsText !== "");
        var properties = {
            visible: overlaysVisible
        };

        var textWidth = Overlays.textSize(redirectOopsText, oopsText).width;
        var textOverlayWidth = oopsDimensions.x;

        var oopsTextProperties = {
            visible: overlaysVisible,
            text: oopsText,
            textAlpha: overlaysVisible,
            // either visible or invisible. 0 doesn't work in Mac.
            backgroundAlpha: overlaysVisible * 0.00393,
            leftMargin: (textOverlayWidth - textWidth) / 2
        };
        Window.copyToClipboard(redirectOopsText);

        Overlays.editOverlay(redirectOopsText, oopsTextProperties);
        Overlays.editOverlay(tryAgainImage, properties);
        Overlays.editOverlay(backImage, properties);

    }

    function clickedOnOverlay(overlayID, event) {
        if (event.isRightButton) {
            // don't allow right-clicks.
            return;
        }
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
