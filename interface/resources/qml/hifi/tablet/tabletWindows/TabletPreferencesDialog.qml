//
//  TabletPreferencesDialog.qml
//
//  Created by Dante Ruiz on 9 Feb 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0

import "."
import "./preferences"
import "../../../styles-uit"
import "../../../controls-uit" as HifiControls

Item {
    id: root
    width: 480
    height: 600

    HifiConstants { id: hifi }
    property var sections: []
    property var showCategories: []

    function saveAll() {
        
    }

    function restoreAll() {
        
    }

    ListView {

        Component {
            id: sectionBuilder
            Section {}
        }

        Component.onCompleted: {
            var categories = Preferences.categories;
            var i;

            // build a map of valid categories.
            var categoryMap = {};
            for (i = 0; i < categories.length; i++) {
                categoryMap[categories[i]] = true;
            }

            // create a section for each valid category in showCategories
            // NOTE: the sort order of items in the showCategories array is the same order in the dialog.
            for (i = 0; i < showCategories.length; i++) {
                if (categoryMap[showCategories[i]]) {
                    sections.push(sectionBuilder.createObject(prefControls, {name: showCategories[i]}));
                }
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
            width: 320
        }
    }
    
}
