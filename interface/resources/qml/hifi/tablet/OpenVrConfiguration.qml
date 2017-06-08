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
    property string pluginName: ""

    readonly property bool feetChecked: feetBox.checked
    readonly property bool hipsChecked: hipBox.checked
    readonly property bool chestChecked: chestBox.checked
    readonly property bool shouldersChecked: shoulderBox.checked
    readonly property bool hmdHead: headBox.checked
    readonly property bool headPuck: headPuckBox.checked
    readonly property bool handController: handBox.checked
    readonly property bool handPuck: handPuckBox.checked

    HifiConstants { id: hifi }

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
        anchors.leftMargin: leftMargin + 20
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
                composeConfigurationSettings();
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
                composeConfigurationSettings();
            }
        }

        RalewayBold {
            size: 12
            text: "Tracker"
            color: hifi.colors.lightGrayText
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
        anchors.leftMargin: leftMargin + 20
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
                composeConfigurationSettings();
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
                composeConfigurationSettings();
            }
        }
        
        RalewayBold {
            size: 12
            text: "Trackers"
            color: hifi.colors.lightGrayText
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
        anchors.leftMargin: leftMargin + 20
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
                composeConfigurationSettings();
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
        anchors.leftMargin: leftMargin + 20
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
                composeConfigurationSettings();
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
        anchors.leftMargin: leftMargin + 20
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
                composeConfigurationSettings();
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
        anchors.leftMargin: leftMargin + 20
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
                composeConfigurationSettings();
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


    Rectangle {
        id: calibrationButton
        width: 200
        height: 35
        radius: 6

        color: hifi.colors.blueHighlight

        anchors.top: bottomSeperator.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: leftMargin


        HiFiGlyphs {
            id: calibrationGlyph
            text: hifi.glyphs.sliders
            size: 36
            color: hifi.colors.white

            anchors.horizontalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 30
            
        }
        
        RalewayRegular {
            id: calibrate
            text: "CALIBRATE"
            size: 17
            color: hifi.colors.white

            anchors.left: calibrationGlyph.right
            anchors.top: parent.top
            anchors.topMargin: 8
        }

        MouseArea {
            anchors.fill: parent

            onClicked: {
                InputCalibration.calibratePlugin(pluginName);
            }
        }
    }

    HifiControls.SpinBox {
        id: timeToCalibrate

        anchors.top: calibrationButton.bottom
        anchors.topMargin: 40
        anchors.left: parent.left
        anchors.leftMargin: leftMargin

        label: "Time til calibration ( in seconds )"
        colorScheme: hifi.colorSchemes.dark
    }

    Component.onCompleted: {
        var settings = InputConfiguration.configurationSettings(pluginName);

        var configurationType = settings["trackerConfiguration"];
        displayTrackerConfiguration(configurationType);

        
        var HmdHead = settings["HMDHead"];
        var viveController = settings["handController"];

        console.log(HmdHead);
        console.log(viveController);
        if (HmdHead) {
            headBox.checked = true;
        } else {
            headPuckBox.checked = true;
        }

        if (viveController) {
            handBox.checked = true;
        } else {
            handPuckBox.checked = true;
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

    
    function composeConfigurationSettings() {
        var trackerConfiguration = "";
        var overrideHead = false;
        var overrideHandController = false;

        if (shouldersChecked && chestChecked) {
            trackerConfiguration = "FeetHipsChestAndShoulders";
        } else if (shouldersChecked) {
            trackerConfiguration = "FeetHipsAndShoulders";
        } else if (chestChecked) {
            trackerConfiguration = "FeetHipsChest";
        } else if (hipsChecked) {
            trackerConfiguration = "FeetAndHips";
        } else if (feetChecked) {
            trackerConfiguration = "Feet";
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
            
        
        var settingsObject = {
            "trackerConfiguration": trackerConfiguration,
            "overrideHead": overrideHead,
            "overrideHandController": overrideHandController
        }

        InputConfiguration.setConfigurationSettings(settingsObject, pluginName);
        
    }
}
