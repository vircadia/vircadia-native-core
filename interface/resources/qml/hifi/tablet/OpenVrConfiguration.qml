//
//  Created by Dante Ruiz on 6/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4
import "../../styles-uit"
import "../../controls"
import "../../controls-uit" as HifiControls
import "."


Rectangle {
    id: openVrConfiguration

    width: parent.width
    height: parent.height
    anchors.fill: parent

    property int leftMargin: 75
    property int countDown: 0
    property string pluginName: ""
    property var displayInformation: null

    readonly property bool feetChecked: feetBox.checked
    readonly property bool hipsChecked: hipBox.checked
    readonly property bool chestChecked: chestBox.checked
    readonly property bool shouldersChecked: shoulderBox.checked
    readonly property bool hmdHead: headBox.checked
    readonly property bool headPuck: headPuckBox.checked
    readonly property bool handController: handBox.checked
    readonly property bool handPuck: handPuckBox.checked

    property int state: buttonState.disabled
    property var lastConfiguration: null

    HifiConstants { id: hifi }

    Component {  id: screen; CalibratingScreen {} }
    QtObject {
        id: buttonState
        readonly property int disabled: 0
        readonly property int apply: 1
        readonly property int applyAndCalibrate: 2
        readonly property int calibrate: 3

    }





    MouseArea {
        id: mouseArea

        anchors.fill: parent
        propagateComposedEvents: true
        onPressed: {
            parent.forceActiveFocus()
            mouse.accepted = false;
        }
    }

    color: hifi.colors.baseGray

    RalewayBold {
        id: head

        text: "Head:"
        size: 12

        color: "white"

        anchors.left: parent.left
        anchors.leftMargin: leftMargin
    }

    Row {
        id: headConfig
        anchors.top: head.bottom
        anchors.topMargin: 5
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 10
        spacing: 10

        HifiControls.CheckBox {
            id: headBox
            width: 15
            height: 15
            boxRadius: 7

            onClicked: {
                if (checked) {
                    headPuckBox.checked = false;
                } else {
                    checked = true;
                }
                sendConfigurationSettings();
            }
        }

        RalewayBold {
            size: 12
            text: "Vive HMD"
            color: hifi.colors.lightGrayText
        }

        HifiControls.CheckBox {
            id: headPuckBox
            width: 15
            height: 15
            boxRadius: 7

            onClicked: {
                if (checked) {
                    headBox.checked = false;
                } else {
                    checked = true;
                }
                sendConfigurationSettings();
            }
        }

        RalewayBold {
            size: 12
            text: "Tracker"
            color: hifi.colors.lightGrayText
        }
    }

    Row {
        id: headOffsets
        anchors.top: headConfig.bottom
        anchors.topMargin: 5
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 10
        spacing: 10
        visible: headPuckBox.checked
        HifiControls.SpinBox {
            id: headYOffset
            decimals: 1
            width: 112
            label: "Y Offset"
            suffix: " cm"
            minimumValue: -10
            stepSize: 1
            value: -5
            colorScheme: hifi.colorSchemes.dark

            onEditingFinished: {
                sendConfigurationSettings();
            }
        }


        HifiControls.SpinBox {
            id: headZOffset
            width: 112
            label: "Z Offset"
            minimumValue: -10
            stepSize: 1
            decimals: 1
            suffix: " cm"
            value: -5
            colorScheme: hifi.colorSchemes.dark

            onEditingFinished: {
                sendConfigurationSettings();
            }
        }
    }

    RalewayBold {
        id: hands

        text: "Hands:"
        size: 12

        color: "white"

        anchors.top: (headOffsets.visible ? headOffsets.bottom : headConfig.bottom)
        anchors.topMargin: (headOffsets.visible ? 22 : 10)
        anchors.left: parent.left
        anchors.leftMargin: leftMargin
    }

    Row {
        id: handConfig
        anchors.top: hands.bottom
        anchors.topMargin: 5
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 10
        spacing: 10

        HifiControls.CheckBox {
            id: handBox
            width: 15
            height: 15
            boxRadius: 7

            onClicked: {
                if (checked) {
                    handPuckBox.checked = false;
                } else {
                    checked = true;
                }
                sendConfigurationSettings();
            }
        }

        RalewayBold {
            size: 12
            text: "Controllers"
            color: hifi.colors.lightGrayText
        }

        HifiControls.CheckBox {
            id: handPuckBox
            width: 12
            height: 15
            boxRadius: 7

            onClicked: {
                if (checked) {
                    handBox.checked = false;
                } else {
                    checked = true;
                }
                sendConfigurationSettings();
            }
        }

        RalewayBold {
            size: 12
            text: "Trackers"
            color: hifi.colors.lightGrayText
        }
    }

    Row {
        id: handOffset
        visible: handPuckBox.checked
        anchors.top: handConfig.bottom
        anchors.topMargin: 5
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 10
        spacing: 10

        HifiControls.SpinBox {
            id: handYOffset
            decimals: 1
            width: 112
            suffix: " cm"
            label: "Y Offset"
            minimumValue: -10
            stepSize: 1
            colorScheme: hifi.colorSchemes.dark

            onEditingFinished: {
                sendConfigurationSettings();
            }
        }


        HifiControls.SpinBox {
            id: handZOffset
            width: 112
            label: "Z Offset"
            suffix: " cm"
            minimumValue: -10
            stepSize: 1
            decimals: 1
            colorScheme: hifi.colorSchemes.dark

            onEditingFinished: {
                sendConfigurationSettings();
            }
        }
    }

    RalewayBold {
        id: additional

        text: "Additional Trackers"
        size: 12

        color: hifi.colors.white

        anchors.top: (handOffset.visible ? handOffset.bottom : handConfig.bottom)
        anchors.topMargin: (handOffset.visible ? 22 : 10)
        anchors.left: parent.left
        anchors.leftMargin: leftMargin
    }

    RalewayRegular {
        id: info

        text: "See Recommended Tracker Placement"
        color: hifi.colors.blueHighlight
        size: 10
        anchors {
            left: additional.right
            leftMargin: 10
            verticalCenter: additional.verticalCenter
        }

        Rectangle {
            id: selected
            color: hifi.colors.blueHighlight

            width: info.width
            height: 1

            anchors {
                top: info.bottom
                topMargin: 1
                left: info.left
                right: info.right
            }

            visible: false
        }

        MouseArea {
            anchors.fill: parent;
            hoverEnabled: true

            onEntered: {
                selected.visible = true;
            }

            onExited: {
                selected.visible = false;
            }
            onClicked: {
                stack.messageVisible = true;
            }
        }
    }

    Row {
        id: feetConfig
        anchors.top: additional.bottom
        anchors.topMargin: 15
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 10
        spacing: 10

        HifiControls.CheckBox {
            id: feetBox
            width: 15
            height: 15
            boxRadius: 7

            onClicked: {
                if (!checked) {
                    shoulderBox.checked = false;
                    chestBox.checked = false;
                    hipBox.checked = false;
                }
                sendConfigurationSettings();
            }
        }

        RalewayBold {
            size: 12
            text: "Feet"
            color: hifi.colors.lightGrayText
        }
    }

    Row {
        id: hipConfig
        anchors.top: feetConfig.bottom
        anchors.topMargin: 15
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 10
        spacing: 10

        HifiControls.CheckBox {
            id: hipBox
            width: 15
            height: 15
            boxRadius: 7

            onClicked: {
                if (checked) {
                    feetBox.checked = true;
                }

                if (chestChecked) {
                    checked = true;
                }
                sendConfigurationSettings();
            }
        }

        RalewayBold {
            size: 12
            text: "Hips"
            color: hifi.colors.lightGrayText
        }

        RalewayRegular {
            size: 12
            text: "requires feet"
            color: hifi.colors.lightGray
        }
    }


     Row {
        id: chestConfig
        anchors.top: hipConfig.bottom
        anchors.topMargin: 15
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 10
        spacing: 10

        HifiControls.CheckBox {
            id: chestBox
            width: 15
            height: 15
            boxRadius: 7

            onClicked: {
                if (checked) {
                    hipBox.checked = true;
                    feetBox.checked = true;
                    shoulderBox.checked = false;
                }
                sendConfigurationSettings();
            }
        }

        RalewayBold {
            size: 12
            text: "Chest"
            color: hifi.colors.lightGrayText
        }

        RalewayRegular {
            size: 12
            text: "requires hips"
            color: hifi.colors.lightGray
        }
     }


    Row {
        id: shoulderConfig
        anchors.top: chestConfig.bottom
        anchors.topMargin: 15
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 10
        spacing: 10

        HifiControls.CheckBox {
            id: shoulderBox
            width: 15
            height: 15
            boxRadius: 7

            onClicked: {
                if (checked) {
                    hipBox.checked = true;
                    feetBox.checked = true;
                    chestBox.checked = false;
                }
                sendConfigurationSettings();
            }
        }

        RalewayBold {
            size: 12
            text: "Shoulders"
            color: hifi.colors.lightGrayText
        }

        RalewayRegular {
            size: 12
            text: "requires hips"
            color: hifi.colors.lightGray
        }
    }

    Row {
        id: shoulderAdditionalConfig
        visible: shoulderBox.checked
        anchors.top: shoulderConfig.bottom
        anchors.topMargin: 5
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 20
        spacing: 10

        HifiControls.SpinBox {
            id: armCircumference
            decimals: 1
            width: 160
            suffix: " cm"
            label: "Arm Circumference"
            minimumValue: 0
            stepSize: 1.0
            colorScheme: hifi.colorSchemes.dark
            value: 33.0

            onEditingFinished: {
                sendConfigurationSettings();
            }
        }

        HifiControls.SpinBox {
            id: shoulderWidth
            width: 160
            label: "Shoulder Width"
            suffix: " cm"
            minimumValue: 0
            stepSize: 1.0
            decimals: 1
            colorScheme: hifi.colorSchemes.dark
            value: 48

            onEditingFinished: {
                sendConfigurationSettings();
            }
        }
    }

    Separator {
        id: bottomSeperator
        width: parent.width
        anchors.top: shoulderAdditionalConfig.visible ? shoulderAdditionalConfig.bottom : shoulderConfig.bottom
        anchors.topMargin: (shoulderAdditionalConfig.visible ? 25 : 10)
    }

    Rectangle {
        id: calibrationButton
        property int color: hifi.buttons.blue
        property int colorScheme: hifi.colorSchemes.light
        property string glyph: hifi.glyphs.avatar1
        property bool enabled: false
        property bool pressed: false
        property bool hovered: false
        property int size: 32
        property string text: "apply"
        property int padding: 12

        width: glyphButton.width + calibrationText.width + padding
        height: hifi.dimensions.controlLineHeight
        anchors.top: bottomSeperator.bottom
        anchors.topMargin: 15
        anchors.left: parent.left
        anchors.leftMargin: leftMargin

        radius: hifi.buttons.radius

        gradient: Gradient {
            GradientStop {
                position: 0.2
                color: {
                    if (!calibrationButton.enabled) {
                        hifi.buttons.disabledColorStart[calibrationButton.colorScheme]
                    } else if (calibrationButton.pressed) {
                        hifi.buttons.pressedColor[calibrationButton.color]
                    } else if (calibrationButton.hovered) {
                        hifi.buttons.hoveredColor[calibrationButton.color]
                    } else {
                        hifi.buttons.colorStart[calibrationButton.color]
                    }
                }
            }

            GradientStop {
                position: 1.0
                color: {
                    if (!calibrationButton.enabled) {
                        hifi.buttons.disabledColorFinish[calibrationButton.colorScheme]
                    } else if (calibrationButton.pressed) {
                        hifi.buttons.pressedColor[calibrationButton.color]
                    } else if (calibrationButton.hovered) {
                        hifi.buttons.hoveredColor[calibrationButton.color]
                    } else {
                        hifi.buttons.colorFinish[calibrationButton.color]
                    }
                }
            }
        }




        HiFiGlyphs {
            id: glyphButton
            color: enabled ? hifi.buttons.textColor[calibrationButton.color]
                : hifi.buttons.disabledTextColor[calibrationButton.colorScheme]
            text: calibrationButton.glyph
            size: calibrationButton.size

            anchors {
                top: parent.top
                bottom: parent.bottom
                bottomMargin: 1
            }
        }

        RalewayBold {
            id: calibrationText
            font.capitalization: Font.AllUppercase
            color: enabled ? hifi.buttons.textColor[calibrationButton.color]
                : hifi.buttons.disabledTextColor[calibrationButton.colorScheme]
            size: hifi.fontSizes.buttonLabel
            text: calibrationButton.text

            anchors {
                left: glyphButton.right
                top: parent.top
                topMargin: 7
            }
        }


        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                if (calibrationButton.enabled) {
                    if (openVrConfiguration.state === buttonState.apply) {
                        InputConfiguration.uncalibratePlugin(pluginName);
                        updateCalibrationButton();
                    } else {
                        calibrationTimer.interval = timeToCalibrate.value * 1000
                        openVrConfiguration.countDown = timeToCalibrate.value;
                        var calibratingScreen = screen.createObject();
                        stack.push(calibratingScreen);
                        calibratingScreen.canceled.connect(cancelCalibration);
                        calibratingScreen.restart.connect(restartCalibration);
                        calibratingScreen.start(calibrationTimer.interval, timeToCalibrate.value);
                        calibrationTimer.start();
                    }
                }
            }

            onPressed: {
                calibrationButton.pressed = true;
            }

            onReleased: {
                calibrationButton.pressed = false;
            }

            onEntered: {
                calibrationButton.hovered = true;
            }

            onExited: {
                calibrationButton.hovered = false;
            }
        }
    }

    Timer {
        id: calibrationTimer
        repeat: false
        interval: 20
        onTriggered: {
            InputConfiguration.calibratePlugin(pluginName)
        }
    }

    Timer {
        id: displayTimer
        repeat: false
        interval: 3000
        onTriggered: {
        }
    }

    Component.onCompleted: {
        InputConfiguration.calibrationStatus.connect(calibrationStatusInfo);
        lastConfiguration = composeConfigurationSettings();
    }

    Component.onDestruction: {
        var settings = InputConfiguration.configurationSettings(pluginName);
        var data = {
            "num_pucks": settings["puckCount"]
        }
        UserActivityLogger.logAction("mocap_ui_close_dialog", data);
    }

    HifiControls.SpinBox {
        id: timeToCalibrate
        width: 70
        anchors.top: calibrationButton.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: leftMargin

        minimumValue: 5
        value: 5
        colorScheme: hifi.colorSchemes.dark

        onEditingFinished: {
            calibrationTimer.interval = value * 1000;
            openVrConfiguration.countDown = value;
            numberAnimation.duration = calibrationTimer.interval;
        }
    }

    RalewayBold {
        id: delayTextInfo
        size: 10
        text: "Delay Before Calibration Starts"
        color: hifi.colors.white

        anchors {
            left: timeToCalibrate.right
            leftMargin: 20
            verticalCenter: timeToCalibrate.verticalCenter
        }
    }

    RalewayRegular {
        size: 12
        text: "sec"
        color: hifi.colors.lightGray

        anchors {
            left: delayTextInfo.right
            leftMargin: 10
            verticalCenter: delayTextInfo.verticalCenter
        }
    }

    Separator {
        id: advanceSeperator
        width: parent.width
        anchors.top: timeToCalibrate.bottom
        anchors.topMargin: 10
    }

    RalewayBold {
        id: advanceSettings
        
        text: "Advanced Settings"
        size: 12
        
        color: hifi.colors.white
        
        anchors.top: advanceSeperator.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: leftMargin
    }


    HifiControls.CheckBox {
        id: viveInDesktop
        width: 15
        height: 15
        boxRadius: 7

        anchors.top: advanceSettings.bottom
        anchors.topMargin: 5
        anchors.left: openVrConfiguration.left
        anchors.leftMargin: leftMargin + 10
        
        onClicked: {
            sendConfigurationSettings();
        }
    }

    RalewayBold {
        id: viveDesktopText
        size: 10
        text: "Use Vive devices in desktop mode"
        color: hifi.colors.white
        
        anchors {
            left: viveInDesktop.right
            leftMargin: 5
            verticalCenter: viveInDesktop.verticalCenter
        }
    }
    
    NumberAnimation {
        id: numberAnimation
        target: openVrConfiguration
        property: "countDown"
        to: 0
    }

    function logAction(action, status) {
        console.log("calibrated from ui");
        var data = {
            "num_pucks": status["puckCount"],
            "puck_configuration": status["configuration"],
            "head_puck": status["head_puck"],
            "hand_puck": status["hand_pucks"]
        }
        UserActivityLogger.logAction(action, data);
    }

    function calibrationStatusInfo(status) {
        var calibrationScreen = stack.currentItem;

        if (!status["UI"]) {
            calibratingScreen = screen.createObject();
            stack.push(calibratingScreen);
        }

        if (status["calibrated"]) {
            calibrationScreen.success();

            if (status["UI"]) {
                logAction("mocap_ui_success", status);
            }

        } else if (!status["calibrated"]) {
            calibrationScreen.failure();

            if (status["UI"]) {
                logAction("mocap_ui_failed", status);
            }
        }
        updateCalibrationButton();
    }


    function trackersForConfiguration() {
        var pucksNeeded = 0;

        if (headPuckBox.checked) {
            pucksNeeded++;
        }

        if (feetBox.checked) {
            pucksNeeded++;
        }

        if (hipBox.checked) {
            pucksNeeded++;
        }

        if (chestBox.checked) {
            pucksNeeded++;
        }

        if (shoulderBox.checked) {
            pucksNeeded++;
        }

        return pucksNeeded;
    }

    function cancelCalibration() {
        calibrationTimer.stop();
    }

    function restartCalibration() {
        calibrationTimer.restart();
    }

    function displayConfiguration() {
        var settings = InputConfiguration.configurationSettings(pluginName);
        var configurationType = settings["trackerConfiguration"];
        displayTrackerConfiguration(configurationType);


        var HmdHead = settings["HMDHead"];
        var viveController = settings["handController"];
        var desktopMode = settings["desktopMode"];

        armCircumference.value = settings.armCircumference;
        shoulderWidth.value = settings.shoulderWidth;

        if (HmdHead) {
            headBox.checked = true;
            headPuckBox.checked = false;
        } else {
            headPuckBox.checked = true;
            headBox.checked = false;
        }

        if (viveController) {
            handBox.checked = true;
            handPuckBox.checked = false;
        } else {
            handPuckBox.checked = true;
            handBox.checked = false;
        }

        viveInDesktop.checked = desktopMode;

        initializeButtonState();
        updateCalibrationText();

        var data = {
            "num_pucks": settings["puckCount"]
        };

        UserActivityLogger.logAction("mocap_ui_open_dialog", data);
    }

    function displayTrackerConfiguration(type) {
        if (type === "Feet") {
            feetBox.checked = true;
        } else if (type === "FeetAndHips") {
            feetBox.checked = true;
            hipBox.checked = true;
        } else if (type === "FeetHipsChest") {
            feetBox.checked = true;
            hipBox.checked = true;
            chestBox.checked = true;
        } else if (type === "FeetHipsAndShoulders") {
            feetBox.checked = true;
            hipBox.checked = true;
            shoulderBox.checked = true;
        } else if (type === "FeetHipsChestAndShoulders") {
            feetBox.checked = true;
            hipBox.checked = true;
            chestBox.checked = true;
            shoulderBox.checked = true;
        }
    }

    function updateButtonState() {
        var settings = composeConfigurationSettings();
        var bodySetting = settings["bodyConfiguration"];
        var headSetting = settings["headConfiguration"];
        var headOverride = headSetting["override"];
        var handSetting = settings["handConfiguration"];
        var handOverride = handSetting["override"];

        var settingsChanged = false;

        if (lastConfiguration["bodyConfiguration"] !== bodySetting) {
            settingsChanged = true;
        }

        var lastHead = lastConfiguration["headConfiguration"];
        if (lastHead["override"] !== headOverride) {
            settingsChanged = true;
        }

        var lastHand = lastConfiguration["handConfiguration"];
        if (lastHand["override"] !== handOverride) {
            settingsChanged = true;
        }

        if (settingsChanged) {
            if ((!handOverride) && (!headOverride) && (bodySetting === "None")) {
                state = buttonState.apply;
            } else {
                state = buttonState.applyAndCalibrate;
            }
        } else {
            if (state == buttonState.apply) {
                state = buttonState.disabled;
            } else if (state == buttonState.applyAndCalibrate) {
                state = buttonState.calibrate;
            }
        }

        lastConfiguration = settings;
    }

    function initializeButtonState() {
        var settings = composeConfigurationSettings();
        var bodySetting = settings["bodyConfiguration"];
        var headSetting = settings["headConfiguration"];
        var headOverride = headSetting["override"];
        var handSetting = settings["handConfiguration"];
        var handOverride = handSetting["override"];


        if ((!handOverride) && (!headOverride) && (bodySetting === "None")) {
            state = buttonState.disabled;
        } else {
            state = buttonState.calibrate;
        }
    }

    function updateCalibrationButton() {
        updateButtonState();
        updateCalibrationText();
    }

    function updateCalibrationText() {
        if (buttonState.disabled == state) {
            calibrationButton.enabled = false;
            calibrationButton.text = "Apply";
        } else if (buttonState.apply == state) {
            calibrationButton.enabled = true;
            calibrationButton.text = "Apply";
        } else if (buttonState.applyAndCalibrate == state) {
            calibrationButton.enabled = true;
            calibrationButton.text =  "Apply And Calibrate";
        } else if (buttonState.calibrate == state) {
            calibrationButton.enabled = true;
            calibrationButton.text = "Calibrate";
        }
    }

    function composeConfigurationSettings() {
        var trackerConfiguration = "";
        var overrideHead = false;
        var overrideHandController = false;

        if (shouldersChecked && chestChecked) {
            trackerConfiguration = "FeetHipsChestAndShoulders";
        } else if (shouldersChecked) {
            trackerConfiguration = "FeetHipsAndShoulders";
        } else if (chestChecked) {
            trackerConfiguration = "FeetHipsAndChest";
        } else if (hipsChecked) {
            trackerConfiguration = "FeetAndHips";
        } else if (feetChecked) {
            trackerConfiguration = "Feet";
        } else {
            trackerConfiguration = "None";
        }

        if (headPuck) {
            overrideHead = true;
        } else if (hmdHead) {
            overrideHead = false;
        }

        if (handController) {
            overrideHandController = false;
        } else if (handPuck) {
            overrideHandController = true;
        }

        var headObject = {
            "override": overrideHead,
            "Y": headYOffset.value,
            "Z": headZOffset.value
        }

        var handObject = {
            "override": overrideHandController,
            "Y": handYOffset.value,
            "Z": handZOffset.value
        }

        var settingsObject = {
            "bodyConfiguration": trackerConfiguration,
            "headConfiguration": headObject,
            "handConfiguration": handObject,
            "armCircumference": armCircumference.value,
            "shoulderWidth": shoulderWidth.value,
            "desktopMode": viveInDesktop.checked
        }

        return settingsObject;
    }

    function sendConfigurationSettings() {
        var settings = composeConfigurationSettings();
        InputConfiguration.setConfigurationSettings(settings, pluginName);
        updateCalibrationButton();
    }
}
