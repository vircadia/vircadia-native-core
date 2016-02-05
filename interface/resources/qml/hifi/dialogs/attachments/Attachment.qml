import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import "../../../windows"
import "../../../controls" as VrControls
import "."
import ".."

Item {
    height: column.height + 2 * 8

    property var attachment;

    signal deleteAttachment(var attachment);
    signal updateAttachment();
    property bool completed: false;

    Rectangle { color: "white"; anchors.fill: parent; radius: 4 }

    Component.onCompleted: {
        completed = true;
    }

    Column {
        y: 8
        id: column
        anchors { left: parent.left; right: parent.right; margins: 8 }
        spacing: 8

        Item {
            height: modelChooserButton.height
            anchors { left: parent.left; right: parent.right; }
            Text { id: urlLabel; text: "Model URL:"; width: 80; anchors.verticalCenter: modelUrl.verticalCenter }
            VrControls.TextField {
                id: modelUrl;
                height:  jointChooser.height;
                anchors { left: urlLabel.right; leftMargin: 8; rightMargin: 8; right: modelChooserButton.left }
                text: attachment ? attachment.modelUrl : ""
                onTextChanged: {
                    if (completed && attachment && attachment.modelUrl !== text) {
                        attachment.modelUrl = text;
                        updateAttachment();
                    }
                }
            }
            Button {
                id: modelChooserButton;
                text: "Choose";
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
            height: jointChooser.height
            anchors { left: parent.left; right: parent.right; }
            Text {
                id: jointLabel;
                width: 80;
                text: "Joint:";
                anchors.verticalCenter: jointChooser.verticalCenter;
            }
            VrControls.ComboBox {
                id: jointChooser;
                anchors { left: jointLabel.right; leftMargin: 8; right: parent.right }
                model: MyAvatar.jointNames
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
            height: translation.height
            anchors { left: parent.left; right: parent.right; }
            Text { id: translationLabel; width: 80; text: "Translation:"; anchors.verticalCenter: translation.verticalCenter; }
            Translation {
                id: translation;
                anchors { left: translationLabel.right; leftMargin: 8; right: parent.right }
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
            height: rotation.height
            anchors { left: parent.left; right: parent.right; }
            Text { id: rotationLabel; width: 80; text: "Rotation:"; anchors.verticalCenter: rotation.verticalCenter; }
            Rotation {
                id: rotation;
                anchors { left: rotationLabel.right; leftMargin: 8; right: parent.right }
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
            height: scaleSpinner.height
            anchors { left: parent.left; right: parent.right; }
            Text { id: scaleLabel; width: 80; text: "Scale:"; anchors.verticalCenter: scale.verticalCenter; }
            SpinBox {
                id: scaleSpinner;
                anchors { left: scaleLabel.right; leftMargin: 8; right: parent.right }
                decimals: 1;
                minimumValue: 0.1
                maximumValue: 10
                stepSize: 0.1;
                value: attachment ? attachment.scale : 1.0
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
            Text { id: softLabel; width: 80; text: "Is soft:"; anchors.verticalCenter: soft.verticalCenter; }
            CheckBox {
                id: soft;
                anchors { left: softLabel.right; leftMargin: 8; right: parent.right }
                checked: attachment ? attachment.soft : false
                onCheckedChanged: {
                    if (completed && attachment && attachment.soft !== checked) {
                        attachment.soft = checked;
                        updateAttachment();
                    }
                }
            }
        }

        Button {
            anchors { left: parent.left; right: parent.right; }
            text: "Delete"
            onClicked: deleteAttachment(root.attachment);
        }
    }
}
