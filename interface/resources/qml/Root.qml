import QtQuick 2.3
import "componentCreation.js" as Creator


Item {
    id: root
    width: 1280
    height: 720

    function loadChild(url) {
        Creator.createObject(root, url)
    }
}

