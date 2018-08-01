import Hifi 1.0 as Hifi
import QtQuick 2.5
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls

Rectangle {
    id: root;
    visible: false;
    anchors.fill: parent;
    color: Qt.rgba(0, 0, 0, 0.5);
    z: 999;

    property string titleText: ''
    property string bodyText: ''
    property alias inputText: input;
    property alias dialogButtons: buttons

    property string imageSource: null
    onImageSourceChanged: {
        console.debug('imageSource = ', imageSource)
    }

    property string button1color: hifi.buttons.noneBorderlessGray;
    property string button1text: ''
    property string button2color: hifi.buttons.blue;
    property string button2text: ''

    property var onButton2Clicked;
    property var onButton1Clicked;
    property var onLinkClicked;

    function open() {
        visible = true;
    }

    function close() {
        visible = false;

        dialogButtons.yesButton.fontCapitalization = Font.AllUppercase;
        onButton1Clicked = null;
        onButton2Clicked = null;
        button1text = '';
        button2text = '';
        imageSource = null;
        inputText.visible = false;
        inputText.placeholderText = '';
        inputText.text = '';
    }

    HifiConstants {
        id: hifi
    }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
        hoverEnabled: true;
    }

    Rectangle {
        id: mainContainer;
        width: Math.max(parent.width * 0.8, 400)
        property int margin: 30;

        height: childrenRect.height + margin * 2
        onHeightChanged: {
            console.debug('mainContainer: height = ', height)
        }

        anchors.centerIn: parent

        color: "white"

        // TextStyle1
        RalewaySemiBold {
            id: title
            size: 24;
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 30
            anchors.leftMargin: 30
            anchors.rightMargin: 30

            text: root.titleText
            elide: Qt.ElideRight
        }

        Column {
            id: contentContainer
            spacing: 15

            anchors.top: title.bottom
            anchors.topMargin: 10
            anchors.left: parent.left;
            anchors.leftMargin: 30;
            anchors.right: parent.right;
            anchors.rightMargin: 30;

            InputTextStyle4 {
                id: input
                visible: false
                height: visible ? implicitHeight : 0

                anchors.left: parent.left;
                anchors.right: parent.right;
            }

            // TextStyle3
            RalewayRegular {
                id: body

                AvatarAppStyle {
                    id: style
                }

                size: 18
                text: root.bodyText;
                linkColor: style.colors.blueHighlight
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: paintedHeight;
                verticalAlignment: Text.AlignTop;
                wrapMode: Text.WordWrap;

                onLinkActivated: {
                    if(onLinkClicked)
                        onLinkClicked(link);
                }
            }

            Image {
                id: image
                Binding on height {
                    when: imageSource === null
                    value: 0
                }

                anchors.left: parent.left;
                anchors.right: parent.right;

                Binding on source {
                    when: imageSource !== null
                    value: imageSource
                }

                visible: imageSource !== null ? true : false
            }
        }

        DialogButtons {
            id: buttons

            anchors.top: contentContainer.bottom
            anchors.topMargin: 30
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 30

            yesButton.enabled: !input.visible || input.text.length !== 0
            yesText: root.button2text
            noText: root.button1text

            onYesClicked: function() {
                if(onButton2Clicked) {
                    onButton2Clicked();
                } else {
                    root.close();
                }
            }

            onNoClicked: function() {
                if(onButton1Clicked) {
                    onButton1Clicked();
                } else {
                    root.close();
                }
            }
        }

    }
}
