import QtQuick 2.7
import stylesUit 1.0 as HifiStylesUit
import controlsUit 1.0 as HifiControlsUit

Rectangle {
    color: "black"
    height: 480
    width: 720

    readonly property string avatarTheftEntityName: "hifi-avatar-theft-banner";

    HifiStylesUit.RalewayRegular {
        id: displayMessage;
        text: "The avatar you're using is registered to another user.";
        size: 20;
        color: "white";
        anchors.top: parent.top;
        anchors.topMargin: 0.1 * parent.height;
        anchors.left: parent.left;
        anchors.leftMargin: (parent.width - width) / 2
        
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    HifiStylesUit.ShortcutText {
        id: gotoShortcut; 
        anchors.top: parent.top;
        anchors.topMargin: 0.4 * parent.height;
        anchors.left: parent.left;
        anchors.leftMargin: (parent.width - width) / 2
        font.family: "Raleway"
        font.pixelSize: 20

        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        linkColor: hifi.colors.blueAccent
        text: "<a href='https://fake.link'>Click here to change your avatar and dismiss this banner.</a>"
        onLinkActivated: {
           AddressManager.handleLookupString("hifi://BodyMart");
        }
    }

    HifiStylesUit.RalewayRegular {
        id: contactText;
        text: "If you own this avatar, please contact"
        size: 20;
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0.15 * parent.height
        anchors.left: parent.left;
        anchors.leftMargin: (parent.width - width) / 2
    }

    HifiStylesUit.ShortcutText {
        id: gotoShortcut; 
        anchors.top: contactText.bottom;
        anchors.left: parent.left;
        anchors.leftMargin: (parent.width - width) / 2
        font.family: "Raleway"
        font.pixelSize: 20

        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        linkColor: hifi.colors.blueAccent
        text: "<a href='mailto:support@highfidelity.com'>Click here to change your avatar and dismiss this banner.</a>"
        onLinkActivated: {
           HiFiAbout.openUrl("mailto:support@highfidelity.com");
        }
    }
}
