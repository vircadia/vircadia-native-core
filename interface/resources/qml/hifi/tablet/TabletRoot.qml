import QtQuick 2.0
import Hifi 1.0

Item {
    id: tabletRoot
    objectName: "tabletRoot"
    property var eventBridge;

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
            // propogate eventBridge to WebEngineView
            if (loader.item.hasOwnProperty("eventBridge")) {
                loader.item.eventBridge = eventBridge;

                // Hook up callback for clara.io download from the marketplace.
                eventBridge.webEventReceived.connect(function (event) {
                    if (event.slice(0, 17) === "CLARA.IO DOWNLOAD") {
                        ApplicationInterface.addAssetToWorldFromURL(event.slice(18));
                    }
                });
            }
        }
    }

    width: 480
    height: 720
}
