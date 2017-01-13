//
//  NameCard.qml
//  qml/hifi
//
//  Created by Howard Stearns on 12/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0
import "../styles-uit"

Row {
    id: thisNameCard
    // Spacing
    spacing: 10
    // Anchors
    anchors {
        verticalCenter: parent.verticalCenter
        leftMargin: 10
        rightMargin: 10
    }

    // Properties
    property string uuid: ""
    property string displayName: ""
    property string userName: ""
    property int displayTextHeight: 18
    property int usernameTextHeight: 12
    property real audioLevel: 0.0
    property bool isMyCard: false

    /* User image commented out for now - will probably be re-introduced later.
    Column {
        id: avatarImage
        // Size
        height: parent.height
        width: height
        Image {
            id: userImage
            source: "../../icons/defaultNameCardUser.png"
            // Anchors
            width: parent.width
            height: parent.height
        }
    }
    */
    Column {
        id: textContainer
        // Size
        width: parent.width - /*avatarImage.width - parent.spacing - */parent.anchors.leftMargin - parent.anchors.rightMargin
        anchors.verticalCenter: parent.verticalCenter
        // DisplayName Text
        FiraSansSemiBold {
            id: displayNameText
            // Properties
            text: thisNameCard.displayName
            elide: Text.ElideRight
            // Size
            width: parent.width
            // Text Size
            size: thisNameCard.displayTextHeight
            // Text Positioning
            verticalAlignment: Text.AlignVCenter
            // Style
            color: hifi.colors.darkGray
        }

        // UserName Text
        FiraSansRegular {
            id: userNameText
            // Properties
            text: thisNameCard.userName
            elide: Text.ElideRight
            visible: thisNameCard.displayName
            // Size
            width: parent.width
            // Text Size
            size: thisNameCard.usernameTextHeight
            // Text Positioning
            verticalAlignment: Text.AlignVCenter
            // Style
            color: hifi.colors.baseGray
        }

        // Spacer
        Item {
            height: 3
            width: parent.width
        }

        // VU Meter
        Rectangle { // CHANGEME to the appropriate type!
            id: nameCardVUMeter
            // Size
            width: parent.width
            height: 8
            // Style
            radius: 4
            // Rectangle for the VU meter base
            Rectangle {
                id: vuMeterBase
                // Anchors
                anchors.fill: parent
                // Style
                color: "#c5c5c5"
                radius: parent.radius
            }
            // Rectangle for the VU meter audio level
            Rectangle {
                id: vuMeterLevel
                // Size
                width: (thisNameCard.audioLevel) * parent.width
                // Style
                color: "#c5c5c5"
                radius: parent.radius
                // Anchors
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                anchors.left: parent.left
            }
            // Gradient for the VU meter audio level
            LinearGradient {
                anchors.fill: vuMeterLevel
                source: vuMeterLevel
                start: Qt.point(0, 0)
                end: Qt.point(parent.width, 0)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#2c8e72" }
                    GradientStop { position: 0.9; color: "#1fc6a6" }
                    GradientStop { position: 0.91; color: "#ea4c5f" }
                    GradientStop { position: 1.0; color: "#ea4c5f" }
                }
            }
        }

        // Per-Avatar Gain Slider Spacer
        Item {
            width: parent.width
            height: 3
            visible: !isMyCard
        }
        // Per-Avatar Gain Slider 
        Slider {
            id: gainSlider
            visible: !isMyCard
            width: parent.width
            height: 18
            value: pal.gainSliderValueDB[uuid] ? pal.gainSliderValueDB[uuid] : 0.0
            minimumValue: -60.0
            maximumValue: 20.0
            stepSize: 2
            updateValueWhileDragging: true
            onValueChanged: updateGainFromQML(uuid, value)
            MouseArea {
                anchors.fill: parent
                onWheel: {
                    // Do nothing.
                }
                onDoubleClicked: {
                    gainSlider.value = 0.0
                }
                onPressed: {
                    // Pass through to Slider
                    mouse.accepted = false
                }
                onReleased: {
                    // Pass through to Slider
                    mouse.accepted = false
                }
            }
            style: SliderStyle {
                groove: Rectangle {
                    color: "#dbdbdb"
                    implicitWidth: gainSlider.width
                    implicitHeight: 4
                    radius: 2
                }
                handle: Rectangle {
                    anchors.centerIn: parent
                    color: (control.pressed || control.hovered) ? "#00b4ef" : "#8F8F8F"
                    implicitWidth: 10
                    implicitHeight: 18
                }
            }
        }
    }

    function updateGainFromQML(avatarUuid, sliderValue) {
        if (pal.gainSliderValueDB[avatarUuid] !== sliderValue) {
            pal.gainSliderValueDB[avatarUuid] = sliderValue;
            var data = {
                sessionId: avatarUuid,
                gain: sliderValue
            };
            pal.sendToScript({method: 'updateGain', params: data});
        }
    }
}
