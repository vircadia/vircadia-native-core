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
    id: dialog
    width: 480
    height: 720

    HifiConstants { id: hifi }
    property var sections: []
    property var showCategories: []
  
    function saveAll() {
        for (var i = 0; i < sections.length; ++i) {
            var section = sections[i];
            section.saveAll();
        }
    }

    function restoreAll() {
        for (var i = 0; i < sections.length; ++i) {
            var section = sections[i];
            section.restoreAll();
        }
    }
    
    Rectangle {
        id: main
        height: parent.height - 40
        anchors {
            top: parent.top
            bottom: footer.top
            left: parent.left
            right: parent.right
        }
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"
                
            }
            
            GradientStop {
                position: 1
                color: "#0f212e"
            }
        }
        Flickable {
            id: scrollView
            width: parent.width
            height: parent.height
            contentWidth: parent.width
            contentHeight: getSectionsHeight();
            Column {
                width: 480
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
                                sections[i].collapsable = false;
                                sections[i].expanded = true;
                            }
                        }
                        sections[0].isFirst = true;
                        sections[sections.length - 1].isLast = true;
                    }

                    scrollView.contentHeight = scrollView.getSectionsHeight();

                }
               

                Column {
                    id: prefControls
                    width: 480
                }
            }

            function getSectionsHeight() {
                var totalHeight = 0;
                for (var i = 0; i < sections.length; i++) {
                    totalHeight += sections[i].height + sections[i].getPreferencesHeight();
                }
                console.log(totalHeight);
                return totalHeight;
            }
        }
    }

    Rectangle {
        id: footer
        height: 40

        anchors {
            top: main.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"
                
            }
            
            GradientStop {
                position: 1
                color: "#0f212e"
            }
        }

        Row {
            anchors {
                top: parent,top
                right: parent.right
                rightMargin: hifi.dimensions.contentMargin.x
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
    
}
