import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0
import QtQuick.Controls.Styles 1.4

import "../../styles-uit"
import "../../controls-uit" as HifiControls
import "../../windows"
import "attachments"

ScrollingWindow {
    id: root
    title: "Attachments"
    objectName: "AttachmentsDialog"
    width: 600
    height: 600
    resizable: true
    destroyOnHidden: true
    minSize: Qt.vector2d(400, 500)

    HifiConstants { id: hifi }

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

    Column {
        width: pane.contentWidth

        Rectangle {
            width: parent.width
            height: root.height - (keyboardEnabled && keyboardRaised ? 200 : 0)
            radius: 4
            color: hifi.colors.baseGray

            Rectangle {
                id: attachmentsBackground
                anchors {
                    left: parent.left; right: parent.right; top: parent.top; bottom: newAttachmentButton.top;
                    margins: hifi.dimensions.contentMargin.x
                    bottomMargin: hifi.dimensions.contentSpacing.y
                }
                color: hifi.colors.baseGrayShadow
                radius: 4

                ScrollView {
                    id: scrollView
                    anchors.fill: parent
                    anchors.margins: 4

                    style: ScrollViewStyle {

                        padding {
                            top: 0
                            right: 0
                            bottom: 0
                        }

                        decrementControl: Item {
                            visible: false
                        }
                        incrementControl: Item {
                            visible: false
                        }
                        scrollBarBackground: Rectangle{
                            implicitWidth: 14
                            color: hifi.colors.baseGray
                            radius: 4
                            Rectangle {
                                // Make top left corner of scrollbar appear square
                                width: 8
                                height: 4
                                color: hifi.colors.baseGray
                                anchors.top: parent.top
                                anchors.horizontalCenter: parent.left
                            }

                        }
                        handle:
                            Rectangle {
                            implicitWidth: 8
                            anchors {
                                left: parent.left
                                leftMargin: 3
                                top: parent.top
                                topMargin: 3
                                bottom: parent.bottom
                                bottomMargin: 4
                            }
                            radius: 4
                            color: hifi.colors.lightGrayText
                        }
                    }

                    ListView {
                        id: listView
                        model: ListModel {}
                        delegate: Item {
                            id: attachmentDelegate
                            implicitHeight: attachmentView.height + 8;
                            implicitWidth: attachmentView.width
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

                    function scrollBy(delta) {
                        flickableItem.contentY += delta;
                    }
                }
            }

            HifiControls.Button {
                id: newAttachmentButton
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: buttonRow.top
                    margins: hifi.dimensions.contentMargin.x;
                    topMargin: hifi.dimensions.contentSpacing.y
                    bottomMargin: hifi.dimensions.contentSpacing.y
                }
                text: "New Attachment"
                color: hifi.buttons.black
                colorScheme: hifi.colorSchemes.dark
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
                anchors {
                    right: parent.right
                    bottom: parent.bottom
                    margins: hifi.dimensions.contentMargin.x
                    topMargin: hifi.dimensions.contentSpacing.y
                    bottomMargin: hifi.dimensions.contentSpacing.y
                }
                HifiControls.Button {
                    action: okAction
                    color: hifi.buttons.black
                    colorScheme: hifi.colorSchemes.dark
                }
                HifiControls.Button {
                    action: cancelAction
                    color: hifi.buttons.black
                    colorScheme: hifi.colorSchemes.dark
                }
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

    onKeyboardRaisedChanged: {
        if (keyboardEnabled && keyboardRaised) {
            // Scroll to item with focus if necessary.
            var footerHeight = newAttachmentButton.height + buttonRow.height + 3 * hifi.dimensions.contentSpacing.y;
            var delta = activator.mouseY
                    - (activator.height + activator.y - 200 - footerHeight - hifi.dimensions.controlLineHeight);

            if (delta > 0) {
                scrollView.scrollBy(delta);
            } else {
                // HACK: Work around for case where are 100% scrolled; stops window from erroneously scrolling to 100% when show keyboard.
                scrollView.scrollBy(-1);
                scrollView.scrollBy(1);
            }
        }
    }
}

