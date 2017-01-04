import QtQuick 2.0

Item {
    id: tabletRoot
    objectName: "tabletRoot"
    property var eventBridge;

    function loadSource(url) {
        loader.source = url;
    }

    function loadWebUrl(url) {
        loader.item.url = url;
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
            }
        }
    }

    width: 480
    height: 720
}
