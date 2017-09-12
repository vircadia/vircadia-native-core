import QtQuick 2.0
import Hifi 1.0
import QtQuick.Controls 1.4
import "../../dialogs"
import "../../controls"

Item {
    id: tabletRoot
    objectName: "tabletRoot"
    property string username: "Unknown user"
    property string usernameShort: "Unknown user"
    property var rootMenu;
    property var openModal: null;
    property var openMessage: null;
    property var openBrowser: null;
    property string subMenu: ""
    signal showDesktop();
    property bool shown: true
    property int currentApp: -1;
    property alias tabletApps: tabletApps 

    function setOption(value) {
        option = value;
    }

    Component { id: inputDialogBuilder; TabletQueryDialog { } }
    function inputDialog(properties) {
        openModal = inputDialogBuilder.createObject(tabletRoot, properties);
        return openModal;
    }
    Component { id: messageBoxBuilder; TabletMessageBox { } }
    function messageBox(properties) {
        openMessage  = messageBoxBuilder.createObject(tabletRoot, properties);
        return openMessage;
    }

    Component { id: customInputDialogBuilder; TabletCustomQueryDialog { } }
    function customInputDialog(properties) {
        openModal = customInputDialogBuilder.createObject(tabletRoot, properties);
        return openModal;
    }

    Component { id: fileDialogBuilder; TabletFileDialog { } }
    function fileDialog(properties) {
        openModal = fileDialogBuilder.createObject(tabletRoot, properties);
        return openModal; 
    }

    Component { id: assetDialogBuilder; TabletAssetDialog { } }
    function assetDialog(properties) {
        openModal = assetDialogBuilder.createObject(tabletRoot, properties);
        return openModal;
    }

    function setMenuProperties(rootMenu, subMenu) {
        tabletRoot.rootMenu = rootMenu;
        tabletRoot.subMenu = subMenu;
    }

    function isDialogOpen() {
        if (openMessage !== null || openModal !== null) {
            return true;
        }

        return false;
    }

    function loadSource(url) {
        tabletApps.clear();
        loader.source = "";  // make sure we load the qml fresh each time.
        loader.source = url;
        tabletApps.append({"appUrl": url, "isWebUrl": false, "scriptUrl": "", "appWebUrl": ""});
    }

    function loadQMLOnTop(url) {
        tabletApps.append({"appUrl": url, "isWebUrl": false, "scriptUrl": "", "appWebUrl": ""});
        loader.source = "";
        loader.source = tabletApps.get(currentApp).appUrl;
        if (loader.item.hasOwnProperty("gotoPreviousApp")) {
            loader.item.gotoPreviousApp = true;
        }
    }

    function loadWebOnTop(url, injectJavaScriptUrl) {
        tabletApps.append({"appUrl": loader.source, "isWebUrl": true, "scriptUrl": injectJavaScriptUrl, "appWebUrl": url});
        loader.item.url = tabletApps.get(currentApp).appWebUrl;
        loader.item.scriptUrl = tabletApps.get(currentApp).scriptUrl;
        if (loader.item.hasOwnProperty("gotoPreviousApp")) {
            loader.item.gotoPreviousApp = true;
        }
    }

    function loadWebBase() {
        loader.source = "";
        loader.source = "TabletWebView.qml";
    }

    function loadTabletWebBase() {
        loader.source = "";
        loader.source = "./BlocksWebView.qml";
    }
        
    function returnToPreviousApp() {
        tabletApps.remove(currentApp);
        var isWebPage = tabletApps.get(currentApp).isWebUrl;
        if (isWebPage) {
            var webUrl = tabletApps.get(currentApp).appWebUrl;
            var scriptUrl = tabletApps.get(currentApp).scriptUrl;
            loadSource("TabletWebView.qml");
            loadWebUrl(webUrl, scriptUrl);
        } else {
            loader.source = tabletApps.get(currentApp).appUrl;
        }
    }

    function openBrowserWindow(request, profile) {
        var component = Qt.createComponent("../../controls/TabletWebView.qml");
        var newWindow = component.createObject(tabletRoot);
        newWindow.remove = true;
        newWindow.profile = profile;
        request.openIn(newWindow.webView);
        tabletRoot.openBrowser = newWindow;
    }

    function loadWebUrl(url, injectedJavaScriptUrl) {
        tabletApps.clear();
        loader.item.url = url;
        loader.item.scriptURL = injectedJavaScriptUrl;
        tabletApps.append({"appUrl": "TabletWebView.qml", "isWebUrl": true, "scriptUrl": injectedJavaScriptUrl, "appWebUrl": url});
        if (loader.item.hasOwnProperty("closeButtonVisible")) {
            loader.item.closeButtonVisible = false;
        }
    }

    // used to send a message from qml to interface script.
    signal sendToScript(var message);

    // used to receive messages from interface script
    function fromScript(message) {
        if (loader.item.hasOwnProperty("fromScript")) {
            loader.item.fromScript(message);
        }
    }

    SoundEffect {
        id: buttonClickSound
        volume: 0.1
        source: "../../../sounds/Gamemaster-Audio-button-click.wav"
    }

    function playButtonClickSound() {
        // Because of the asynchronous nature of initalization, it is possible for this function to be
        // called before the C++ has set the globalPosition context variable.
        if (typeof globalPosition !== 'undefined') {
            buttonClickSound.play(globalPosition);
        }
    }

    function setUsername(newUsername) {
        username = newUsername;
        usernameShort = newUsername.substring(0, 8);

        if (newUsername.length > 8) {
            usernameShort = usernameShort + "..."
        }
    }

    ListModel {
        id: tabletApps
        onCountChanged: {
            currentApp = count - 1
        }
    }

    Loader {
        id: loader
        objectName: "loader"
        asynchronous: false

        width: parent.width
        height: parent.height

        // Hook up callback for clara.io download from the marketplace.
        Connections {
            id: eventBridgeConnection
            target: eventBridge
            onWebEventReceived: {
                if (message.slice(0, 17) === "CLARA.IO DOWNLOAD") {
                    ApplicationInterface.addAssetToWorldFromURL(message.slice(18));
                }
            }
        }

        onLoaded: {
            if (loader.item.hasOwnProperty("sendToScript")) {
                loader.item.sendToScript.connect(tabletRoot.sendToScript);
            }
            if (loader.item.hasOwnProperty("setRootMenu")) {
                loader.item.setRootMenu(tabletRoot.rootMenu, tabletRoot.subMenu);
            }
            loader.item.forceActiveFocus();

            if (openModal) {
                openModal.canceled();
                openModal.destroy();
                openModal = null;
            }

            if (openBrowser) {
                openBrowser.destroy();
                openBrowser = null;
            }
        }
    }

    width: 480
    height: 706

    function setShown(value) {
        if (value === true) {
            HMD.openTablet()
        } else {
            HMD.closeTablet()
        }
    }
}
