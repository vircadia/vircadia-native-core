//
//  walkSettings.js
//  version 0.1
//
//  Created by David Wooldridge, Summer 2015
//  Copyright © 2015 High Fidelity, Inc.
//
//  Presents settings for walk.js
//
//  Editing tools for animation data files available here: https://github.com/DaveDubUK/walkTools
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

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
            _webView.setVisible(_visible);
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
    
    var _control = false;
    var _shift = false;
    function keyPressEvent(event) {
        if (event.text === "CONTROL") {
            _control = true;
        }
        if (event.text === "SHIFT") {
            _shift = true;
        }        
        if (_shift && (event.text === 'o' || event.text === 'O')) {
            _visible = !_visible;
            _webView.setVisible(_visible);
        }
    }
    function keyReleaseEvent(event) {
        if (event.text === "CONTROL") {
            _control = false;
        }
        if (event.text === "SHIFT") {
            _shift = false;
        }           
    }    
    Controller.keyPressEvent.connect(keyPressEvent);  
    Controller.keyReleaseEvent.connect(keyReleaseEvent);      

    // web window
    var _url = Script.resolvePath('html/walkSettings.html');
    const PANEL_WIDTH = 200;
    const PANEL_HEIGHT = 180;
    var _webView = new WebWindow('Walk Settings', _url, PANEL_WIDTH, PANEL_HEIGHT, false);
    _webView.setVisible(false);

    _webView.eventBridge.webEventReceived.connect(function(data) {
        data = JSON.parse(data);

        if (data.type == "init") {
            // send the current settings to the dialog
            _webView.eventBridge.emitScriptEvent(JSON.stringify({
                type: "update",
                armsFree: avatar.armsFree,
                footstepSounds: avatar.makesFootStepSounds,
                blenderPreRotations: avatar.isBlenderExport
            }));
        } else if (data.type == "powerToggle") {
            motion.isLive = !motion.isLive;
        } else if (data.type == "update") {
            // receive settings from the dialog
            avatar.armsFree = data.armsFree;
            avatar.makesFootStepSounds = data.footstepSounds;
            avatar.isBlenderExport = data.blenderPreRotations;
        }
    });
};

walkSettings = WalkSettings();