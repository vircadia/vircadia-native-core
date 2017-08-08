import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import QtQuick.Controls.Styles 1.4

import "../../../styles-uit"
import "../../../controls-uit" as HifiControls
import "../../../windows"
import "../attachments"

Item {
    id: content

    readonly property var originalAttachments: MyAvatar.getAttachmentsVariant();
    property var attachments: [];

    Component.onCompleted: {
        for (var i = 0; i < originalAttachments.length; ++i) {
            var attachment = originalAttachments[i];
            content.attachments.push(attachment);
            listView.model.append({});
        }
    }

    Column {
        width: pane.width

        Rectangle {
            width: parent.width
            height: root.height - (keyboardEnabled && keyboardRaised ? 200 : 0)
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

                ListView {
                    id: listView
                    anchors {
                        top: parent.top
                        left: parent.left
                        right: scrollBar.left
                        bottom: parent.bottom
                        margins: 4
                    }
                    clip: true
                    cacheBuffer: 4000

                    model: ListModel {}
                    delegate: Item {
                        id: attachmentDelegate
                        implicitHeight: attachmentView.height + 8;
                        implicitWidth: attachmentView.width

                        MouseArea {
                            // User can click on whitespace to select item.
                            anchors.fill: parent
                            propagateComposedEvents: true
                            onClicked: {
                                listView.currentIndex = index;
                                attachmentsBackground.forceActiveFocus();  // Unfocus from any control.
                                mouse.accepted = false;
                            }
                        }

                        Attachment {
                            id: attachmentView
                            width: listView.width
                            attachment: content.attachments[index]
                            onSelectAttachment: {
                                listView.currentIndex = index;
                            }
                            onDeleteAttachment: {
                                attachments.splice(index, 1);
                                listView.model.remove(index, 1);
                            }
                            onUpdateAttachment: MyAvatar.setAttachmentsVariant(attachments);
                        }
                    }

                    onCountChanged: MyAvatar.setAttachmentsVariant(attachments);

                    /*
                    // DEBUG
                    highlight: Rectangle { color: "#40ffff00" }
                    highlightFollowsCurrentItem: true
                    */

                    onHeightChanged: {
                        // Keyboard has been raised / lowered.
                        positionViewAtIndex(listView.currentIndex, ListView.SnapPosition);
                    }

                    onCurrentIndexChanged: {
                        if (!yScrollTimer.running) {
                            scrollSlider.y = currentIndex * (scrollBar.height - scrollSlider.height) / (listView.count - 1);
                        }
                    }

                    onContentYChanged: {
                        // User may have dragged content up/down.
                        yScrollTimer.restart();
                    }

                    Timer {
                        id: yScrollTimer
                        interval: 200
                        repeat: false
                        running: false
                        onTriggered: {
                            var index = (listView.count - 1) * listView.contentY / (listView.contentHeight - scrollBar.height);
                            index = Math.round(index);
                            listView.currentIndex = index;
                            scrollSlider.y = index * (scrollBar.height - scrollSlider.height) / (listView.count - 1);
                        }
                    }
                }

                Rectangle {
                    id: scrollBar

                    property bool scrolling: listView.contentHeight > listView.height

                    anchors {
                        top: parent.top
                        right: parent.right
                        bottom: parent.bottom
                        topMargin: 4
                        bottomMargin: 4
                    }
                    width: scrolling ? 18 : 0
                    radius: attachmentsBackground.radius
                    color: hifi.colors.baseGrayShadow

                    MouseArea {
                        anchors.fill: parent

                        onClicked: {
                            var index = listView.currentIndex;
                            index = index + (mouse.y <= scrollSlider.y ? -1 : 1);
                            if (index < 0) {
                                index = 0;
                            }
                            if (index > listView.count - 1) {
                                index = listView.count - 1;
                            }
                            listView.currentIndex = index;
                        }
                    }

                    Rectangle {
                        id: scrollSlider
                        anchors {
                            right: parent.right
                            rightMargin: 3
                        }
                        width: 16
                        height: (listView.height / listView.contentHeight) * listView.height
                        radius: width / 2
                        color: hifi.colors.lightGray

                        visible: scrollBar.scrolling;

                        onYChanged: {
                            var index = y * (listView.count - 1) / (scrollBar.height - scrollSlider.height);
                            index = Math.round(index);
                            listView.currentIndex = index;
                        }

                        MouseArea {
                            anchors.fill: parent
                            drag.target: scrollSlider
                            drag.axis: Drag.YAxis
                            drag.minimumY: 0
                            drag.maximumY: scrollBar.height - scrollSlider.height
                        }
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
                    closeDialog();
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
                    closeDialog();
                }
            }
        }
    }
}
