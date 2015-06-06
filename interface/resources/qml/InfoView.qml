import Hifi 1.0 as Hifi
import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.3
import QtWebKit 3.0
import "controls"

VrDialog {
    id: root
    width: 800
    height: 800
    resizable: true
    
    Hifi.InfoView {
        id: infoView
        // Fille the client area
        anchors.fill: parent
        anchors.margins: parent.margins
        anchors.topMargin: parent.topMargin
 
        ScrollView {
            anchors.fill: parent
            WebView {
                objectName: "WebView"
                id: webview
                url: infoView.url
                anchors.fill: parent
            }
        }        
        
     }
}
