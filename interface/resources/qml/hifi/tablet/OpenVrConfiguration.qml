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
import "../../styles-uit"
import "../../controls"
import "../../controls-uit" as HifiControls


Rectangle {
    id: openVrConfiguration

    width: parent.width
    height: parent.height
    anchors.fill: parent

    property int leftMargin: 75
    property int countDown: 0
    property string pluginName: ""
    property var displayInformation: openVrConfiguration.displayConfiguration()

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

        HifiControls.SpinBox {
            id: headYOffset
            decimals: 4
            width: 110
            label: "Y: offset"
            value:  -0.0254
            stepSize: 0.0254
            colorScheme: hifi.colorSchemes.dark

            onEditingFinished: {
            }
        }


        HifiControls.SpinBox {
            id: headZOffset
            width: 105
            label: "Z: offset"
            value: -0.152
            stepSize: 0.0254
            decimals: 4
            colorScheme: hifi.colorSchemes.dark
            
            onEditingFinished: {
            }
        }
    }
    
    RalewayBold {
        id: hands

        text: "Hands:"
        size: 12

        color: "white"

        anchors.top: headConfig.bottom
        anchors.topMargin: 10
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

        HifiControls.SpinBox {
            id: handYOffset
            decimals: 4
            width: 105
            label: "Y: offset"
            value: -0.0508
            stepSize: 0.0254
            colorScheme: hifi.colorSchemes.dark

            onEditingFinished: {
            }
        }


        HifiControls.SpinBox {
            id: handZOffset
            width: 105
            label: "Z: offset"
            value: -0.0254
            stepSize: 0.0254
            decimals: 4
            colorScheme: hifi.colorSchemes.dark
            
            onEditingFinished: {
            }
        }
    }

    RalewayBold {
        id: additional

        text: "Additional Trackers"
        size: 12

        color: hifi.colors.white

        anchors.top: handConfig.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: leftMargin
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
                if (hipsChecked) {
                    checked = true;
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

    Separator {
        id: bottomSeperator
        width: parent.width
        anchors.top: shoulderConfig.bottom
        anchors.topMargin: 10
    }


    HifiControls.Button {
        
        id: calibrationButton
        color: hifi.buttons.blue
        text: "Calibrate"
        //glyph: hifi.glyphs.avatar1
        anchors.top: bottomSeperator.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: leftMargin
        
        onClicked: {
            openVrConfiguration.countDown = timeToCalibrate.value;
            numberAnimation.start();
            calibrationTimer.start();
            info.visible = true;
            info.showCountDown = true;
        }
    }

    Timer {
        id: calibrationTimer
        repeat: false
        interval: 20
        onTriggered: {
            InputConfiguration.calibratePlugin(pluginName)
            info.visible = false;
            info.showCountDown = false;
        }
    }

    Timer {
        id: displayTimer
        repeat: false
        interval: 3000
        onTriggered: {
            info.showCountDown = false;
            info.calibrationFailed = false
            info.calibrationSucceed = false;
            info.calibrationSucceed = false;
            info.showCalibrationStatus = false;
            info.visible = false;
        }
    }

    Component.onCompleted: {
        InputConfiguration.calibrationStatus.connect(calibrationStatusInfo);
        lastConfiguration = composeConfigurationSettings();
    }

    HifiControls.SpinBox {
        id: timeToCalibrate
        width: 70
        anchors.top: calibrationButton.bottom
        anchors.topMargin: 40
        anchors.left: parent.left
        anchors.leftMargin: leftMargin

        minimumValue: 3
        label: "Time til calibration ( in seconds )"
        colorScheme: hifi.colorSchemes.dark

        onEditingFinished: {
            calibrationTimer.interval = value * 1000;
            openVrConfiguration.countDown = value;
            numberAnimation.duration = calibrationTimer.interval;
        }
    }

    NumberAnimation {
        id: numberAnimation
        target: openVrConfiguration
        property: "countDown"
        to: 0
    }

    function calibrationStatusInfo(status) {
        if (status["calibrated"]) {
            info.visible = true;
            info.showCalibrationStatus = true;
            info.calibrationSucceed = true;
        } else if (!status["calibrated"]) {
            var uncalibrated = status["success"];
            if (uncalibrated) {
                info.visible = true;
                info.showCalibrationStatus = true;
                info.uncalibrationSucceed = true;
            } else {
                info.visible = true;
                info.showCalibrationStatus = true;
                info.calibrationFailed = true;
            }
        }
        displayTimer.start();
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

    function displayConfiguration() {
        var settings = InputConfiguration.configurationSettings(pluginName);
        var configurationType = settings["trackerConfiguration"];
        displayTrackerConfiguration(configurationType);


        var HmdHead = settings["HMDHead"];
        var viveController = settings["handController"];

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
    }


    Rectangle {
        id: info
        property bool showCountDown: false
        property bool showCalibrationStatus: false
        property bool calibrationFailed: false
        property bool calibrationSucceed: false
        property bool uncalibrationSucceed: false

        visible: false
        color: hifi.colors.baseGray
        anchors.top: openVrConfiguration.top
        anchors.bottom: bottomSeperator.bottom
        anchors.left: openVrConfiguration.left
        anchors.right: openVrConfiguration.right

        Item {
            id: countDownContainer
            visible: info.showCountDown
            anchors.centerIn: parent
            RalewayBold {
                id: countDownText

                text: openVrConfiguration.countDown
                size: 92

                color: hifi.colors.blueHighlight

                anchors.centerIn: parent
            }
        }

        Item {
            id: calibrationStatus
            visible: info.showCalibrationStatus
            anchors.centerIn: parent
            Item {
                id: successInfo
                visible: info.calibrationSucceed
                anchors.centerIn: parent
                RalewayBold {
                    id: successText
                    text: "Calibration Successful"
                    size: 42
                    color: hifi.colors.greenHighlight
                    anchors.centerIn: parent
                }
            }

            Item {
                id: failedInfo
                visible: info.calibrationFailed
                anchors.fill: parent
                RalewayBold {
                    id: failedText
                    text: "Calibration Failed"
                    size: 42
                    color: hifi.colors.redAccent
                    anchors.centerIn: parent
                }
            }

            Item {
                id: uncalibrateInfo
                visible: info.uncalibrationSucceed
                anchors.centerIn: parent
                RalewayBold {
                    id: uncalibrateText
                    text: "Uncalibration Successful"
                    size: 37
                    color: hifi.colors.greenHighlight
                    anchors.centerIn: parent
                }
            }
        }
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
        var handOverride = settings["override"];

        var settingsChanged = false;

        if (lastConfiguration["bodyConfiguration"]["override"] !== bodySetting["override"]) {
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
            if ((!handOverride) && (!headOverride) && (bodySetting === "Auto")) {
                state = buttonState.apply;
            } else {
                state = buttonState.applyAndCalibrate;
            }   
        } else {
            if (state == buttonState.apply) {
                state = buttonState.disabled;
            }
        }
        
        lastConfiguration = settings;
    }

    function updateCalibrationText() {
        updateButtonState();
        if (buttonState.disabled == state) {
            calibrationButton.text = "Apply";
        } else if (buttonState.apply == state) {
            calibrationButton.text = "Apply";
        } else if (buttonState.applyAndCalibrate == state) {
            calibrationButton.text =  "Apply And Calibrate";
        } else if (buttonState.calibrate == state) {
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
            "handConfiguration": handObject 
        }

        return settingsObject;
    }

    function sendConfigurationSettings() {
        var settings = composeConfigurationSettings();
        InputConfiguration.setConfigurationSettings(settings, pluginName);
        //updateCalibrationText();
    }
}
