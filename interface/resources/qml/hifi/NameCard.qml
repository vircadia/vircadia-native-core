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

Item {
    id: thisNameCard
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
    property bool selected: false

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
    Item {
        id: textContainer
        // Size
        width: parent.width - /*avatarImage.width - parent.spacing - */parent.anchors.leftMargin - parent.anchors.rightMargin
        height: childrenRect.height
        anchors.verticalCenter: parent.verticalCenter
        // DisplayName Text
        FiraSansSemiBold {
            id: displayNameText
            // Properties
            text: thisNameCard.displayName
            elide: Text.ElideRight
            // Size
            width: parent.width
            // Anchors
            anchors.top: parent.top
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
            // Anchors
            anchors.top: displayNameText.bottom
            // Text Size
            size: thisNameCard.usernameTextHeight
            // Text Positioning
            verticalAlignment: Text.AlignVCenter
            // Style
            color: hifi.colors.baseGray
        }

        // Spacer
        Item {
            id: spacer
            height: 4
            width: parent.width
            // Anchors
            anchors.top: userNameText.bottom
        }

        // VU Meter
        Rectangle {
            id: nameCardVUMeter
            // Size
            width: ((gainSlider.value - gainSlider.minimumValue)/(gainSlider.maximumValue - gainSlider.minimumValue)) * parent.width
            height: 8
            // Anchors
            anchors.top: spacer.bottom
            // Style
            radius: 4
            color: "#c5c5c5"
            // Rectangle for the zero-gain point on the VU meter
            Rectangle {
                id: vuMeterZeroGain
                visible: gainSlider.visible
                // Size
                width: 4
                height: 18
                // Style
                color: hifi.colors.darkGray
                // Anchors
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: (-gainSlider.minimumValue)/(gainSlider.maximumValue - gainSlider.minimumValue) * gainSlider.width - 4
            }
            // Rectangle for the VU meter line
            Rectangle {
                id: vuMeterLine
                width: gainSlider.width
                visible: gainSlider.visible
                // Style
                color: vuMeterBase.color
                radius: nameCardVUMeter.radius
                height: nameCardVUMeter.height / 2
                anchors.verticalCenter: nameCardVUMeter.verticalCenter
            }
            // Rectangle for the VU meter base
            Rectangle {
                id: vuMeterBase
                // Anchors
                anchors.fill: parent
                // Style
                color: parent.color
                radius: parent.radius
            }
            // Rectangle for the VU meter audio level
            Rectangle {
                id: vuMeterLevel
                // Size
                width: (thisNameCard.audioLevel) * parent.width
                // Style
                color: parent.color
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

        // Per-Avatar Gain Slider 
        Slider {
            id: gainSlider
            // Size
            width: parent.width
            height: 14
            // Anchors
            anchors.verticalCenter: nameCardVUMeter.verticalCenter
            // Properties
            visible: !isMyCard && selected
            value: pal.gainSliderValueDB[uuid] ? pal.gainSliderValueDB[uuid] : 0.0
            minimumValue: -60.0
            maximumValue: 20.0
            stepSize: 5
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
                    color: "#c5c5c5"
                    implicitWidth: gainSlider.width
                    implicitHeight: 4
                    radius: 2
                    opacity: 0
                }
                handle: Rectangle {
                    anchors.centerIn: parent
                    color: (control.pressed || control.hovered) ? "#00b4ef" : "#8F8F8F"
                    implicitWidth: 10
                    implicitHeight: 16
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
