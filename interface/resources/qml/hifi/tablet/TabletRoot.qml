import QtQuick 2.0
import Hifi 1.0
import QtQuick.Controls 1.4
import "../../dialogs"

Item {
    id: tabletRoot
    objectName: "tabletRoot"
    property string username: "Unknown user"
    property var eventBridge;
    property var rootMenu;
    property var openModal: null;
    property var openMessage: null;
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
        //tabletApps.append({appUrl: url, isWebUrl: false, scriptUrl: ""});
    }

    function loadQMLOnTop(url) {
        tabletApps.append({"appUrl": url, "isWebUrl": false, "scriptUrl": ""});
        loader.source = "";
        loader.source = tabletApps.get(currentApp).appUrl;
    }

    function loadWebOnTop(url, injectJavaScriptUrl) {
        tabletApps.append({"appUrl": loader.source, "isWebUrl": true, "scriptUrl": injectJavaScriptUrl, "appWebUrl": url});
        loader.item.url = tabletApps.get(currentApp).appWebUrl;
        loader.item.scriptUrl = tabletApps.get(currentApp).scriptUrl;
    }
        
    function returnToPreviousApp() {
        loader.source = "";
        tabletApps.remove(currentApp);
        var isWebPage = tabletApps.get(currentApp).isWebUrl;
        console.log(isWebPage);
        if (isWebPage) {
            console.log(tabletApps.get(currentApp).appUrl);
            loader.source = tabletApps.get(currentApp).appUrl;
            console.log(tabletApps.get(currentApp).appWebUrl);
            console.log(tabletApps.get(currentApp).scriptUrl);
            loader.item.url = tabletApps.get(currentApp).appWebUrl;
            loader.item.scriptUrl = tabletApps.get(currentApp).scriptUrl;
        } else {
            console.log(tabletApps.get(currentApp).appUrl);
            loader.source = tabletApps.get(currentApp).appUrl;
        }
    }

    function loadWebUrl(url, injectedJavaScriptUrl) {
        loader.item.url = url;
        loader.item.scriptURL = injectedJavaScriptUrl;
        var appUrl = loader.source;
        tabletApps.append({"appUrl": "Tablet.qml", "isWebUrl": true, "scriptUrl": injectedJavaScriptUrl, "appWebUrl": url});
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

    function toggleMicEnabled() {
        ApplicationInterface.toggleMuteAudio();
    }

    function setUsername(newUsername) {
        username = newUsername;
    }

    ListModel {
        id: tabletApps
        onCountChanged: {
            currentApp = count;
            console.log("[DR] -> the currnet count: " + currentApp);
        }
    }

    Loader {
        id: loader
        objectName: "loader"
        asynchronous: false

        
        width: parent.width
        height: parent.height

        onLoaded: {
            if (loader.item.hasOwnProperty("eventBridge")) {
                loader.item.eventBridge = eventBridge;

                // Hook up callback for clara.io download from the marketplace.
                eventBridge.webEventReceived.connect(function (event) {
                    if (event.slice(0, 17) === "CLARA.IO DOWNLOAD") {
                        ApplicationInterface.addAssetToWorldFromURL(event.slice(18));
                    }
                });
            }
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
        }
    }

    width: 480
    height: 706

    function setShown(value) {}
}
