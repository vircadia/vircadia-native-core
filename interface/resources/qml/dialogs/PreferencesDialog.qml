//
//  PreferencesDialog.qml
//
//  Created by Bradley Austin Davis on 24 Jan 2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../controls" as HifiControls
import "../styles-uit"
import "../windows-uit"
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
                sections[0].expanded = true;
                if (sections.length === 1) {
                    sections[0].collapsable = false
                }
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

