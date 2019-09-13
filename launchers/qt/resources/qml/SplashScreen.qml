import QtQuick 2.3
import QtQuick.Controls 2.1

Item {
    anchors.fill: parent


    Image {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        mirror: true
        source: PathUtils.resourcePath("images/hifi_window@2x.png");
        transformOrigin: Item.Center
        rotation: 0
    }

    Image {
        anchors.centerIn: parent
        width: 225
        height: 205
        source: PathUtils.resourcePath("images/hifi_logo_large@2x.png");
    }
}
