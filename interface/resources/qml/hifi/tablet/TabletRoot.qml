import QtQuick 2.0

Item {
    id: tabletRoot
    objectName: "tabletRoot"

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
    }

    width: 480
    height: 720
}
