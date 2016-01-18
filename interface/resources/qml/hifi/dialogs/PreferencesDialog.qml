import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../../controls" as HifiControls
import "../../windows"
import "./preferences"

Window {
    id: root
    objectName: "Preferences"
    title: "Preferences"
    resizable: true
    destroyOnInvisible: true
    width: 500
    height: 577
    property var sections: []

    function saveAll() {
        for (var i = 0; i < sections.length; ++i) {
            var section = sections[i];
            section.saveAll();
        }
        destroy();
    }

    Rectangle {
        anchors.fill: parent
        clip: true
        color: "white"

        Settings {
            category: "Overlay.Preferences"
            property alias x: root.x
            property alias y: root.y
        }

        Component {
            id: sectionBuilder
            Section { }
        }

        Component.onCompleted: {
            Preferences.loadAll();
            var categories = Preferences.categories;
            for (var i = 0; i < categories.length; ++i) {
                var category = categories[i];
                sections.push(sectionBuilder.createObject(prefControls, { name: category }));
            }
        }

        Flickable  {
            id: flickable
            clip: true
            interactive: true
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: dialogButtons.top
            anchors.bottomMargin: 8
            contentHeight: prefControls.height
            contentWidth: parent.width

            Column {
                id: prefControls
                anchors.left: parent.left
                anchors.right: parent.right
            }
        }
        Row {
            id: dialogButtons
            anchors { bottom: parent.bottom; right: parent.right; margins: 8 }
            Button { text: "Cancel"; onClicked: root.destroy(); }
            Button {
                text: "Save all changes"
                onClicked: root.saveAll();
            }
        }
    }
}

