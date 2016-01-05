import Hifi 1.0 as Hifi
import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import "controls"
import "styles"

VrDialog {
    id: root
    HifiConstants { id: hifi }
    property real spacing: hifi.layout.spacing
    property real outerSpacing: hifi.layout.spacing * 2

    destroyOnCloseButton: true
    destroyOnInvisible: true
    contentImplicitWidth: content.implicitWidth
    contentImplicitHeight: content.implicitHeight

    Component.onCompleted: {
        enabled = true
    }

    onParentChanged: {
        if (visible && enabled) {
            forceActiveFocus();
        }
    }

    Hifi.MessageDialog {
        id: content
        clip: true

        x: root.clientX
        y: root.clientY
        implicitHeight: contentColumn.implicitHeight + outerSpacing * 2
        implicitWidth: mainText.implicitWidth + outerSpacing * 2

        Component.onCompleted: {
            root.title = title
        }

        onTitleChanged: {
            root.title = title
        }

        Column {
            anchors.fill: parent
            anchors.margins: 8
            id: contentColumn
            spacing: root.outerSpacing

            Item {
                width: parent.width
                height: Math.max(icon.height, mainText.height + informativeText.height + root.spacing)

                FontAwesome {
                    id: icon
                    width: content.icon ? 48 : 0
                    height: content.icon ? 48 : 0
                    visible: content.icon ? true : false
                    font.pixelSize: 48
                    verticalAlignment: Text.AlignTop
                    horizontalAlignment: Text.AlignLeft
                    color: iconColor()
                    text: iconSymbol()

                    function iconSymbol() {
                        switch (content.icon) {
                            case Hifi.MessageDialog.Information:
                                return "\uF05A" 
                            case Hifi.MessageDialog.Question:
                                return "\uF059" 
                            case Hifi.MessageDialog.Warning:
                                return "\uF071" 
                            case Hifi.MessageDialog.Critical:
                                return "\uF057" 
                            default:
                                break;
                        }
                        return content.icon;
                    }
                    function iconColor() {
                        switch (content.icon) {
                            case Hifi.MessageDialog.Information:
                            case Hifi.MessageDialog.Question:
                                return "blue"
                            case Hifi.MessageDialog.Warning:
                            case Hifi.MessageDialog.Critical:
                                return "red"
                            default:
                                break
                        }
                        return "black"
                    }
                }

                Text {
                    id: mainText
                    anchors {
                        left: icon.right
                        leftMargin: root.spacing
                        right: parent.right
                    }
                    text: content.text
                    font.pointSize: 14
                    font.weight: Font.Bold
                    wrapMode: Text.WordWrap
                }

                Text {
                    id: informativeText
                    anchors {
                        left: icon.right
                        right: parent.right
                        top: mainText.bottom
                        leftMargin: root.spacing
                        topMargin: root.spacing
                    }
                    text: content.informativeText
                    font.pointSize: 14
                    wrapMode: Text.WordWrap
                }
            }


            Flow {
                id: buttons
                spacing: root.spacing
                layoutDirection: Qt.RightToLeft
                width: parent.width
                Button {
                    id: okButton
                    text: qsTr("OK")
                    onClicked: content.click(StandardButton.Ok)
                    visible: content.standardButtons & StandardButton.Ok
                }
                Button {
                    id: openButton
                    text: qsTr("Open")
                    onClicked: content.click(StandardButton.Open)
                    visible: content.standardButtons & StandardButton.Open
                }
                Button {
                    id: saveButton
                    text: qsTr("Save")
                    onClicked: content.click(StandardButton.Save)
                    visible: content.standardButtons & StandardButton.Save
                }
                Button {
                    id: saveAllButton
                    text: qsTr("Save All")
                    onClicked: content.click(StandardButton.SaveAll)
                    visible: content.standardButtons & StandardButton.SaveAll
                }
                Button {
                    id: retryButton
                    text: qsTr("Retry")
                    onClicked: content.click(StandardButton.Retry)
                    visible: content.standardButtons & StandardButton.Retry
                }
                Button {
                    id: ignoreButton
                    text: qsTr("Ignore")
                    onClicked: content.click(StandardButton.Ignore)
                    visible: content.standardButtons & StandardButton.Ignore
                }
                Button {
                    id: applyButton
                    text: qsTr("Apply")
                    onClicked: content.click(StandardButton.Apply)
                    visible: content.standardButtons & StandardButton.Apply
                }
                Button {
                    id: yesButton
                    text: qsTr("Yes")
                    onClicked: content.click(StandardButton.Yes)
                    visible: content.standardButtons & StandardButton.Yes
                }
                Button {
                    id: yesAllButton
                    text: qsTr("Yes to All")
                    onClicked: content.click(StandardButton.YesToAll)
                    visible: content.standardButtons & StandardButton.YesToAll
                }
                Button {
                    id: noButton
                    text: qsTr("No")
                    onClicked: content.click(StandardButton.No)
                    visible: content.standardButtons & StandardButton.No
                }
                Button {
                    id: noAllButton
                    text: qsTr("No to All")
                    onClicked: content.click(StandardButton.NoToAll)
                    visible: content.standardButtons & StandardButton.NoToAll
                }
                Button {
                    id: discardButton
                    text: qsTr("Discard")
                    onClicked: content.click(StandardButton.Discard)
                    visible: content.standardButtons & StandardButton.Discard
                }
                Button {
                    id: resetButton
                    text: qsTr("Reset")
                    onClicked: content.click(StandardButton.Reset)
                    visible: content.standardButtons & StandardButton.Reset
                }
                Button {
                    id: restoreDefaultsButton
                    text: qsTr("Restore Defaults")
                    onClicked: content.click(StandardButton.RestoreDefaults)
                    visible: content.standardButtons & StandardButton.RestoreDefaults
                }
                Button {
                    id: cancelButton
                    text: qsTr("Cancel")
                    onClicked: content.click(StandardButton.Cancel)
                    visible: content.standardButtons & StandardButton.Cancel
                }
                Button {
                    id: abortButton
                    text: qsTr("Abort")
                    onClicked: content.click(StandardButton.Abort)
                    visible: content.standardButtons & StandardButton.Abort
                }
                Button {
                    id: closeButton
                    text: qsTr("Close")
                    onClicked: content.click(StandardButton.Close)
                    visible: content.standardButtons & StandardButton.Close
                }
                Button {
                    id: moreButton
                    text: qsTr("Show Details...")
                    onClicked: content.state = (content.state === "" ? "expanded" : "")
                    visible: content.detailedText.length > 0
                }
                Button {
                    id: helpButton
                    text: qsTr("Help")
                    onClicked: content.click(StandardButton.Help)
                    visible: content.standardButtons & StandardButton.Help
                }
            }
        }

        Item {
            id: details
            width: parent.width
            implicitHeight: detailedText.implicitHeight + root.spacing
            height: 0
            clip: true

            anchors {
                left: parent.left
                right: parent.right
                top: contentColumn.bottom
                topMargin: root.spacing
                leftMargin: root.outerSpacing
                rightMargin: root.outerSpacing
            }

            Flickable {
                id: flickable
                contentHeight: detailedText.height
                anchors.fill: parent
                anchors.topMargin: root.spacing
                anchors.bottomMargin: root.outerSpacing
                TextEdit {
                    id: detailedText
                    text: content.detailedText
                    width: details.width
                    wrapMode: Text.WordWrap
                    readOnly: true
                    selectByMouse: true
                }
            }
        }

        states: [
            State {
                name: "expanded"
                PropertyChanges {
                    target: details
                    height: root.height - contentColumn.height - root.spacing - root.outerSpacing
                }
                PropertyChanges {
                    target: content
                    implicitHeight: contentColumn.implicitHeight + root.spacing * 2 +
                        detailedText.implicitHeight + root.outerSpacing * 2
                }
                PropertyChanges {
                    target: moreButton
                    text: qsTr("Hide Details")
                }
            }
        ]
    }


    Keys.onPressed: {
        if (!enabled) {
            return
        }

        if (event.modifiers === Qt.ControlModifier)
            switch (event.key) {
            case Qt.Key_A:
                event.accepted = true
                detailedText.selectAll()
                break
            case Qt.Key_C:
                event.accepted = true
                detailedText.copy()
                break
            case Qt.Key_Period:
                if (Qt.platform.os === "osx") {
                    event.accepted = true
                    content.reject()
                }
                break
        } else switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                event.accepted = true
                content.reject()
                break

            case Qt.Key_Enter:
            case Qt.Key_Return:
                event.accepted = true
                content.accept()
                break
        }
    }
}
