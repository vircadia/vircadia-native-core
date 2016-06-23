//
//  PreferencesDialog.qml
//
//  Created by Bradley Austin Davis on 24 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4

import "../controls-uit" as HifiControls
import "../styles-uit"
import "../windows"
import "preferences"

ScrollingWindow {
    id: root
    title: "Preferences"
    resizable: true
    destroyOnHidden: true
    width: 500
    height: 577
    property var sections: []
    property var showCategories: []
    minSize: Qt.vector2d(400, 500)

    HifiConstants { id: hifi }

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

    Column {
        width: pane.contentWidth

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

            if (sections.length) {
                // Default sections to expanded/collapsed as appropriate for dialog.
                if (sections.length === 1) {
                    sections[0].collapsable = false
                    sections[0].expanded = true
                } else {
                    for (i = 0; i < sections.length; i++) {
                        sections[i].collapsable = true;
                        sections[i].expanded = true;
                    }
                }
                sections[0].isFirst = true;
                sections[sections.length - 1].isLast = true;
            }
        }

        Column {
            id: prefControls
            width: pane.contentWidth
        }
    }

    footer: Row {
        anchors {
            right: parent.right;
            rightMargin: hifi.dimensions.contentMargin.x
            verticalCenter: parent.verticalCenter
        }
        spacing: hifi.dimensions.contentSpacing.x

        HifiControls.Button {
            text: "Save changes"
            color: hifi.buttons.blue
            onClicked: root.saveAll()
        }

        HifiControls.Button {
            text: "Cancel"
            color: hifi.buttons.white
            onClicked: root.restoreAll()
        }
    }
}
