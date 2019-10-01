// root.qml

import QtQuick 2.3
import QtQuick.Controls 2.1
import HQLauncher 1.0
import "HFControls"

Item {
    id: root
    width: 627
    height: 540
    Loader {
        anchors.fill: parent
        id: loader
    }

    Component.onCompleted: {
        loader.source = "./SplashScreen.qml";
        LauncherState.updateSourceUrl.connect(function(url) {
            loader.source = url;
        });
    }


    function loadPage(url) {
        loader.source = url;
    }

    Text {
        id: stateInfo
        font.pixelSize: 12

        anchors.right: root.right
        anchors.bottom: root.bottom

        color: "#FFFFFF"
        text: LauncherState.uiState.toString() + " - " + LauncherState.applicationState
    }
}
