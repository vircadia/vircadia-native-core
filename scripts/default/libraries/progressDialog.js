//
//  progressDialog.js
//  examples/libraries
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
var toolIconUrl = HIFI_PUBLIC_BUCKET + "images/tools/";

progressDialog = (function () {
    var that = {},
        progressBackground,
        progressMessage,
        cancelButton,
        displayed = false,
        backgroundWidth = 300,
        backgroundHeight = 100,
        messageHeight = 32,
        cancelWidth = 70,
        cancelHeight = 32,
        textColor = { red: 255, green: 255, blue: 255 },
        textBackground = { red: 52, green: 52, blue: 52 },
        backgroundUrl = toolIconUrl + "progress-background.svg",
        windowDimensions;

    progressBackground = Overlays.addOverlay("image", {
        width: backgroundWidth,
        height: backgroundHeight,
        imageURL: backgroundUrl,
        alpha: 0.9,
        visible: false
    });

    progressMessage = Overlays.addOverlay("text", {
        width: backgroundWidth - 40,
        height: messageHeight,
        text: "",
        textColor: textColor,
        backgroundColor: textBackground,
        alpha: 0.9,
        backgroundAlpha: 0.9,
        visible: false
    });

    cancelButton = Overlays.addOverlay("text", {
        width: cancelWidth,
        height: cancelHeight,
        text: "Cancel",
        textColor: textColor,
        backgroundColor: textBackground,
        alpha: 0.9,
        backgroundAlpha: 0.9,
        visible: false
    });

    function move() {
        var progressX,
            progressY;

        if (displayed) {

            if (windowDimensions.x === Window.innerWidth && windowDimensions.y === Window.innerHeight) {
                return;
            }
            windowDimensions.x = Window.innerWidth;
            windowDimensions.y = Window.innerHeight;

            progressX = (windowDimensions.x - backgroundWidth) / 2;  // Center.
            progressY = windowDimensions.y / 2 - backgroundHeight;  // A little up from center.

            Overlays.editOverlay(progressBackground, { x: progressX, y: progressY });
            Overlays.editOverlay(progressMessage, { x: progressX + 20, y: progressY + 15 });
            Overlays.editOverlay(cancelButton, {
                x: progressX + backgroundWidth - cancelWidth - 20,
                y: progressY + backgroundHeight - cancelHeight - 15
            });
        }
    }
    that.move = move;

    that.onCancel = undefined;

    function open(message) {
        if (!displayed) {
            windowDimensions = { x: 0, y : 0 };
            displayed = true;
            move();
            Overlays.editOverlay(progressBackground, { visible: true });
            Overlays.editOverlay(progressMessage, { visible: true, text: message });
            Overlays.editOverlay(cancelButton, { visible: true });
        } else {
            throw new Error("open() called on progressDialog when already open");
        }
    }
    that.open = open;

    function isOpen() {
        return displayed;
    }
    that.isOpen = isOpen;

    function update(message) {
        if (displayed) {
            Overlays.editOverlay(progressMessage, { text: message });
        } else {
            throw new Error("update() called on progressDialog when not open");
        }
    }
    that.update = update;

    function close() {
        if (displayed) {
            Overlays.editOverlay(cancelButton, { visible: false });
            Overlays.editOverlay(progressMessage, { visible: false });
            Overlays.editOverlay(progressBackground, { visible: false });
            displayed = false;
        } else {
            throw new Error("close() called on progressDialog when not open");
        }
    }
    that.close = close;

    function mousePressEvent(event) {
        if (Overlays.getOverlayAtPoint({ x: event.x, y: event.y }) === cancelButton) {
            if (typeof this.onCancel === "function") {
                close();
                this.onCancel();
            }
            return true;
        }
        return false;
    }
    that.mousePressEvent = mousePressEvent;

    function cleanup() {
        Overlays.deleteOverlay(cancelButton);
        Overlays.deleteOverlay(progressMessage);
        Overlays.deleteOverlay(progressBackground);
    }
    that.cleanup = cleanup;

    return that;
}());
