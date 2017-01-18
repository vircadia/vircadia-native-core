import QtQuick 2.0
import Hifi 1.0
import "../../desktop"
FocusScope {
    id: tabletRoot
    objectName: "tabletRoot"
    property var eventBridge;
    property bool desktopRoot: true

    signal showDesktop();
    
    function loadSource(url) {
        loader.source = url;
    }

    function loadWebUrl(url, injectedJavaScriptUrl) {
        loader.item.url = url;
        loader.item.scriptURL = injectedJavaScriptUrl;
    }

    SoundEffect {
        id: buttonClickSound
        source: "../../../sounds/button-click.wav"
    }

    function playButtonClickSound() {
        buttonClickSound.play(globalPosition);
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
            loader.item.forceActiveFocus();
            offscreenFlags.navigationFocused = true;
        }
    }
    Component.onCompleted: {
        offscreenWindow.requestActivate();
        offscreenWindow.forceActiveFocus();
    }
    Component.onDestruction: { offscreenFlags.navigationFocused = false; }

    width: 480
    height: 720
}
