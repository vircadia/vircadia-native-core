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
        return customInputDialogBuilder.createObject(tabletRoot, properties);
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
        loader.source = "";  // make sure we load the qml fresh each time.
        loader.source = url;
    }

    function loadWebUrl(url, injectedJavaScriptUrl) {
        loader.item.url = url;
        loader.item.scriptURL = injectedJavaScriptUrl;
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
