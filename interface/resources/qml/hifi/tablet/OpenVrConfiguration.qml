//
//  Created by Dante Ruiz on 6/5/17.
//  Copyright 2017 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtGraphicalEffects 1.0

import stylesUit 1.0
import "../../controls"
import controlsUit 1.0 as HifiControls
import "."


Flickable {
    id: flick
    width: parent.width
    height: parent.height
    anchors.fill: parent
    contentHeight: 550
    flickableDirection: Flickable.VerticalFlick
    property string pluginName: ""
    property var page: null;

    onPluginNameChanged: {
        if (page !== null) {
            page.pluginName = flick.pluginName;
            page.displayConfiguration();
        }
    }

    function bringToView(item) {
        var yTop = item.mapToItem(contentItem, 0, 0).y;
        var yBottom = yTop + item.height;

        var surfaceTop = contentY;
        var surfaceBottom = contentY + height;

        if(yTop < surfaceTop || yBottom > surfaceBottom) {
            contentY = yTop - height / 2 + item.height
        }
    }

    Component.onCompleted: {
        page = config.createObject(flick.contentItem);
    }
    Component {
        id: config
        Rectangle {
            id: openVrConfiguration
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
            readonly property bool hmdDesktop: hmdInDesktop.checked

            property int state: buttonState.disabled
            property var lastConfiguration:  null
            property bool isConfiguring: false

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
                    mouse.accepted = false;
                }
            }

            color: hifi.colors.baseGray

            RalewayBold {
                id: head

                text: "Head:"
                size: 12

                color: "white"
                anchors.left: openVrConfiguration.left
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
                            hmdInDesktop.checked = false;
                        } else {
                            checked = true;
                        }
                        sendConfigurationSettings();
                    }
                }

                RalewayBold {
                    size: 12
                    text: stack.selectedPlugin + " HMD"
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
                            hmdInDesktop.checked = false;
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

                HifiControls.CheckBox {
                    id: hmdInDesktop
                    width: 15
                    height: 15
                    boxRadius: 7
                    visible: viveInDesktop.checked

                    anchors.topMargin: 5
                    anchors.leftMargin: leftMargin + 10

                    onClicked: {
                        if (checked) {
                            headBox.checked = false;
                            headPuckBox.checked = false;
                        } else {
                            checked = true;
                        }
                        sendConfigurationSettings();
                    }
                }

                RalewayBold {
                    size: 12
                    visible: viveInDesktop.checked
                    text: "None"
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
                    minimumValue: -50
                    maximumValue: 50
                    realStepSize: 1
                    colorScheme: hifi.colorSchemes.dark

                    onRealValueChanged: {
                        sendConfigurationSettings();
                    }
                }


                HifiControls.SpinBox {
                    id: headZOffset
                    z: 10
                    width: 112
                    label: "Z Offset"
                    minimumValue: -50
                    maximumValue: 50
                    realStepSize: 1
                    decimals: 1
                    suffix: " cm"
                    colorScheme: hifi.colorSchemes.dark

                    onRealValueChanged: {
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
                anchors.left: openVrConfiguration.left
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
                    minimumValue: -30
                    maximumValue: 30
                    realStepSize: 1
                    colorScheme: hifi.colorSchemes.dark

                    onRealValueChanged: {
                        sendConfigurationSettings();
                    }
                }


                HifiControls.SpinBox {
                    id: handZOffset
                    width: 112
                    label: "Z Offset"
                    suffix: " cm"
                    minimumValue: -30
                    maximumValue: 30
                    realStepSize: 1
                    decimals: 1
                    colorScheme: hifi.colorSchemes.dark

                    onRealValueChanged: {
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

                text: "See Recommended Placement"
                color: hifi.colors.blueHighlight
                size: 12
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
                anchors.left: parent.left
                anchors.leftMargin: leftMargin + 10
                spacing: 10

                HifiControls.CheckBox {
                    id: feetBox
                    width: 15
                    height: 15

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
                    minimumValue: 10.0
                    maximumValue: 50.0
                    realStepSize: 1.0
                    colorScheme: hifi.colorSchemes.dark
                    realValue: 33.0

                    onRealValueChanged: {
                        sendConfigurationSettings();
                    }
                }

                HifiControls.SpinBox {
                    id: shoulderWidth
                    width: 160
                    label: "Shoulder Width"
                    suffix: " cm"
                    minimumValue: 25.0
                    maximumValue: 50.0
                    realStepSize: 1.0
                    decimals: 1
                    colorScheme: hifi.colorSchemes.dark
                    realValue: 48

                    onRealValueChanged: {
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
                                InputConfiguration.uncalibratePlugin(openVrConfiguration.pluginName);
                                updateCalibrationButton();
                            } else {
                                calibrationTimer.interval = timeToCalibrate.realValue * 1000
                                openVrConfiguration.countDown = timeToCalibrate.realValue;
                                var calibratingScreen = screen.createObject();
                                stack.push(calibratingScreen);
                                calibratingScreen.canceled.connect(cancelCalibration);
                                calibratingScreen.restart.connect(restartCalibration);
                                calibratingScreen.start(calibrationTimer.interval, timeToCalibrate.realValue);
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
                    InputConfiguration.calibratePlugin(openVrConfiguration.pluginName)
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
                lastConfiguration = composeConfigurationSettings();
                InputConfiguration.calibrationStatus.connect(calibrationStatusInfo);
            }

            Component.onDestruction: {
                var settings = InputConfiguration.configurationSettings(openVrConfiguration.pluginName);
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

                minimumValue: 0
                maximumValue: 5
                realValue: 5
                realStepSize: 1.0
                colorScheme: hifi.colorSchemes.dark

                onRealValueChanged: {
                    calibrationTimer.interval = realValue * 1000;
                    openVrConfiguration.countDown = realValue;
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

                anchors.top: advanceSettings.bottom
                anchors.topMargin: 5
                anchors.left: openVrConfiguration.left
                anchors.leftMargin: leftMargin + 10

                onClicked: {
                    if (!checked & hmdInDesktop.checked) {
                        headBox.checked = true;
                        headPuckBox.checked = false;
                        hmdInDesktop.checked = false;
                    }
                    sendConfigurationSettings();
                }
            }


            HifiControls.CheckBox {
                id: eyeTracking
                width: 15
                height: 15

                anchors.top: viveInDesktop.bottom
                anchors.topMargin: 5
                anchors.left: openVrConfiguration.left
                anchors.leftMargin: leftMargin + 10

                onClicked: {
                    sendConfigurationSettings();
                }
            }

            RalewayBold {
                id: eyeTrackingLabel
                size: 12
                text: "Use eye tracking (if available)."
                color: hifi.colors.lightGrayText
                anchors {
                    left: eyeTracking.right
                    leftMargin: 5
                    verticalCenter: eyeTracking.verticalCenter
                }
            }

            RalewayRegular {
                id: privacyPolicy
                text: "Privacy Policy"
                color: hifi.colors.blueHighlight
                size: 12
                anchors {
                    left: eyeTrackingLabel.right
                    leftMargin: 10
                    verticalCenter: eyeTrackingLabel.verticalCenter
                }

                Rectangle {
                    id: privacyPolicyUnderline
                    color: hifi.colors.blueHighlight
                    width: privacyPolicy.width
                    height: 1
                    anchors {
                        top: privacyPolicy.bottom
                        topMargin: 1
                        left: privacyPolicy.left
                    }
                    visible: false
                }

                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: true
                    onEntered: privacyPolicyUnderline.visible = true;
                    onExited: privacyPolicyUnderline.visible = false;
                    onClicked: About.openUrl("https://vircadia.com/privacy-policy");
                }
            }


            Row {
                id: outOfRangeDataStrategyRow
                anchors.top: eyeTracking.bottom
                anchors.topMargin: 5
                anchors.left: openVrConfiguration.left
                anchors.leftMargin: leftMargin + 10
                spacing: 15

                RalewayRegular {
                    id: outOfRangeDataStrategyLabel
                    size: 12
                    text: "Out Of Range Data Strategy:"
                    color: hifi.colors.lightGrayText
                    topPadding: 5
                }

                HifiControls.ComboBox {
                    id: outOfRangeDataStrategyComboBox

                    height: 25
                    width: 150

                    editable: true
                    colorScheme: hifi.colorSchemes.dark
                    model: ["None", "Freeze", "Drop", "DropAfterDelay"]
                    label: ""

                    onCurrentIndexChanged: {
                        sendConfigurationSettings();
                    }
                }
            }

            RalewayBold {
                id: viveDesktopText
                size: 12
                text: "Use " + stack.selectedPlugin + " devices in desktop mode"
                color: hifi.colors.lightGrayText

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
                isConfiguring = true;

                var settings = InputConfiguration.configurationSettings(openVrConfiguration.pluginName);
                var configurationType = settings["trackerConfiguration"];
                displayTrackerConfiguration(configurationType);

                // default offset for user wearing puck on the center of their forehead.
                headYOffset.realValue = 4; // (cm), puck is above the head joint.
                headZOffset.realValue = 8; // (cm), puck is in front of the head joint.

                // defaults for user wearing the pucks on the backs of their palms.
                handYOffset.realValue = 8; // (cm), puck is past the the hand joint.  (set this to zero if puck is on the wrist)
                handZOffset.realValue = -4; // (cm), puck is on above hand joint.

                var HmdHead = settings["HMDHead"];
                var viveController = settings["handController"];
                var desktopMode = settings["desktopMode"];
                var hmdDesktopPosition = settings["hmdDesktopTracking"];
                var eyeTrackingEnabled = settings["eyeTrackingEnabled"];

                armCircumference.realValue = settings.armCircumference;
                shoulderWidth.realValue = settings.shoulderWidth;

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
                hmdInDesktop.checked = hmdDesktopPosition;
                eyeTracking.checked = eyeTrackingEnabled;
                outOfRangeDataStrategyComboBox.currentIndex = outOfRangeDataStrategyComboBox.model.indexOf(settings.outOfRangeDataStrategy);

                initializeButtonState();
                updateCalibrationText();

                var data = {
                    "num_pucks": settings["puckCount"]
                };

                UserActivityLogger.logAction("mocap_ui_open_dialog", data);

                isConfiguring = false;
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
                if (lastConfiguration === null) {
                    lastConfiguration = composeConfigurationSettings();
                }
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
                    "Y": headYOffset.realValue,
                    "Z": headZOffset.realValue
                }

                var handObject = {
                    "override": overrideHandController,
                    "Y": handYOffset.realValue,
                    "Z": handZOffset.realValue
                }

                var settingsObject = {
                    "bodyConfiguration": trackerConfiguration,
                    "headConfiguration": headObject,
                    "handConfiguration": handObject,
                    "armCircumference": armCircumference.realValue,
                    "shoulderWidth": shoulderWidth.realValue,
                    "desktopMode": viveInDesktop.checked,
                    "hmdDesktopTracking": hmdInDesktop.checked,
                    "eyeTrackingEnabled": eyeTracking.checked,
                    "outOfRangeDataStrategy": outOfRangeDataStrategyComboBox.model[outOfRangeDataStrategyComboBox.currentIndex]
                }

                return settingsObject;
            }

            function sendConfigurationSettings() {
                if (isConfiguring) {
                    // Ignore control value changes during dialog initialization.
                    return;
                }
                var settings = composeConfigurationSettings();
                InputConfiguration.setConfigurationSettings(settings, openVrConfiguration.pluginName);
                updateCalibrationButton();
            }
        }
    }
}
