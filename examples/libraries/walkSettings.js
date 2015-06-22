//
//  walkSettings.js
//  version 1.0
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

WalkSettings = function() {
    var that = {};
    
    // ui minimised tab
    var _innerWidth = Window.innerWidth;
    var visible = false;
    var _minimisedTab = Overlays.addOverlay("image", {
        x: _innerWidth - 58, y: Window.innerHeight - 145,
        width: 50, height: 50,
        imageURL: pathToAssets + 'overlay-images/ddpa-minimised-ddpa-tab.png',
        visible: true, alpha: 0.9
    });
    function mousePressEvent(event) {
        if (Overlays.getOverlayAtPoint(event)  === _minimisedTab) {
            visible = !visible;
            webView.setVisible(visible);
        }
    }
    Controller.mousePressEvent.connect(mousePressEvent);
    function cleanup() {
        Overlays.deleteOverlay(_minimisedTab);
    }
    Script.update.connect(function() {
        
        if (_innerWidth !== Window.innerWidth) {
            _innerWidth = Window.innerWidth;
            Overlays.editOverlay(_minimisedTab, {x: _innerWidth - 58});
        }
    });
    Script.scriptEnding.connect(cleanup);

    // web window
    var url = Script.resolvePath('html/walkSettings.html');
    var webView = new WebWindow('Walk Settings', url, 200, 180, false);
    webView.setVisible(false);

    webView.eventBridge.webEventReceived.connect(function(data) {
        data = JSON.parse(data);

        if (data.type == "init") {
            // send the current settings to the dialog
            webView.eventBridge.emitScriptEvent(JSON.stringify({
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

    return that;
};