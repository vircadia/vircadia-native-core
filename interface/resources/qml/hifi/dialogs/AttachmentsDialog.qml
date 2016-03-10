import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

import "../../windows"
import "attachments"

Window {
    id: root
    title: "Edit Attachments"
    objectName: "AttachmentsDialog"
    width: 600
    height: 600
    resizable: true
    // User must click OK or cancel to close the window
    closable: false

    readonly property var originalAttachments: MyAvatar.getAttachmentsVariant();
    property var attachments: [];

    property var settings: Settings {
        category: "AttachmentsDialog"
        property alias x: root.x
        property alias y: root.y
        property alias width: root.width
        property alias height: root.height
    }

    Component.onCompleted: {
        for (var i = 0; i < originalAttachments.length; ++i) {
            var attachment = originalAttachments[i];
            root.attachments.push(attachment);
            listView.model.append({});
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 4

        Rectangle {
            id: attachmentsBackground
            anchors { left: parent.left; right: parent.right; top: parent.top; bottom: newAttachmentButton.top; margins: 8 }
            color: "gray"
            radius: 4

            ScrollView{
                id: scrollView
                anchors.fill: parent
                anchors.margins: 4
                ListView {
                    id: listView
                    model: ListModel {}
                    delegate:  Item {
                        implicitHeight: attachmentView.height + 8;
                        implicitWidth: attachmentView.width;
                        Attachment {
                            id: attachmentView
                            width: scrollView.width
                            attachment: root.attachments[index]
                            onDeleteAttachment: {
                                attachments.splice(index, 1);
                                listView.model.remove(index, 1);
                            }
                            onUpdateAttachment: MyAvatar.setAttachmentsVariant(attachments);
                        }
                    }
                    onCountChanged: MyAvatar.setAttachmentsVariant(attachments);
                }
            }
        }

        Button {
            id: newAttachmentButton
            anchors { left: parent.left; right: parent.right; bottom: buttonRow.top; margins: 8 }
            text: "New Attachment"

            onClicked: {
                var template = {
                    modelUrl: "",
                    translation: { x: 0, y: 0, z: 0 },
                    rotation: { x: 0, y: 0, z: 0 },
                    scale: 1,
                    jointName: MyAvatar.jointNames[0],
                    soft: false
                };
                attachments.push(template);
                listView.model.append({});
                MyAvatar.setAttachmentsVariant(attachments);
            }
        }

        Row {
            id: buttonRow
            spacing: 8
            anchors { right: parent.right; bottom: parent.bottom; margins: 8 }
            Button { action: cancelAction }
            Button { action: okAction }
        }

        Action {
            id: cancelAction
            text: "Cancel"
            onTriggered: {
                MyAvatar.setAttachmentsVariant(originalAttachments);
                root.destroy()
            }
        }

        Action {
            id: okAction
            text: "OK"
            onTriggered: {
                for (var i = 0; i < attachments.length; ++i) {
                    console.log("Attachment " + i + ": " + attachments[i]);
                }

                MyAvatar.setAttachmentsVariant(attachments);
                root.destroy()
            }
        }
    }
}

