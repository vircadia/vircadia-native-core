// root.qml

import QtQuick 2.3
import QtQuick.Controls 2.1
import HQLauncher 1.0
import "HFControls"
Image {
    id: root
    width: 515
    height: 450
    source: "../images/hifi_window@2x.png"

    Loader {
        anchors.centerIn: parent
        anchors.fill: parent
        id: loader
    }

    Component.onCompleted: {
        loader.source = LauncherState.getCurrentUISource();
        LauncherState.updateSourceUrl.connect(function(url) {
            loader.source = url;
        });
    }
}
