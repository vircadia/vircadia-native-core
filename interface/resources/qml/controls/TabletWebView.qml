import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.2
import QtWebChannel 1.0
import "../controls-uit" as HiFiControls
import "../styles" as HifiStyles
import "../styles-uit"
import HFWebEngineProfile 1.0
import HFTabletWebEngineProfile 1.0
import "../"
Item {
    id: web
    width: parent.width
    height: parent.height
    property var parentStackItem: null
    property int headerHeight: 38
    property string url
    property string address: url //for compatibility
    property string scriptURL
    property alias eventBridge: eventBridgeWrapper.eventBridge
    property bool keyboardEnabled: HMD.active
    property bool keyboardRaised: false
    property bool punctuationMode: false
    property bool isDesktop: false
    property WebEngineView view: loader.currentView

    
    property int currentPage: -1 // used as a model for repeater
    property alias pagesModel: pagesModel

    Row {
        id: buttons
        HifiConstants { id: hifi }
        HifiStyles.HifiConstants { id: hifistyles }
        height: headerHeight
        spacing: 4
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 8
        HiFiGlyphs {
            id: back;
            enabled: currentPage >= 0
            text: hifi.glyphs.backward
            color: enabled ? hifistyles.colors.text : hifistyles.colors.disabledText
            size: 48
            MouseArea { anchors.fill: parent;  onClicked: goBack() }
        }
        
        HiFiGlyphs {
            id: forward;
            enabled: currentPage < pagesModel.count - 1
            text: hifi.glyphs.forward
            color: enabled ? hifistyles.colors.text : hifistyles.colors.disabledText
            size: 48
            MouseArea { anchors.fill: parent;  onClicked: goForward() }
        }
        
        HiFiGlyphs {
            id: reload;
            enabled: view != null;
            text: (view !== null && view.loading) ? hifi.glyphs.close : hifi.glyphs.reload
            color: enabled ? hifistyles.colors.text : hifistyles.colors.disabledText
            size: 48
            MouseArea { anchors.fill: parent;  onClicked: reloadPage(); }
        }
        
    }

    TextField {
        id: addressBar
        height: 30
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.left: buttons.right
        anchors.leftMargin: 0
        anchors.verticalCenter: buttons.verticalCenter
        focus: true
        text: address
        Component.onCompleted: ScriptDiscoveryService.scriptsModelFilter.filterRegExp = new RegExp("^.*$", "i")

        Keys.onPressed: {
            switch (event.key) {
                case Qt.Key_Enter:
                case Qt.Key_Return: 
                    event.accepted = true;
                    if (text.indexOf("http") != 0) {
                        text = "http://" + text;
                     }
                    //root.hidePermissionsBar();
                    web.keyboardRaised = false;
                    gotoPage(text);
                    break;

                
            }
        }
    }

    ListModel {
        id: pagesModel
        onCountChanged: {
            currentPage = count - 1
        }
    }
        
    function goBack() {
        if (currentPage > 0) {
            currentPage--;
        } else if (parentStackItem) {
            parentStackItem.pop();
        }
    }

    function goForward() {
        if (currentPage < pagesModel.count - 1) {
            currentPage++;
        }
    }

    function gotoPage(url) {
        urlAppend(url)
    }

    function reloadPage() {
        view.reloadAndBypassCache()
        view.setActiveFocusOnPress(true);
        view.setEnabled(true);
    }

    function urlAppend(url) {
        var lurl = decodeURIComponent(url)
        if (lurl[lurl.length - 1] !== "/")
            lurl = lurl + "/"
        if (currentPage === -1 || pagesModel.get(currentPage).webUrl !== lurl) {
            pagesModel.append({webUrl: lurl})
        }
    }

    onCurrentPageChanged: {
        if (currentPage >= 0 && currentPage < pagesModel.count && loader.item !== null) {
            loader.item.url = pagesModel.get(currentPage).webUrl
            web.url = loader.item.url
            web.address = loader.item.url
        }
    }

    onUrlChanged: {
        gotoPage(url)
    }

    QtObject {
        id: eventBridgeWrapper
        WebChannel.id: "eventBridgeWrapper"
        property var eventBridge;
    }

    Loader {
        id: loader

        property WebEngineView currentView: null

        width: parent.width
        height: parent.height - web.headerHeight
        asynchronous: true
        anchors.top: buttons.bottom
        active: false
        source: "../TabletBrowser.qml"
        onStatusChanged: {
            if (loader.status === Loader.Ready) {
                currentView = item.webView
                item.webView.userScriptUrl = web.scriptURL
                if (currentPage >= 0) {
                    //we got something to load already
                    item.url = pagesModel.get(currentPage).webUrl
                    web.address = loader.item.url
                }
            }
        }
    }

    Component.onCompleted: {
        web.isDesktop = (typeof desktop !== "undefined");
        address = url;
        loader.active = true
    }

     Keys.onPressed: {
        switch(event.key) {
            case Qt.Key_L:
                if (event.modifiers == Qt.ControlModifier) {
                    event.accepted = true
                    addressBar.selectAll()
                    addressBar.forceActiveFocus()
                }
                break;
        }
    }
}
