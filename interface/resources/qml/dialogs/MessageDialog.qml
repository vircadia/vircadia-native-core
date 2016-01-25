import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2 as OriginalDialogs

import "../controls"
import "../styles"
import "../windows"

// FIXME respect default button functionality
ModalWindow {
    id: root
    HifiConstants { id: hifi }
    implicitWidth: 640
    implicitHeight: 320
    destroyOnCloseButton: true
    destroyOnInvisible: true
    visible: true

    signal selected(int button);

    function click(button) {
        clickedButton = button;
        selected(button);
        destroy();
    }

    property alias detailedText: detailedText.text
    property alias text: mainTextContainer.text
    property alias informativeText: informativeTextContainer.text
    onIconChanged: iconHolder.updateIcon();
    property int buttons: OriginalDialogs.StandardButton.Ok
    property int icon: OriginalDialogs.StandardIcon.NoIcon
    property int defaultButton: OriginalDialogs.StandardButton.NoButton;
    property int clickedButton: OriginalDialogs.StandardButton.NoButton;
    focus: defaultButton === OriginalDialogs.StandardButton.NoButton

    Rectangle {
        id: messageBox
        clip: true
        anchors.fill: parent
        radius: 4
        color: "white"

        QtObject {
            id: d
            readonly property real spacing: hifi.layout.spacing
            readonly property real outerSpacing: hifi.layout.spacing * 2
            readonly property int minWidth: 480
            readonly property int maxWdith: 1280
            readonly property int minHeight: 160
            readonly property int maxHeight: 720

            function resize() {
                var targetWidth = iconHolder.width + mainTextContainer.width + d.spacing * 6
                var targetHeight = mainTextContainer.implicitHeight + informativeTextContainer.implicitHeight + d.spacing * 8 + buttons.height + details.height
                root.width = (targetWidth < d.minWidth) ? d.minWidth : ((targetWidth > d.maxWdith) ? d.maxWidth : targetWidth)
                root.height = (targetHeight < d.minHeight) ? d.minHeight: ((targetHeight > d.maxHeight) ? d.maxHeight : targetHeight)
            }
        }

        FontAwesome {
            id: iconHolder
            size: 48
            anchors {
                left: parent.left
                top: parent.top
                margins: d.spacing * 2
            }

            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            style: Text.Outline; styleColor: "black"
            Component.onCompleted: updateIcon();
            function updateIcon() {
                if (!root) {
                    return;
                }
                switch (root.icon) {
                    case OriginalDialogs.StandardIcon.Information:
                        text = "\uF05A";
                        color = "blue";
                        break;

                    case OriginalDialogs.StandardIcon.Question:
                        text = "\uF059"
                        color = "blue";
                        break;

                    case OriginalDialogs.StandardIcon.Warning:
                        text = "\uF071"
                        color = "yellow";
                        break;

                    case OriginalDialogs.StandardIcon.Critical:
                        text = "\uF057"
                        color = "red"
                        break;

                    default:
                        text = ""
                }
                visible = (text != "");
            }
        }

        Text {
            id: mainTextContainer
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            wrapMode: Text.WordWrap
            font { pointSize: 14; weight: Font.Bold }
            anchors { left: iconHolder.right; top: parent.top; margins: d.spacing * 2 }
        }

        Text {
            id: informativeTextContainer
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            wrapMode: Text.WordWrap
            font.pointSize: 11
            anchors { top: mainTextContainer.bottom; right: parent.right; left: iconHolder.right; margins: d.spacing * 2 }
        }

        Flow {
            id: buttons
            focus: true
            spacing: d.spacing
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            layoutDirection: Qt.RightToLeft
            anchors { bottom: details.top; right: parent.right; margins: d.spacing * 2; bottomMargin: 0 }
            Button {
                id: okButton
                text: qsTr("OK")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Ok
                onClicked: root.click(OriginalDialogs.StandardButton.Ok)
                visible: root.buttons & OriginalDialogs.StandardButton.Ok

            }
            Button {
                id: openButton
                text: qsTr("Open")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Open
                onClicked: root.click(OriginalDialogs.StandardButton.Open)
                visible: root.buttons & OriginalDialogs.StandardButton.Open
            }
            Button {
                id: saveButton
                text: qsTr("Save")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Save
                onClicked: root.click(OriginalDialogs.StandardButton.Save)
                visible: root.buttons & OriginalDialogs.StandardButton.Save
            }
            Button {
                id: saveAllButton
                text: qsTr("Save All")
                focus: root.defaultButton === OriginalDialogs.StandardButton.SaveAll
                onClicked: root.click(OriginalDialogs.StandardButton.SaveAll)
                visible: root.buttons & OriginalDialogs.StandardButton.SaveAll
            }
            Button {
                id: retryButton
                text: qsTr("Retry")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Retry
                onClicked: root.click(OriginalDialogs.StandardButton.Retry)
                visible: root.buttons & OriginalDialogs.StandardButton.Retry
            }
            Button {
                id: ignoreButton
                text: qsTr("Ignore")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Ignore
                onClicked: root.click(OriginalDialogs.StandardButton.Ignore)
                visible: root.buttons & OriginalDialogs.StandardButton.Ignore
            }
            Button {
                id: applyButton
                text: qsTr("Apply")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Apply
                onClicked: root.click(OriginalDialogs.StandardButton.Apply)
                visible: root.buttons & OriginalDialogs.StandardButton.Apply
            }
            Button {
                id: yesButton
                text: qsTr("Yes")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Yes
                onClicked: root.click(OriginalDialogs.StandardButton.Yes)
                visible: root.buttons & OriginalDialogs.StandardButton.Yes
            }
            Button {
                id: yesAllButton
                text: qsTr("Yes to All")
                focus: root.defaultButton === OriginalDialogs.StandardButton.YesToAll
                onClicked: root.click(OriginalDialogs.StandardButton.YesToAll)
                visible: root.buttons & OriginalDialogs.StandardButton.YesToAll
            }
            Button {
                id: noButton
                text: qsTr("No")
                focus: root.defaultButton === OriginalDialogs.StandardButton.No
                onClicked: root.click(OriginalDialogs.StandardButton.No)
                visible: root.buttons & OriginalDialogs.StandardButton.No
            }
            Button {
                id: noAllButton
                text: qsTr("No to All")
                focus: root.defaultButton === OriginalDialogs.StandardButton.NoToAll
                onClicked: root.click(OriginalDialogs.StandardButton.NoToAll)
                visible: root.buttons & OriginalDialogs.StandardButton.NoToAll
            }
            Button {
                id: discardButton
                text: qsTr("Discard")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Discard
                onClicked: root.click(OriginalDialogs.StandardButton.Discard)
                visible: root.buttons & OriginalDialogs.StandardButton.Discard
            }
            Button {
                id: resetButton
                text: qsTr("Reset")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Reset
                onClicked: root.click(OriginalDialogs.StandardButton.Reset)
                visible: root.buttons & OriginalDialogs.StandardButton.Reset
            }
            Button {
                id: restoreDefaultsButton
                text: qsTr("Restore Defaults")
                focus: root.defaultButton === OriginalDialogs.StandardButton.RestoreDefaults
                onClicked: root.click(OriginalDialogs.StandardButton.RestoreDefaults)
                visible: root.buttons & OriginalDialogs.StandardButton.RestoreDefaults
            }
            Button {
                id: cancelButton
                text: qsTr("Cancel")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Cancel
                onClicked: root.click(OriginalDialogs.StandardButton.Cancel)
                visible: root.buttons & OriginalDialogs.StandardButton.Cancel
            }
            Button {
                id: abortButton
                text: qsTr("Abort")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Abort
                onClicked: root.click(OriginalDialogs.StandardButton.Abort)
                visible: root.buttons & OriginalDialogs.StandardButton.Abort
            }
            Button {
                id: closeButton
                text: qsTr("Close")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Close
                onClicked: root.click(OriginalDialogs.StandardButton.Close)
                visible: root.buttons & OriginalDialogs.StandardButton.Close
            }
            Button {
                id: moreButton
                text: qsTr("Show Details...")
                onClicked: { content.state = (content.state === "" ? "expanded" : "")
                }
                visible: detailedText && detailedText.length > 0
            }
            Button {
                id: helpButton
                text: qsTr("Help")
                focus: root.defaultButton === OriginalDialogs.StandardButton.Help
                onClicked: root.click(OriginalDialogs.StandardButton.Help)
                visible: root.buttons & OriginalDialogs.StandardButton.Help
            }
        }

        Item {
            id: details
            width: parent.width
            implicitHeight: detailedText.implicitHeight + root.spacing
            height: 0
            clip: true
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right; margins: d.spacing * 2 }
            Flickable {
                id: flickable
                contentHeight: detailedText.height
                anchors.fill: parent
                anchors.topMargin: root.spacing
                anchors.bottomMargin: root.outerSpacing
                TextEdit {
                    id: detailedText
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
                PropertyChanges { target: root; anchors.fill: undefined }
                PropertyChanges { target: details; height: 120 }
                PropertyChanges { target: moreButton; text: qsTr("Hide Details") }
            }
        ]

        onStateChanged: d.resize()
    }

    Keys.onPressed: {
        if (!visible) {
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
                root.click(OriginalDialogs.StandardButton.Cancel)
                break

            case Qt.Key_Enter:
            case Qt.Key_Return:
                event.accepted = true
                root.click(OriginalDialogs.StandardButton.Ok)
                break
        }
    }
}
