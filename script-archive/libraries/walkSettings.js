//
//  walkSettings.js
//  version 0.1
//
//  Created by David Wooldridge, June 2015
//  Copyright © 2015 High Fidelity, Inc.
//
//  Presents settings for walk.js
//
//  Editing tools for animation data files available here: https://github.com/DaveDubUK/walkTools
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var WALK_SETTINGS_HTML_URL = Script.resolvePath('../html/walkSettings.html');

WalkSettings = function() {
    var _visible = false;
    var _innerWidth = Window.innerWidth;
    const MARGIN_RIGHT = 58;
    const MARGIN_TOP = 145;
    const ICON_SIZE = 50;
    const ICON_ALPHA = 0.9;

    var minimisedTab = Overlays.addOverlay("image", {
        x: _innerWidth - MARGIN_RIGHT, y: Window.innerHeight - MARGIN_TOP,
        width: ICON_SIZE, height: ICON_SIZE,
        imageURL: pathToAssets + 'overlay-images/ddpa-minimised-ddpa-tab.png',
        visible: true, alpha: ICON_ALPHA
    });

    function mousePressEvent(event) {
        if (Overlays.getOverlayAtPoint(event) === minimisedTab) {
            _visible = !_visible;
            _webWindow.setVisible(_visible);
        }
    }
    Controller.mousePressEvent.connect(mousePressEvent);

    Script.update.connect(function(deltaTime) {
        if (window.innerWidth !== _innerWidth) {
            _innerWidth = window.innerWidth;
            Overlays.EditOverlay(minimisedTab, {x: _innerWidth - MARGIN_RIGHT});
        }
    });

    function cleanup() {
        Overlays.deleteOverlay(minimisedTab);
    }
    Script.scriptEnding.connect(cleanup);

    var _shift = false;
    function keyPressEvent(event) {
        if (event.text === "SHIFT") {
            _shift = true;
        }
        if (_shift && (event.text === 'o' || event.text === 'O')) {
            _visible = !_visible;
            _webWindow.setVisible(_visible);
        }
    }
    function keyReleaseEvent(event) {
        if (event.text === "SHIFT") {
            _shift = false;
        }
    }
    Controller.keyPressEvent.connect(keyPressEvent);
    Controller.keyReleaseEvent.connect(keyReleaseEvent);

    // web window
    const PANEL_WIDTH = 200;
    const PANEL_HEIGHT = 180;
    var _url = WALK_SETTINGS_HTML_URL;
    var _webWindow = new WebWindow('Walk Settings', _url, PANEL_WIDTH, PANEL_HEIGHT, false);
    _webWindow.setVisible(false);
    _webWindow.eventBridge.webEventReceived.connect(function(data) {
        data = JSON.parse(data);

        if (data.type == "init") {
            // send the current settings to the window
            _webWindow.eventBridge.emitScriptEvent(JSON.stringify({
                type: "update",
                armsFree: avatar.armsFree,
                makesFootStepSounds: avatar.makesFootStepSounds,
                blenderPreRotations: avatar.blenderPreRotations
            }));
        } else if (data.type == "powerToggle") {
            motion.isLive = !motion.isLive;
        } else if (data.type == "update") {
            // receive settings from the window
            avatar.armsFree = data.armsFree;
            avatar.makesFootStepSounds = data.makesFootStepSounds;
            avatar.blenderPreRotations = data.blenderPreRotations;
        }
    });
};

walkSettings = WalkSettings();
