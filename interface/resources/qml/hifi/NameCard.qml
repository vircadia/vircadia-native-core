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

import Hifi 1.0 as Hifi
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
    anchors.top: parent.top
    anchors {
        topMargin: (parent.height - contentHeight)/2
        bottomMargin: (parent.height - contentHeight)/2
        leftMargin: 10
        rightMargin: 10
    }

    // Properties
    property int contentHeight: 70
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
        height: contentHeight
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
        width: parent.width - /*avatarImage.width - */parent.anchors.leftMargin - parent.anchors.rightMargin - parent.spacing
        height: contentHeight

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
            height: 4
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
                    GradientStop { position: 0.05; color: "#00CFEF" }
                    GradientStop { position: 0.5; color: "#9450A5" }
                    GradientStop { position: 0.95; color: "#EA4C5F" }
                }
            }
        }

        // Per-Avatar Gain Slider Spacer
        Item {
            width: parent.width
            height: 4
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
            updateValueWhileDragging: false
            onValueChanged: updateGainFromQML(uuid, value)
            style: SliderStyle {
                groove: Rectangle {
                    color: "#dbdbdb"
                    implicitWidth: gainSlider.width
                    implicitHeight: 4
                    radius: 2
                    MouseArea {
                        anchors.fill: parent
                        onDoubleClicked: {
                            gainSlider.value = 0.0
                        }
                    }
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
