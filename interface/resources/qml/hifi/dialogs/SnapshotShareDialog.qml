import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.XmlListModel 2.0

import "../../windows"
import "../../js/Utils.js" as Utils
import "../models"

Window {
    id: root
    resizable: true
    width: 516
    height: 616
    minSize: Qt.vector2d(500, 600);
    maxSize: Qt.vector2d(1000, 800);

    property alias source: image.source

    Rectangle {
        anchors.fill: parent
        color: "white"

        Item {
            anchors { fill: parent; margins: 8 }

            Image {
                id: image
                anchors { top: parent.top; left: parent.left; right: parent.right; bottom: notesLabel.top; bottomMargin: 8 }
                fillMode: Image.PreserveAspectFit
            }

            Text {
                id: notesLabel
                anchors { left: parent.left; bottom: notes.top; bottomMargin: 8; }
                text: "Notes about this image"
                font.pointSize: 14
                font.bold: true
                color: "#666"
            }

            TextArea {
                id: notes
                anchors { left: parent.left; bottom: parent.bottom;  right: shareButton.left; rightMargin: 8 }
                height: 60
            }

            Button {
                id: shareButton
                anchors { verticalCenter: notes.verticalCenter; right: parent.right; }
                width: 120; height: 50
                text: "Share"

                style: ButtonStyle {
                    background:  Rectangle {
                        implicitWidth: 120
                        implicitHeight: 50
                        border.width: control.activeFocus ? 2 : 1
                        color: "#333"
                        radius: 9
                    }
                    label: Text {
                        color: shareButton.enabled ? "white" : "gray"
                        font.pixelSize: 18
                        font.bold: true
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        anchors.fill: parent
                        text: shareButton.text
                    }
                }

                onClicked: {
                    enabled = false;
                    uploadTimer.start();
                }

                Timer {
                    id: uploadTimer
                    running: false
                    interval: 5
                    repeat: false
                    onTriggered: {
                        var uploaded = SnapshotUploader.uploadSnapshot(root.source.toString())
                        console.log("Uploaded result " + uploaded)
                        if (!uploaded) {
                            console.log("Upload failed ");
                        }
                    }
                }
            }
        }

        Action {
            id: shareAction
            text: qsTr("OK")
            enabled: root.result ? true : false
            shortcut: Qt.Key_Return
            onTriggered: {
                root.destroy();
            }
        }

        Action {
            id: cancelAction
            text: qsTr("Cancel")
            shortcut: Qt.Key_Escape
            onTriggered: {
                root.destroy();
            }
        }
    }
}




