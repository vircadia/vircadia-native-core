import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebKit 3.0
import "controls"

Dialog {
    title: "Browser Window"
    id: testDialog
    objectName: "Browser"
    width: 1280
    height: 720

    Item {
        id: clientArea
        // The client area
        anchors.fill: parent
        anchors.margins: parent.margins
        anchors.topMargin: parent.topMargin
 
        ScrollView {
            anchors.fill: parent
            WebView {
                id: webview
                url: "http://slashdot.org"
                anchors.fill: parent
            }
        }        
        
     }
}
