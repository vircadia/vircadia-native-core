//
//  Created by Dante Ruiz on 6/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


import QtQuick 2.5

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import QtQuick.Controls.Styles 1.4
import "../../styles-uit"
import "../../controls"
import "../../controls-uit" as HifiControls


Rectangle {
    id: info


    signal canceled()
    signal restart()

    property int count: 5
    property string calibratingText: "CALIBRATING..."
    property string calibratingCountText: "CALIBRATION STARTING IN"
    property string calibrationSuccess: "CALIBRATION COMPLETED"
    property string calibrationFailed: "CALIBRATION FAILED"
    property string instructionText: "Please stand in a T-Pose during calibration"
    
    HifiConstants { id: hifi }
    visible: true
    color: hifi.colors.baseGray

    property string whiteIndicator: "../../../images/loader-calibrate-white.png"
    property string blueIndicator: "../../../images/loader-calibrate-blue.png"
    
    Image {
        id: busyIndicator
        width: 350
        height: 350

        property bool running: true

        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: 60
        }
        visible: busyIndicator.running
        source: blueIndicator
        NumberAnimation on rotation {
            id: busyRotation
            running: busyIndicator.running
            loops: Animation.Infinite
            duration: 1000
            from: 0 ; to: 360
        }
    }


    HiFiGlyphs {
        id: image
        text: hifi.glyphs.avatarTPose
        size: 190
        color: hifi.colors.white

        anchors {
            top: busyIndicator.top
            topMargin: 40
            horizontalCenter: busyIndicator.horizontalCenter
        }
    }

    RalewayBold {
        id: statusText
        text: info.calibratingCountText
        size: 16
        color: hifi.colors.blueHighlight

        anchors {
            top: image.bottom
            topMargin: 15
            horizontalCenter: image.horizontalCenter
        }
    }


    RalewayBold {
        id: countDown
        text: info.count
        color: hifi.colors.blueHighlight

        anchors {
            top: statusText.bottom
            topMargin: 12
            horizontalCenter: statusText.horizontalCenter
        }
    }
    

    RalewayBold {
        id: directions

        anchors {
            top: busyIndicator.bottom
            topMargin: 100
            horizontalCenter: parent.horizontalCenter
        }

        color: hifi.colors.white
        size: hifi.fontSizes.rootMenuDisclosure
        text: "Please stand in a T-Pose during calibration"
    }

    NumberAnimation {
        id: numberAnimation
        target: info
        property: "count"
        to: 0
    }

    Timer {
        id: timer

        repeat: false
        interval: 3000
        onTriggered: {
            info.calibrating();
        }
    }

    Timer {
        id: closeWindow
        repeat: false
        interval: 3000
        onTriggered: stack.pop();
    }

    Row {

        spacing: 20
        anchors {
            top: directions.bottom
            topMargin: 30
            horizontalCenter: parent.horizontalCenter
        }


        HifiControls.Button {
            width: 120
            height: 30
            color: hifi.buttons.red
            text: "RESTART"

            onClicked: {
                restart();
                statusText.color = hifi.colors.blueHighlight;
                statusText.text = info.calibratingCountText;
                directions.text = instructionText;
                countDown.visible = true;
                busyIndicator.running = true;
                busyRotation.from = 0
                busyRotation.to = 360
                busyIndicator.source = blueIndicator;
                closeWindow.stop();
                numberAnimation.stop();
                info.count = (timer.interval / 1000);
                numberAnimation.start();
                timer.restart();
            }
        }

        HifiControls.Button {
            width: 120
            height: 30
            color: hifi.buttons.black
            text: "CANCEL"

            onClicked: {
                canceled();
                stack.pop()
            }
        }
    }
    
    
    function start(interval, countNumber) {
        countDown.visible = true;
        statusText.color = hifi.colors.blueHighlight;
        numberAnimation.duration = interval
        info.count = countNumber;
        timer.interval = interval;
        numberAnimation.start();
        timer.start();
    }

    function calibrating() {
        countDown.visible = false;
        busyIndicator.source = whiteIndicator;
        busyRotation.from = 360
        busyRotation.to = 0
        statusText.text = info.calibratingText;
        statusText.color = hifi.colors.white
    }

    function success() {
        busyIndicator.running = false;
        statusText.text = info.calibrationSuccess
        statusText.color = hifi.colors.greenHighlight
        directions.text = "SUCCESS"
        closeWindow.start();
    }

    function failure() {
        busyIndicator.running = false;
        statusText.text = info.calibrationFailed
        statusText.color = hifi.colors.redHighlight
        closeWindow.start();
    }
}
