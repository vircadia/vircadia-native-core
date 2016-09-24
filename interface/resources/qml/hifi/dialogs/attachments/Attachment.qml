import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "."
import ".."
import "../../../styles-uit"
import "../../../controls-uit" as HifiControls
import "../../../windows"

Item {
    height: column.height + 2 * 8

    property var attachment;

    HifiConstants { id: hifi }

    signal deleteAttachment(var attachment);
    signal updateAttachment();
    property bool completed: false;

    Rectangle { color: hifi.colors.baseGray; anchors.fill: parent; radius: 4 }

    Component.onCompleted: {
        jointChooser.model = MyAvatar.jointNames;
        completed = true;
    }

    Column {
        y: 8
        id: column
        anchors { left: parent.left; right: parent.right; margins: 20 }
        spacing: 8

        property bool keyboardRaised: false
        property bool punctuationMode: false

        Item {
            height: modelChooserButton.height + urlLabel.height + 4
            anchors { left: parent.left; right: parent.right;}
            HifiControls.Label { id: urlLabel; color: hifi.colors.lightGrayText; text: "Model URL"; anchors.top: parent.top;}
            HifiControls.TextField {
                id: modelUrl;
                height:  jointChooser.height;
                colorScheme: hifi.colorSchemes.dark
                anchors { bottom: parent.bottom; left: parent.left; rightMargin: 8; right: modelChooserButton.left }
                text: attachment ? attachment.modelUrl : ""
                onTextChanged: {
                    if (completed && attachment && attachment.modelUrl !== text) {
                        attachment.modelUrl = text;
                        updateAttachment();
                    }
                }
            }
            HifiControls.Button {
                id: modelChooserButton;
                text: "Choose";
                color: hifi.buttons.black
                colorScheme: hifi.colorSchemes.dark
                anchors { right: parent.right; verticalCenter: modelUrl.verticalCenter }
                Component {
                    id: modelBrowserBuilder;
                    ModelBrowserDialog {}
                }

                onClicked: {
                    var browser = modelBrowserBuilder.createObject(desktop);
                    browser.selected.connect(function(newModelUrl){
                        modelUrl.text = newModelUrl;
                    })
                }
            }
        }

        // virtual keyboard, letters
        HifiControls.Keyboard {
            id: keyboard1
            // y: parent.keyboardRaised ? parent.height : 0
            height: parent.keyboardRaised ? 200 : 0
            visible: parent.keyboardRaised && !parent.punctuationMode
            enabled: parent.keyboardRaised && !parent.punctuationMode
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            // anchors.bottom: buttonRow.top
            // anchors.bottomMargin: 2 * hifi.dimensions.contentSpacing.y
        }

        HifiControls.KeyboardPunctuation {
            id: keyboard2
            // y: parent.keyboardRaised ? parent.height : 0
            height: parent.keyboardRaised ? 200 : 0
            visible: parent.keyboardRaised && parent.punctuationMode
            enabled: parent.keyboardRaised && parent.punctuationMode
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            // anchors.bottom: buttonRow.top
            // anchors.bottomMargin: 2 * hifi.dimensions.contentSpacing.y
        }

        Item {
            height: jointChooser.height + jointLabel.height + 4
            anchors { left: parent.left; right: parent.right; }
            HifiControls.Label {
                id: jointLabel;
                text: "Joint";
                color: hifi.colors.lightGrayText;
                anchors.top: parent.top
            }
            HifiControls.ComboBox {
                id: jointChooser;
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                colorScheme: hifi.colorSchemes.dark
                currentIndex: attachment ? model.indexOf(attachment.jointName) : -1
                onCurrentIndexChanged: {
                    if (completed && attachment && currentIndex != -1 && currentText && currentText !== attachment.jointName) {
                        attachment.jointName = currentText;
                        updateAttachment();
                    }
                }
            }
        }

        Item {
            height: translation.height + translationLabel.height + 4
            anchors { left: parent.left; right: parent.right; }
            HifiControls.Label { id: translationLabel; color: hifi.colors.lightGrayText; text: "Translation"; anchors.top: parent.top; }
            Translation {
                id: translation;
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom}
                vector: attachment ? attachment.translation : {x: 0, y: 0, z: 0};
                onValueChanged: {
                    if (completed && attachment) {
                        attachment.translation = vector;
                        updateAttachment();
                    }
                }
            }
        }

        Item {
            height: rotation.height + rotationLabel.height + 4
            anchors { left: parent.left; right: parent.right; }
            HifiControls.Label { id: rotationLabel; color: hifi.colors.lightGrayText; text: "Rotation"; anchors.top: parent.top; }
            Rotation {
                id: rotation;
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom; }
                vector: attachment ? attachment.rotation : {x: 0, y: 0, z: 0};
                onValueChanged: {
                    if (completed && attachment) {
                        attachment.rotation = vector;
                        updateAttachment();
                    }
                }
            }
        }

        Item {
            height: scaleItem.height
            anchors { left: parent.left; right: parent.right; }

            Item {
                id: scaleItem
                height: scaleSpinner.height + scaleLabel.height + 4
                width: parent.width / 3 - 8
                anchors { right: parent.right; }
                HifiControls.Label { id: scaleLabel; color: hifi.colors.lightGrayText; text: "Scale"; anchors.top: parent.top; }
                HifiControls.SpinBox {
                    id: scaleSpinner;
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom; }
                    decimals: 1;
                    minimumValue: 0.1
                    maximumValue: 10
                    stepSize: 0.1;
                    value: attachment ? attachment.scale : 1.0
                    colorScheme: hifi.colorSchemes.dark
                    onValueChanged: {
                        if (completed && attachment && attachment.scale !== value) {
                            attachment.scale = value;
                            updateAttachment();
                        }
                    }
                }
            }

            Item {
                id: isSoftItem
                height: scaleSpinner.height
                anchors {
                    left: parent.left
                    bottom: parent.bottom
                }
                HifiControls.CheckBox {
                    id: soft
                    text: "Is soft"
                    anchors {
                        left: parent.left
                        verticalCenter: parent.verticalCenter
                    }
                    checked: attachment ? attachment.soft : false
                    colorScheme: hifi.colorSchemes.dark
                    onCheckedChanged: {
                        if (completed && attachment && attachment.soft !== checked) {
                            attachment.soft = checked;
                            updateAttachment();
                        }
                    }
                }
            }
        }
        HifiControls.Button {
            color: hifi.buttons.black
            colorScheme: hifi.colorSchemes.dark
            anchors { left: parent.left; right: parent.right; }
            text: "Delete"
            onClicked: deleteAttachment(root.attachment);
        }
    }
}
