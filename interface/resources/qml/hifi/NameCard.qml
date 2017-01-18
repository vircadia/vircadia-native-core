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
    property real displayNameTextPixelSize: 18
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

        // DisplayName field for my card
        Rectangle {
            id: myDisplayName
            visible: isMyCard
            // Size
            width: parent.width + 70
            height: 35
            // Anchors
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.leftMargin: -10
            // Style
            color: hifi.colors.textFieldLightBackground
            border.color: hifi.colors.blueHighlight
            border.width: 0
            TextInput {
                id: myDisplayNameText
                // Properties
                text: thisNameCard.displayName
                maximumLength: 256
                clip: true
                // Size
                width: parent.width
                height: parent.height
                // Anchors
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: parent.right
                anchors.rightMargin: editGlyph.width + editGlyph.anchors.rightMargin
                // Style
                color: hifi.colors.darkGray
                FontLoader { id: firaSansSemiBold; source: "../../fonts/FiraSans-SemiBold.ttf"; }
                font.family: firaSansSemiBold.name
                font.pixelSize: displayNameTextPixelSize
                selectionColor: hifi.colors.blueHighlight
                selectedTextColor: "black"
                // Text Positioning
                verticalAlignment: TextInput.AlignVCenter
                horizontalAlignment: TextInput.AlignLeft
                // Signals
                onEditingFinished: {
                    pal.sendToScript({method: 'displayNameUpdate', params: text})
                    cursorPosition = 0
                    focus = false
                    myDisplayName.border.width = 0
                    color = hifi.colors.darkGray
                }
            }
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                onClicked: {
                    myDisplayName.border.width = 1
                    myDisplayNameText.focus ? myDisplayNameText.cursorPosition = myDisplayNameText.positionAt(mouseX, mouseY, TextInput.CursorOnCharacter) : myDisplayNameText.selectAll();
                    myDisplayNameText.focus = true
                    myDisplayNameText.color = "black"
                }
                onDoubleClicked: {
                    myDisplayNameText.selectAll();
                    myDisplayNameText.focus = true;
                }
                onEntered: myDisplayName.color = hifi.colors.lightGrayText
                onExited: myDisplayName.color = hifi.colors.textFieldLightBackground
            }
            // Edit pencil glyph
            HiFiGlyphs {
                id: editGlyph
                text: hifi.glyphs.editPencil
                // Text Size
                size: displayNameTextPixelSize*1.5
                // Anchors
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                // Style
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: hifi.colors.baseGray
            }
        }
        // Spacer for DisplayName for my card
        Rectangle {
            id: myDisplayNameSpacer
            width: myDisplayName.width
            // Anchors
            anchors.top: myDisplayName.bottom
            height: 5
            visible: isMyCard
            opacity: 0
        }
        // DisplayName Text for others' cards
        FiraSansSemiBold {
            id: displayNameText
            // Properties
            text: thisNameCard.displayName
            elide: Text.ElideRight
            visible: !isMyCard
            // Size
            width: parent.width
            // Anchors
            anchors.top: parent.top
            // Text Size
            size: displayNameTextPixelSize
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
            anchors.top: isMyCard ? myDisplayNameSpacer.bottom : displayNameText.bottom
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
            width: isMyCard ? myDisplayName.width - 20 : ((gainSlider.value - gainSlider.minimumValue)/(gainSlider.maximumValue - gainSlider.minimumValue)) * parent.width
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
