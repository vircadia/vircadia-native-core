import QtQuick 2.5
import Hifi 1.0 as Hifi

import "controls"
import "windows" as Windows

Windows.Window {
    id: root
    width: 800
    height: 800
    resizable: true
    
    Hifi.InfoView {
        id: infoView
        // Fill the client area
        anchors.fill: parent
 
        WebView {
            id: webview
            objectName: "WebView"
            anchors.fill: parent
            url: infoView.url
        }
     }
}
