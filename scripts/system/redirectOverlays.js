"use strict";
(function() {

    var ERROR_MESSAGE_MAP = [
        "Oops! Protocol version mismatch.",
        "Oops! Not authorized to join domain.",
        "Oops! Connection timed out.",
        "Oops! The domain is full.",
        "Oops! Something went wrong."
    ];

    var PROTOCOL_VERSION_MISMATCH = 1;
    var NOT_AUTHORIZED = 3;
    var DOMAIN_FULL = 4;
    var TIMEOUT = 5;
    var hardRefusalErrors = [
        PROTOCOL_VERSION_MISMATCH,
        NOT_AUTHORIZED,
        TIMEOUT,
        DOMAIN_FULL
    ];
    var timer = null;
    var isErrorState = false;

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
            return ERROR_MESSAGE_MAP[ERROR_MESSAGE_MAP.length - 1];
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
        ignoreRayIntersection: true,
        dimensions: oopsDimensions,
        grabbable: false,
    });

    var tryAgainImageNeutral = Overlays.addOverlay("image3d", {
        name: "tryAgainImage",
        localPosition: {x: -0.6, y: -0.6, z: 0.0},
        url: Script.resourcesPath() + "images/interstitialPage/button.png",
        alpha: 1,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        grabbable: false,
        orientation: Overlays.getProperty(redirectOopsText, "orientation"),
        parentID: redirectOopsText
    });

    var tryAgainImageHover = Overlays.addOverlay("image3d", {
        name: "tryAgainImageHover",
        localPosition: {x: -0.6, y: -0.6, z: 0.0},
        url: Script.resourcesPath() + "images/interstitialPage/button_hover.png",
        alpha: 1,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        grabbable: false,
        orientation: Overlays.getProperty(redirectOopsText, "orientation"),
        parentID: redirectOopsText
    });

    var tryAgainText = Overlays.addOverlay("text3d", {
        name: "tryAgainText",
        localPosition: {x: -0.6, y: -0.962, z: 0.0},
        text: "Try Again",
        textAlpha: 1,
        backgroundAlpha: 0.00393,
        lineHeight: 0.08,
        visible: false,
        emissive: true,
        ignoreRayIntersection: true,
        grabbable: false,
        orientation: Overlays.getProperty(redirectOopsText, "orientation"),
        parentID: redirectOopsText
    });

    var backImageNeutral = Overlays.addOverlay("image3d", {
        name: "backImage",
        localPosition: {x: 0.6, y: -0.6, z: 0.0},
        url: Script.resourcesPath() + "images/interstitialPage/button.png",
        alpha: 1,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        grabbable: false,
        orientation: Overlays.getProperty(redirectOopsText, "orientation"),
        parentID: redirectOopsText
    });

    var backImageHover = Overlays.addOverlay("image3d", {
        name: "backImageHover",
        localPosition: {x: 0.6, y: -0.6, z: 0.0},
        url: Script.resourcesPath() + "images/interstitialPage/button_hover.png",
        alpha: 1,
        visible: false,
        emissive: true,
        ignoreRayIntersection: false,
        grabbable: false,
        orientation: Overlays.getProperty(redirectOopsText, "orientation"),
        parentID: redirectOopsText
    });

    var backText = Overlays.addOverlay("text3d", {
        name: "backText",
        localPosition: {x: 0.6, y: -0.962, z: 0.0},
        text: "Back",
        textAlpha: 1,
        backgroundAlpha: 0.00393,
        lineHeight: 0.08,
        visible: false,
        emissive: true,
        ignoreRayIntersection: true,
        grabbable: false,
        orientation: Overlays.getProperty(redirectOopsText, "orientation"),
        parentID: redirectOopsText
    });

    function toggleOverlays(isInErrorState) {
        isErrorState = isInErrorState;
        if (!isInErrorState) {
            var properties = {
                visible: false
            };

            Overlays.editOverlay(redirectOopsText, properties);
            Overlays.editOverlay(tryAgainImageNeutral, properties);
            Overlays.editOverlay(tryAgainImageHover, properties);
            Overlays.editOverlay(backImageNeutral, properties);
            Overlays.editOverlay(backImageHover, properties);
            Overlays.editOverlay(tryAgainText, properties);
            Overlays.editOverlay(backText, properties);
            return;
        }
        var oopsText = getOopsText();
        // if oopsText === "", it was a success.
        var overlaysVisible = (oopsText !== "");
        // for catching init or if error state were to be different.
        isErrorState = overlaysVisible;
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

        var tryAgainTextWidth = Overlays.textSize(tryAgainText, "Try Again").width;
        var tryAgainImageWidth = Overlays.getProperty(tryAgainImageNeutral, "dimensions").x;

        var tryAgainTextProperties = {
            visible: overlaysVisible,
            leftMargin: (tryAgainImageWidth - tryAgainTextWidth) / 2
        };

        var backTextWidth = Overlays.textSize(backText, "Back").width;
        var backImageWidth = Overlays.getProperty(backImageNeutral, "dimensions").x;

        var backTextProperties = {
            visible: overlaysVisible,
            leftMargin: (backImageWidth - backTextWidth) / 2
        };

        Overlays.editOverlay(redirectOopsText, oopsTextProperties);
        Overlays.editOverlay(tryAgainImageNeutral, properties);
        Overlays.editOverlay(backImageNeutral, properties);
        Overlays.editOverlay(tryAgainImageHover, {visible: false});
        Overlays.editOverlay(backImageHover, {visible: false});
        Overlays.editOverlay(tryAgainText, tryAgainTextProperties);
        Overlays.editOverlay(backText, backTextProperties);

    }

    function clickedOnOverlay(overlayID, event) {
        if (event.isRightButton) {
            // don't allow right-clicks.
            return;
        }
        if (tryAgainImageHover === overlayID) {
            location.goToLastAddress();
        } else if (backImageHover === overlayID && location.canGoBack()) {
            location.goBack();
        }
    }

    function cleanup() {
        Script.clearInterval(timer);
        timer = null;
        Overlays.deleteOverlay(redirectOopsText);
        Overlays.deleteOverlay(tryAgainImageNeutral);
        Overlays.deleteOverlay(backImageNeutral);
        Overlays.deleteOverlay(tryAgainImageHover);
        Overlays.deleteOverlay(backImageHover);
        Overlays.deleteOverlay(tryAgainText);
        Overlays.deleteOverlay(backText);
    }

    toggleOverlays(true);

    Overlays.mouseReleaseOnOverlay.connect(clickedOnOverlay);
    Overlays.hoverEnterOverlay.connect(function(overlayID, event) {
        if (!isErrorState) {
            // don't allow hover overlay events to get caught if it's not in error state anymore.
            return;
        }
        if (overlayID === backImageNeutral && location.canGoBack()) {
            Overlays.editOverlay(backImageNeutral, {visible: false});
            Overlays.editOverlay(backImageHover, {visible: true});
        }
        if (overlayID === tryAgainImageNeutral) {
            Overlays.editOverlay(tryAgainImageNeutral, {visible: false});
            Overlays.editOverlay(tryAgainImageHover, {visible: true});
        }
    });

    Overlays.hoverLeaveOverlay.connect(function(overlayID, event) {
        if (!isErrorState) {
            // don't allow hover overlay events to get caught if it's not in error state anymore.
            return;
        }
        if (overlayID === backImageHover) {
            Overlays.editOverlay(backImageHover, {visible: false});
            Overlays.editOverlay(backImageNeutral, {visible: true});
        }
        if (overlayID === tryAgainImageHover) {
            Overlays.editOverlay(tryAgainImageHover, {visible: false});
            Overlays.editOverlay(tryAgainImageNeutral, {visible: true});
        }
    });

    Window.redirectErrorStateChanged.connect(toggleOverlays);

    Script.scriptEnding.connect(cleanup);
}());
