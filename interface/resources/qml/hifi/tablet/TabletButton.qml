import QtQuick 2.0
import "../../styles-uit/"

Rectangle {
    id: tabletButton
    width: 140
    height: 140
    color: "#1794c3"

    Image {
        id: icon
        x: 40
        y: 30
        width: 60
        height: 60
        source: "../../../icons/edit-icon.svg"
    }

    RalewaySemiBold {
        id: text
        x: 50
        y: 96
        color: "#ffffff"
        text: qsTr("EDIT")
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 18
    }
}
