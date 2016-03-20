import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

//import "../../../windows"
//import "../../../controls" as VrControls
import "."
import ".."

import "../../../styles-uit"
import "../../../controls-uit" as HifiControls
import "../../../windows-uit"

Item {
    height: column.height + 2 * 8

    property var attachment;

    HifiConstants { id: hifi }

    signal deleteAttachment(var attachment);
    signal updateAttachment();
    property bool completed: false;

    Rectangle { color: hifi.colors.baseGray; anchors.fill: parent; radius: 4 }

    Component.onCompleted: {
        completed = true;
    }

    Column {
        y: 8
        id: column
        anchors { left: parent.left; right: parent.right; margins: 8 }
        spacing: 8

        Item {
            height: modelChooserButton.height + urlLabel.height
            anchors { left: parent.left; right: parent.right;}
            Text { id: urlLabel; color: hifi.colors.lightGrayText; text: "Model URL:"; width: 80;  anchors.top: parent.top;}
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
                    id: modelBrowserBuiler;
                    ModelBrowserDialog {}
                }

                onClicked: {
                    var browser = modelBrowserBuiler.createObject(desktop);
                    browser.selected.connect(function(newModelUrl){
                        modelUrl.text = newModelUrl;
                    })
                }
            }
        }

        Item {
            height: jointChooser.height + jointLabel.height
            anchors { left: parent.left; right: parent.right; }
            Text {
                id: jointLabel;
                width: 80;
                text: "Joint:";
                color: hifi.colors.lightGrayText;
                anchors.top: parent.top
            }
            HifiControls.ComboBox {
                id: jointChooser;
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                model: MyAvatar.jointNames
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
            height: translation.height + translationLabel.height
            anchors { left: parent.left; right: parent.right; }
            Text { id: translationLabel; width: 80; color: hifi.colors.lightGrayText; text: "Translation:"; anchors.top: parent.top; }
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
            height: rotation.height + rotationLabel.height
            anchors { left: parent.left; right: parent.right; }
            Text { id: rotationLabel; width: 80; color: hifi.colors.lightGrayText; text: "Rotation:"; anchors.top: parent.top; }
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
            height: scaleSpinner.height + scaleLabel.height
            anchors { left: parent.left; right: parent.right; }
            Text { id: scaleLabel; width: 80; color: hifi.colors.lightGrayText; text: "Scale:"; anchors.top: parent.top; }
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
            height: soft.height
            anchors { left: parent.left; right: parent.right; }
            Text { id: softLabel; width: 80; color: hifi.colors.lightGrayText; text: "Is soft"; anchors.left: soft.right; anchors.leftMargin: 8; }
            HifiControls.CheckBox {
                id: soft;
                anchors { left: parent.left; bottom: parent.bottom;}
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

        HifiControls.Button {
            color: hifi.buttons.black
            colorScheme: hifi.colorSchemes.dark
            anchors { left: parent.left; right: parent.right; }
            text: "Delete"
            onClicked: deleteAttachment(root.attachment);
        }
    }
}
