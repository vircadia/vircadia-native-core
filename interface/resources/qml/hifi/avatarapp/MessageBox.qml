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

    function open() {
        visible = true;
    }

    function close() {
        visible = false;

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

        TextStyle1 {
            id: title
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 30
            anchors.leftMargin: 30
            anchors.rightMargin: 30

            text: root.titleText
        }

        Item {
            id: contentContainer
            width: parent.width - 60
            height: childrenRect.height
            onHeightChanged: {
                console.debug('contentContainer: height = ', height,
                              'image.height = ', image.height,
                              'body.height = ', body.height
                              )
            }

            anchors.top: title.bottom
            anchors.topMargin: 10
            anchors.left: parent.left;
            anchors.leftMargin: 30;
            anchors.right: parent.right;
            anchors.rightMargin: 30;

            TextStyle3 {
                id: body;
                text: root.bodyText;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: paintedHeight;
                verticalAlignment: Text.AlignTop;
                wrapMode: Text.WordWrap;
            }

            Image {
                id: image
                Binding on height {
                    when: imageSource === null
                    value: 0
                }

                anchors.top: body.bottom
                anchors.topMargin: imageSource === null ? 0 : 30
                anchors.left: parent.left;
                anchors.right: parent.right;

                Binding on source {
                    when: imageSource !== null
                    value: imageSource
                }

                visible: imageSource !== null ? true : false
            }

            InputTextStyle4 {
                id: input
                visible: false
                height: visible ? implicitHeight : 0

                anchors.top: imageSource !== null ? image.bottom : body.bottom
                anchors.left: parent.left;
                anchors.right: parent.right;
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
