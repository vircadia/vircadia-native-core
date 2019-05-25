//
//  TTS.qml
//
//  TTS App
//
//  Created by Zach Fox on 2018-10-10
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.10
import QtQuick.Controls 2.3
import "qrc:////qml//styles-uit" as HifiStylesUit
import "qrc:////qml//controls-uit" as HifiControlsUit
import "qrc:////qml//controls" as HifiControls

Rectangle {
    HifiStylesUit.HifiConstants { id: hifi; }

    id: root;
    // Style
    color: hifi.colors.darkGray;
    property bool keyboardRaised: false;

    //
    // TITLE BAR START
    //
    Item {
        id: titleBarContainer;
        // Size
        width: root.width;
        height: 50;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;

        // Title bar text
        HifiStylesUit.RalewaySemiBold {
            id: titleBarText;
            text: "Text-to-Speech";
            // Text size
            size: hifi.fontSizes.overlayTitle;
            // Anchors
            anchors.top: parent.top;
			anchors.bottom: parent.bottom;
			anchors.left: parent.left;
            anchors.leftMargin: 16;
			width: paintedWidth;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        // Separator
        HifiControlsUit.Separator {
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
        }
    }
    //
    // TITLE BAR END
    //
    

    Item {
        id: tagButtonContainer;
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: 2;
        anchors.left: parent.left;
        anchors.right: parent.right;
        height: 70;

        HifiStylesUit.RalewaySemiBold {
            id: tagButtonTitle;
            text: "Insert Tag:";
            // Text size
            size: 18;
            // Anchors
            anchors.top: parent.top;
			anchors.left: parent.left;
            anchors.right: parent.right;
            height: 35;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }

		HifiControlsUit.Button {
			id: pitch10Button;
            focusPolicy: Qt.NoFocus;
			color: hifi.buttons.none;
			colorScheme: hifi.colorSchemes.dark;
            anchors.top: tagButtonTitle.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 3;
            width: parent.width/6 - 6;
			height: 30;
			text: "Pitch 10";
			onClicked: {
                messageToSpeak.insert(messageToSpeak.cursorPosition, "<pitch absmiddle='10'/>");
			}
		}

		HifiControlsUit.Button {
			id: pitch0Button;
            focusPolicy: Qt.NoFocus;
			color: hifi.buttons.none;
			colorScheme: hifi.colorSchemes.dark;
            anchors.top: tagButtonTitle.bottom;
            anchors.left: pitch10Button.right;
            anchors.leftMargin: 6;
            width: parent.width/6 - anchors.leftMargin;
			height: 30;
			text: "Pitch 0";
			onClicked: {
                messageToSpeak.insert(messageToSpeak.cursorPosition, "<pitch absmiddle='0'/>");
			}
		}

		HifiControlsUit.Button {
			id: pitchNeg10Button;
            focusPolicy: Qt.NoFocus;
			color: hifi.buttons.none;
			colorScheme: hifi.colorSchemes.dark;
            anchors.top: tagButtonTitle.bottom;
            anchors.left: pitch0Button.right;
            anchors.leftMargin: 6;
            width: parent.width/6 - anchors.leftMargin;
			height: 30;
			text: "Pitch -10";
			onClicked: {
                messageToSpeak.insert(messageToSpeak.cursorPosition, "<pitch absmiddle='-10'/>");
			}
		}

		HifiControlsUit.Button {
			id: speed5Button;
            focusPolicy: Qt.NoFocus;
			color: hifi.buttons.none;
			colorScheme: hifi.colorSchemes.dark;
            anchors.top: tagButtonTitle.bottom;
            anchors.left: pitchNeg10Button.right;
            anchors.leftMargin: 6;
            width: parent.width/6 - anchors.leftMargin;
			height: 30;
			text: "Speed 5";
			onClicked: {
                messageToSpeak.insert(messageToSpeak.cursorPosition, "<rate absspeed='5'/>");
			}
		}

		HifiControlsUit.Button {
			id: speed0Button;
            focusPolicy: Qt.NoFocus;
			color: hifi.buttons.none;
			colorScheme: hifi.colorSchemes.dark;
            anchors.top: tagButtonTitle.bottom;
            anchors.left: speed5Button.right;
            anchors.leftMargin: 6;
            width: parent.width/6 - anchors.leftMargin;
			height: 30;
			text: "Speed 0";
			onClicked: {
                messageToSpeak.insert(messageToSpeak.cursorPosition, "<rate absspeed='0'/>");
			}
		}

		HifiControlsUit.Button {
			id: speedNeg10Button;
            focusPolicy: Qt.NoFocus;
			color: hifi.buttons.none;
			colorScheme: hifi.colorSchemes.dark;
            anchors.top: tagButtonTitle.bottom;
            anchors.left: speed0Button.right;
            anchors.leftMargin: 6;
            width: parent.width/6 - anchors.leftMargin;
			height: 30;
			text: "Speed -10";
			onClicked: {
                messageToSpeak.insert(messageToSpeak.cursorPosition, "<rate absspeed='-10'/>");
			}
		}
    }

	Item {
		anchors.top: tagButtonContainer.bottom;
        anchors.topMargin: 8;
		anchors.bottom: keyboardContainer.top;
        anchors.bottomMargin: 16;
		anchors.left: parent.left;
        anchors.leftMargin: 16;
		anchors.right: parent.right;
        anchors.rightMargin: 16;

        TextArea {
            id: messageToSpeak;
            font.family: "Fira Sans SemiBold";
            font.pixelSize: 20;
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: speakButton.top;
            anchors.bottomMargin: 8;
            // Style
            background: Rectangle {
                anchors.fill: parent;
                color: parent.activeFocus ? hifi.colors.black : hifi.colors.baseGrayShadow;
                border.width: parent.activeFocus ? 1 : 0;
                border.color: parent.activeFocus ? hifi.colors.primaryHighlight : hifi.colors.textFieldLightBackground;
            }
            color: hifi.colors.white;
            textFormat: TextEdit.PlainText;
            wrapMode: TextEdit.Wrap;
            activeFocusOnPress: true;
            activeFocusOnTab: true;
            Keys.onPressed: {
                if (event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                    TextToSpeech.speakText(messageToSpeak.text, 480, 10, 24000, 16, true);
                    event.accepted = true;
                }
            }

            HifiStylesUit.FiraSansRegular {
                text: "<i>Input Text to Speak...</i>";
                size: 20;
                anchors.fill: parent;
                anchors.topMargin: 4;
                anchors.leftMargin: 4;
                color: hifi.colors.lightGrayText;
                visible: !parent.activeFocus && messageToSpeak.text === "";
                verticalAlignment: Text.AlignTop;
            }
        }

		HifiControlsUit.Button {
			id: speakButton;
            focusPolicy: Qt.NoFocus;
			color: hifi.buttons.blue;
			colorScheme: hifi.colorSchemes.dark;
			anchors.right: parent.right;
            anchors.bottom: parent.bottom;
            width: 215;
			height: 40;
			text: "Speak";
			onClicked: {
                TextToSpeech.speakText(messageToSpeak.text, 480, 10, 24000, 16, true);
			}
		}

		HifiControlsUit.Button {
			id: clearButton;
            focusPolicy: Qt.NoFocus;
			color: hifi.buttons.white;
			colorScheme: hifi.colorSchemes.dark;
			anchors.right: speakButton.left;
            anchors.rightMargin: 16;
            anchors.bottom: parent.bottom;
            width: 100;
			height: 40;
			text: "Clear";
			onClicked: {
				messageToSpeak.text = "";
			}
		}

		HifiControlsUit.Button {
			id: stopButton;
            focusPolicy: Qt.NoFocus;
			color: hifi.buttons.red;
			colorScheme: hifi.colorSchemes.dark;
			anchors.right: clearButton.left;
            anchors.rightMargin: 16;
            anchors.bottom: parent.bottom;
            width: 100;
			height: 40;
			text: "Stop Last";
			onClicked: {
				TextToSpeech.stopLastSpeech();
			}
		}
	}

    Item {
        id: keyboardContainer;
        z: 998;
        visible: keyboard.raised;
        property bool punctuationMode: false;
        anchors {
            bottom: parent.bottom;
            left: parent.left;
            right: parent.right;
        }

        HifiControlsUit.Keyboard {
            id: keyboard;
            raised: HMD.mounted && root.keyboardRaised;
            numeric: parent.punctuationMode;
            anchors {
                bottom: parent.bottom;
                left: parent.left;
                right: parent.right;
            }
        }
    }
}
