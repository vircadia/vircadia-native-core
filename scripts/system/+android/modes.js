"use strict";
//
//  modes.js
//  scripts/system/
//
//  Created by Gabriel Calero & Cristian Duarte on Jan 23, 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() { // BEGIN LOCAL_SCOPE

var modesbar;
var modesButtons;
var currentSelectedBtn;

var SETTING_CURRENT_MODE_KEY = 'Android/Mode';
var MODE_VR = "VR", MODE_RADAR = "RADAR", MODE_MY_VIEW = "MY VIEW";
var DEFAULT_MODE = MODE_MY_VIEW;
var logEnabled = true;

var radar = Script.require('./radar.js');
var uniqueColor = Script.require('./uniqueColor.js');

var modesInterface = {};

function printd(str) {
    if (logEnabled) {       
        print("[modes.js] " + str);
    }
}

function init() {
    radar.setUniqueColor(uniqueColor);
    radar.init();
    setupModesBar();
    radar.isTouchValid = isRadarModeValidTouch;
}

function shutdown() {

}

function setupModesBar() {

    var bar = new QmlFragment({
        qml: "hifi/modesbar.qml"
    });
    var buttonRadarMode = bar.addButton({
        icon: "icons/radar-i.svg",
        activeIcon: "icons/radar-a.svg",
        hoverIcon: "icons/radar-a.svg",
        activeBgOpacity: 0.0,
        hoverBgOpacity: 0.0,
        activeHoverBgOpacity: 0.0,
        text: "RADAR",
        height:240,
        bottomMargin: 6,
        textSize: 45
    });
    var buttonMyViewMode = bar.addButton({
        icon: "icons/myview-i.svg",
        activeIcon: "icons/myview-a.svg",
        hoverIcon: "icons/myview-a.svg",
        activeBgOpacity: 0.0,
        hoverBgOpacity: 0.0,
        activeHoverBgOpacity: 0.0,
        text: "MY VIEW",
        height: 240,
        bottomMargin: 6,
        textSize: 45
    });
    
    modesButtons = [buttonRadarMode, buttonMyViewMode];

    var mode = getCurrentModeSetting();

    var buttonsRevealed = false;
    bar.sendToQml({type: "inactiveButtonsHidden"});

    modesbar = {
        restoreMyViewButton: function() {
            switchModeButtons(buttonMyViewMode);
            saveCurrentModeSetting(MODE_MY_VIEW);
        },
        sendToQml: function(o) { bar.sendToQml(o); },
        qmlFragment: bar
    };

    buttonRadarMode.clicked.connect(function() {
        //if (connections.isVisible()) return;
        saveCurrentModeSetting(MODE_RADAR);
        printd("Radar clicked");
        onButtonClicked(buttonRadarMode, function() {
            radar.startRadarMode();
        });
    });
    buttonMyViewMode.clicked.connect(function() {
        //if (connections.isVisible()) return;
        saveCurrentModeSetting(MODE_MY_VIEW);
        printd("My View clicked");
        onButtonClicked(buttonMyViewMode, function() {
            if (currentSelectedBtn == buttonRadarMode) {
                radar.endRadarMode();
            }
        });
    });

    var savedButton;
    if (mode == MODE_MY_VIEW) {
        savedButton = buttonMyViewMode;
    } else {
        savedButton = buttonRadarMode;
    }
    printd("[MODE] previous mode " + mode);

    savedButton.clicked();
}

function saveCurrentModeSetting(mode) {
    Settings.setValue(SETTING_CURRENT_MODE_KEY, mode);
}

function getCurrentModeSetting(mode) {
    return Settings.getValue(SETTING_CURRENT_MODE_KEY, DEFAULT_MODE);
}

function showAllButtons() {
    for (var i=0; i<modesButtons.length; i++) {
        modesButtons[i].visible = true;
    }
    buttonsRevealed = true;
    print("Modesbar: " + modesbar);
    modesbar.sendToQml({type: "allButtonsShown"});
}

function hideAllButtons() {
    for (var i=0; i<modesButtons.length; i++) {
        modesButtons[i].visible = false;
    }
}

function hideOtherButtons(thisButton) {
    printd("Hiding all but " + thisButton);
    printd("modesButtons: " + modesButtons.length);
    for (var i=0; i<modesButtons.length; i++) {
        if (modesButtons[i] != thisButton) {
            printd("----Hiding " + thisButton);
            modesButtons[i].visible = false;
        } else {
            // be sure to keep it visible
            modesButtons[i].visible = true;
        }
    }
    buttonsRevealed = false;
    print("Modesbar: " + modesbar);
    modesbar.sendToQml({type: "inactiveButtonsHidden"});
}

function switchModeButtons(clickedButton, hideAllAfter) {
    if (currentSelectedBtn) {
        currentSelectedBtn.isActive = false;
    }
    currentSelectedBtn = clickedButton;
    clickedButton.isActive = true;
    if (hideAllAfter) {
        hideAllButtons();
    } else {
        hideOtherButtons(clickedButton);
    }
}

function onButtonClicked(clickedButton, whatToDo, hideAllAfter) {
    if (currentSelectedBtn == clickedButton) {
        if (buttonsRevealed) {
            hideOtherButtons(clickedButton);
        } else {
            showAllButtons();
        }
    } else {
        // mode change
        whatToDo();
        switchModeButtons(clickedButton, hideAllAfter);
    }
}

function isRadarModeValidTouch(coords) {
    if (!modesInterface.isRadarModeValidTouch(coords)) {
        printd("isRadarModeValidTouch- false because of modesInterface");
        return false;
    }
    printd("isRadarModeValidTouch- modesInterface is true, evaluating modesbar qmls..");

    var qmlFragments = [modesbar.qmlFragment];
    var windows = [];
    for (var i=0; i < qmlFragments.length; i++) {
        var aQmlFrag = qmlFragments[i];
        if (aQmlFrag != null && aQmlFrag.isVisible() &&
            coords.x >= aQmlFrag.position.x && coords.x <= aQmlFrag.position.x + aQmlFrag.size.x &&
            coords.y >= aQmlFrag.position.y && coords.y <= aQmlFrag.position.y + aQmlFrag.size.y
           ) {
            printd("isRadarModeValidTouch- false because of qmlFragments!? idx " + i);
            return false;
        }
    }

    for (var i=0; i < windows.length; i++) {
        var aWin = windows[i];
        if (aWin != null && aWin.position() != null && aWin.isVisible() &&
            coords.x >= aWin.position().x && coords.x <= aWin.position().x + aWin.width() &&
            coords.y >= aWin.position().y && coords.y <= aWin.position().y + aWin.height()
        ) {
            printd("isRadarModeValidTouch- false because of windows!? idx " + i);
            return false;
        }
    }
    printd("isRadarModeValidTouch- true by default ");
    return true;
}

// default
modesInterface.isRadarModeValidTouch = function(coords) {return true;};

module.exports = modesInterface;

Script.scriptEnding.connect(function () {
    shutdown();
});

init();

}()); // END LOCAL_SCOPE