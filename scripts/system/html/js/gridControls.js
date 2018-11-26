//  gridControls.js
//
//  Created by Ryan Huffman on 6 Nov 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

function loaded() {
    openEventBridge(function() {
        elPosY = document.getElementById("horiz-y");
        elMinorSpacing = document.getElementById("minor-spacing");
        elMajorSpacing = document.getElementById("major-spacing");
        elSnapToGrid = document.getElementById("snap-to-grid");
        elHorizontalGridVisible = document.getElementById("horiz-grid-visible");
        elMoveToSelection = document.getElementById("move-to-selection");
        elMoveToAvatar = document.getElementById("move-to-avatar");

        if (window.EventBridge !== undefined) {
            EventBridge.scriptEventReceived.connect(function(data) {
                data = JSON.parse(data);

                if (data.origin) {
                    var origin = data.origin;
                    elPosY.value = origin.y;
                }

                if (data.minorGridEvery !== undefined) {
                    elMinorSpacing.value = data.minorGridEvery;
                }

                if (data.majorGridEvery !== undefined) {
                    elMajorSpacing.value = data.majorGridEvery;
                }

                if (data.gridColor) {
                    gridColor = data.gridColor;
                }

                if (data.snapToGrid !== undefined) {
                    elSnapToGrid.checked = data.snapToGrid == true;
                }

                if (data.visible !== undefined) {
                    elHorizontalGridVisible.checked = data.visible == true;
                }
            });

            function emitUpdate() {
                EventBridge.emitWebEvent(JSON.stringify({
                    type: "update",
                    origin: {
                        y: elPosY.value,
                    },
                    minorGridEvery: elMinorSpacing.value,
                    majorGridEvery: elMajorSpacing.value,
                    gridColor: gridColor,
                    snapToGrid: elSnapToGrid.checked,
                    visible: elHorizontalGridVisible.checked,
                }));
            }

        }

        elPosY.addEventListener("change", emitUpdate);
        elMinorSpacing.addEventListener("change", emitUpdate);
        elMajorSpacing.addEventListener("change", emitUpdate);
        elSnapToGrid.addEventListener("change", emitUpdate);
        elHorizontalGridVisible.addEventListener("change", emitUpdate);

        elMoveToAvatar.addEventListener("click", function() {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "moveToAvatar",
            }));
        });
        elMoveToSelection.addEventListener("click", function() {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "action",
                action: "moveToSelection",
            }));
        });

        var gridColor = { red: 255, green: 255, blue: 255 };
        var elColor = document.getElementById("grid-color");
        elColor.style.backgroundColor = "rgb(" + gridColor.red + "," + gridColor.green + "," + gridColor.blue + ")";

        var colorPickFunction = function (red, green, blue) {
            gridColor = { red: red, green: green, blue: blue };
            emitUpdate();
        };

        $('#grid-color').colpick({
            colorScheme: 'dark',
            layout: 'rgbhex',
            color: { r: gridColor.red, g: gridColor.green, b: gridColor.blue },
            submit: false,
            onShow: function (colpick) {
                $('#grid-color').attr('active', 'true');
            },
            onHide: function (colpick) {
                $('#grid-color').attr('active', 'false');
            },
            onChange: function (hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                colorPickFunction(rgb.r, rgb.g, rgb.b);
            }
        });

        augmentSpinButtons();

        EventBridge.emitWebEvent(JSON.stringify({ type: 'init' }));
    });

    const KEY_CODES = {
        BACKSPACE: 8,
        DELETE: 46
    };

    document.addEventListener("keyup", function (keyUpEvent) {
        if (keyUpEvent.target.nodeName === "INPUT") {
            return;
        }
        let {code, key, keyCode, altKey, ctrlKey, metaKey, shiftKey} = keyUpEvent;

        let controlKey = window.navigator.platform.startsWith("Mac") ? metaKey : ctrlKey;

        let keyCodeString;
        switch (keyCode) {
            case KEY_CODES.DELETE:
                keyCodeString = "Delete";
                break;
            case KEY_CODES.BACKSPACE:
                keyCodeString = "Backspace";
                break;
            default:
                keyCodeString = String.fromCharCode(keyUpEvent.keyCode);
                break;
        }

        EventBridge.emitWebEvent(JSON.stringify({
            type: 'keyUpEvent',
            keyUpEvent: {
                code,
                key,
                keyCode,
                keyCodeString,
                altKey,
                controlKey,
                shiftKey,
            }
        }));
    }, false);

    // Disable right-click context menu which is not visible in the HMD and makes it seem like the app has locked
    document.addEventListener("contextmenu", function (event) {
        event.preventDefault();
    }, false);
}
