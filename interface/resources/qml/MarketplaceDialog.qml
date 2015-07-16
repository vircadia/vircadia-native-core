import Hifi 1.0
import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.3
import QtWebKit 3.0
import "controls"

VrDialog {
    title: "Test Dlg"
    id: testDialog
    objectName: "Browser"
    width: 720
    height: 720
    resizable: true
    
    MarketplaceDialog {
        id: marketplaceDialog
    }

    Item {
        id: clientArea
        // The client area
        anchors.fill: parent
        anchors.margins: parent.margins
        anchors.topMargin: parent.topMargin
        
 
        ScrollView {
            anchors.fill: parent
            WebView {
                objectName: "WebView"
                id: webview
                url: "https://metaverse.highfidelity.com/marketplace"
                anchors.fill: parent
                onNavigationRequested: {
                    console.log(request.url)
                    if (!marketplaceDialog.navigationRequested(request.url)) {
                        console.log("Application absorbed the request")
                        request.action = WebView.IgnoreRequest;
                        return;
                    } 
                    console.log("Application passed on the request")
                    request.action = WebView.AcceptRequest;
                    return;
                }
            }
        }        
        
     }
}
