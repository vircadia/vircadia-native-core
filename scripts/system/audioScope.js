"use strict";
//
//  audioScope.js
//  scripts/system/
//
//  Created by Brad Hefta-Gaub on 3/10/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* global Script, Tablet, AudioScope, Audio */

(function () { // BEGIN LOCAL_SCOPE

    var scopeVisibile = AudioScope.getVisible();
    var scopePaused = AudioScope.getPause();
    var autoPause = false;

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var showScopeButton = tablet.addButton({
        icon: "http://s3.amazonaws.com/hifi-public/tony/icons/star.png",
        text: "Audio Scope",
        isActive: scopeVisibile
    });

    var pauseScopeButton = tablet.addButton({
        icon: "http://s3.amazonaws.com/hifi-public/tony/icons/star.png",
        text: "Pause",
        isActive: scopePaused
    });

    var autoPauseScopeButton = tablet.addButton({
        icon: "http://s3.amazonaws.com/hifi-public/tony/icons/star.png",
        text: "Auto Pause",
        isActive: autoPause
    });

    function setScopePause(paused) {
        scopePaused = paused;
        pauseScopeButton.editProperties({
            isActive: scopePaused,
            text: scopePaused ? "Unpause" : "Pause"
        });
        AudioScope.setPause(scopePaused);
    }

    showScopeButton.clicked.connect(function () {
        // toggle button active state
        scopeVisibile = !scopeVisibile;
        showScopeButton.editProperties({
            isActive: scopeVisibile
        });

        AudioScope.setVisible(scopeVisibile);
    });

    pauseScopeButton.clicked.connect(function () {
        // toggle button active state
        setScopePause(!scopePaused);
    });

    autoPauseScopeButton.clicked.connect(function () {
        // toggle button active state
        autoPause = !autoPause;
        autoPauseScopeButton.editProperties({
            isActive: autoPause,
            text: autoPause ? "Auto Pause" : "Manual"
        });
    });

    Script.scriptEnding.connect(function () {
        tablet.removeButton(showScopeButton);
        tablet.removeButton(pauseScopeButton);
        tablet.removeButton(autoPauseScopeButton);
    });

    Audio.noiseGateOpened.connect(function(){
        if (autoPause) {
            setScopePause(false);
        }
    });

    Audio.noiseGateClosed.connect(function(){
        // noise gate closed
        if (autoPause) {
            setScopePause(true);
        }
    });

}()); // END LOCAL_SCOPE