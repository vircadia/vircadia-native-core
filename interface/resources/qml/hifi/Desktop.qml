import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.1;

import "../desktop"
import ".."
import "."

Desktop {
    id: desktop

    MouseArea {
        id: hoverWatch
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
        scrollGestureEnabled: false // we don't need/want these
        onEntered: ApplicationCompositor.reticleOverDesktop = true
        onExited: ApplicationCompositor.reticleOverDesktop = false
        acceptedButtons: Qt.NoButton
    }

    Component.onCompleted: {
        WebEngine.settings.javascriptCanOpenWindows = true;
        WebEngine.settings.javascriptCanAccessClipboard = false;
        WebEngine.settings.spatialNavigationEnabled = false;
        WebEngine.settings.localContentCanAccessRemoteUrls = true;
    }

    // The tool window, one instance
    property alias toolWindow: toolWindow
    ToolWindow { id: toolWindow }

    property var browserProfile: WebEngineProfile {
        id: webviewProfile
        httpUserAgent: "Chrome/48.0 (HighFidelityInterface)"
        storageName: "qmlWebEngine"
    }

    Action {
        text: "Open Browser"
        shortcut: "Ctrl+B"
        onTriggered: {
            console.log("Open browser");
            browserBuilder.createObject(desktop);
        }
        property var browserBuilder: Component {
            Browser{}
        }
    }


    ToggleHudButton {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 32
        anchors.horizontalCenter: parent.horizontalCenter
    }
}


