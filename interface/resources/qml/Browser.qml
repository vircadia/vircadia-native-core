import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Styles 1.3
import QtWebKit 3.0

CustomDialog {
    title: "Test Dlg"
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


/*

// This is the behavior, and it applies a NumberAnimation to any attempt to set the x property

MouseArea {
    anchors.fill: parent
}
*/
