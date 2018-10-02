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

import "."
import "./preferences"
import "../../../styles-uit"
import "../../../controls-uit" as HifiControls

Item {
    id: dialog
    width: parent.width
    height: parent.height
    anchors.fill: parent

    HifiConstants { id: hifi }
    property var sections: []
    property var showCategories: []
    property var categoryProperties: ({})

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false
    property bool gotoPreviousApp: false

    property var tablet;
  
    function saveAll() {
        dialog.forceActiveFocus();  // Accept any text box edits in progress.

        for (var i = 0; i < sections.length; ++i) {
            var section = sections[i];
            section.saveAll();
        }

        if (HMD.active) {
            if (gotoPreviousApp) {
                tablet.returnToPreviousApp();
            } else {
                tablet.popFromStack();
            }
        } else {
            closeDialog();
        }
    }

    function restoreAll() {
        for (var i = 0; i < sections.length; ++i) {
            var section = sections[i];
            section.restoreAll();
        }

        if (HMD.active) {
            if (gotoPreviousApp) {
                tablet.returnToPreviousApp();
            } else {
                tablet.popFromStack();
            }
        } else {
            closeDialog();
        }
    }

    function closeDialog() {
        var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

        if (gotoPreviousApp) {
            tablet.returnToPreviousApp();
        } else {
            tablet.gotoHomeScreen();
        }
    }

    Rectangle {
        id: main
        anchors {
            top: parent.top
            bottom: footer.top
            left: parent.left
            right: parent.right
        }

        color: hifi.colors.baseGray

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
                            var properties = categoryProperties.hasOwnProperty(showCategories[i]) ? categoryProperties[showCategories[i]] : {};
                            sections.push(sectionBuilder.createObject(prefControls, {name: showCategories[i], sectionProperties: properties}));
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
                var bottomPadding = 170;
                return (totalHeight + bottomPadding);
            }
        }
    }

    MouseArea {
        // Defocuses the current control so that the HMD keyboard gets hidden.
        // Created under the footer so that the non-button part of the footer can defocus a control.
        id: mouseArea
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: keyboard.top
        }
        propagateComposedEvents: true
        acceptedButtons: Qt.AllButtons
        onPressed: {
            parent.forceActiveFocus();
            mouse.accepted = false;
        }
    }

    Rectangle {
        id: footer
        height: dialog.height * 0.15
        anchors.bottom: dialog.bottom

        anchors {
            bottom: keyboard.top
            left: parent.left
            right: parent.right
        }

        color: hifi.colors.baseGray

        Row {
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: hifi.dimensions.contentMargin.x
            }

            spacing: hifi.dimensions.contentSpacing.x
            HifiControls.Button {
                text: "Save changes"
                color: hifi.buttons.blue
                onClicked: dialog.saveAll()
            }

            HifiControls.Button {
                text: "Cancel"
                color: hifi.buttons.white
                onClicked: dialog.restoreAll()
            }
        }
    }

    HifiControls.Keyboard {
        id: keyboard
        raised: parent.keyboardEnabled && parent.keyboardRaised
        numeric: parent.punctuationMode
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
    }

    Component.onCompleted: {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        keyboardEnabled = HMD.active;
    }

    onKeyboardRaisedChanged: {
        if (keyboardEnabled && keyboardRaised) {
            var delta = mouseArea.mouseY - (dialog.height - footer.height - keyboard.raisedHeight -hifi.dimensions.controlLineHeight);
            if (delta > 0) {
                scrollView.contentY += delta;
            }
        }
    }
}
