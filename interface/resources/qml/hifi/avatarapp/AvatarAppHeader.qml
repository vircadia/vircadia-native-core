import Hifi 1.0 as Hifi
import QtQuick 2.5
import stylesUit 1.0

ShadowRectangle {
    id: header
    anchors.left: parent.left
    anchors.right: parent.right
    height: 60

    property alias pageTitle: title.text
    property alias avatarIconVisible: avatarIcon.visible
    property alias settingsButtonVisible: settingsButton.visible

    signal settingsClicked;

    AvatarAppStyle {
        id: style
    }

    color: style.colors.lightGrayBackground

    HiFiGlyphs {
        id: avatarIcon
        anchors.left: parent.left
        anchors.leftMargin: 23
        anchors.verticalCenter: header.verticalCenter

        size: 38
        text: "<"
    }

    // TextStyle6
    RalewaySemiBold {
        id: title
        size: 22;
        anchors.left: avatarIcon.visible ? avatarIcon.right : avatarIcon.left
        anchors.leftMargin: 4
        anchors.verticalCenter: avatarIcon.verticalCenter
        text: 'Avatar'
    }

    HiFiGlyphs {
        id: settingsButton
        anchors.right: parent.right
        anchors.rightMargin: 30
        anchors.verticalCenter: avatarIcon.verticalCenter
        text: "&"

        MouseArea {
            id: settingsMouseArea
            anchors.fill: parent
            onClicked: {
                settingsClicked();
            }
        }
    }
}
