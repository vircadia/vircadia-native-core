import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../controls" as HifiControls
import "../windows"
import "preferences"

Window {
    id: root
    title: "Preferences"
    resizable: true
    destroyOnInvisible: true
    width: 500
    height: 577
    property var sections: []
    property var showCategories: []

    function saveAll() {
        for (var i = 0; i < sections.length; ++i) {
            var section = sections[i];
            section.saveAll();
        }
        destroy();
    }

    function restoreAll() {
        for (var i = 0; i < sections.length; ++i) {
            var section = sections[i];
            section.restoreAll();
        }
        destroy();
    }

    Rectangle {
        anchors.fill: parent
        clip: true
        color: "white"

        Component {
            id: sectionBuilder
            Section { }
        }

        Component.onCompleted: {
            var categories = Preferences.categories;
            var categoryMap;
            var i;
            if (showCategories && showCategories.length) {
                categoryMap = {};
                for (i = 0; i < showCategories.length; ++i) {
                    categoryMap[showCategories[i]] = true;
                }
            }

            for (i = 0; i < categories.length; ++i) {
                var category = categories[i];
                if (categoryMap && !categoryMap[category]) {
                    continue;
                }

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

            Button {
                text: "Cancel";
                onClicked: root.restoreAll();
            }

            Button {
                text: "Save all changes"
                onClicked: root.saveAll();
            }
        }
    }
}

