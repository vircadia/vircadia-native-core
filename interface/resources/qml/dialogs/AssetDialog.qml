//
//  AssetDialog.qml
//
//  Created by David Rowe on 18 Apr 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

import "../controls-uit"
import "../styles-uit"
import "../windows"

ModalWindow {
    id: root
    resizable: true
    implicitWidth: 480
    implicitHeight: 360 + (fileDialogItem.keyboardEnabled && fileDialogItem.keyboardRaised ? keyboard.raisedHeight + hifi.dimensions.contentSpacing.y : 0)

    minSize: Qt.vector2d(360, 240)
    draggable: true

    HifiConstants { id: hifi }

    Settings {
        category: "FileDialog"
        property alias width: root.width
        property alias height: root.height
        property alias x: root.x
        property alias y: root.y
    }

    // Set from OffscreenUi::assetDialog()
    //property alias caption: root.title;
    property string caption
    //property alias dir: fileTableModel.folder;
    property string dir
    //property alias filter: selectionType.filtersString;
    property string filter
    property int options  // Unused

    // TODO: Other properties

    signal selectedAsset(var asset);
    signal canceled();

    Component.onCompleted: {
        // TODO: Required?
    }

    Item {
        id: assetDialogItem
        clip: true
        width: pane.width
        height: pane.height
        anchors.margins: 0

        Action {
            id: okAction
            // TODO
            /*
            text: currentSelection.text ? (root.selectDirectory && fileTableView.currentRow === -1 ? "Choose" : (root.saveDialog ? "Save" : "Open")) : "Open"
            enabled: currentSelection.text || !root.selectDirectory && d.currentSelectionIsFolder ? true : false
            onTriggered: {
                if (!root.selectDirectory && !d.currentSelectionIsFolder
                        || root.selectDirectory && fileTableView.currentRow === -1) {
                    okActionTimer.start();
                } else {
                    fileTableView.navigateToCurrentRow();
                }
            }
            */
            text: "Choose"

            onTriggered: {
                selectedAsset("/recordings/20170417-233012.hfr");
                root.destroy();  // TODO: root.shown = false?
            }
        }

        Action {
            id: cancelAction
            text: "Cancel"
            onTriggered: {
                canceled();
                root.shown = false;
            }
        }

        Row {
            id: buttonRow
            anchors {
                right: parent.right
                bottom: parent.bottom
            }
            spacing: hifi.dimensions.contentSpacing.y

            Button {
                id: openButton
                color: hifi.buttons.blue
                action: okAction
                Keys.onReturnPressed: okAction.trigger()
                KeyNavigation.up: selectionType
                KeyNavigation.left: selectionType
                KeyNavigation.right: cancelButton
            }

            Button {
                id: cancelButton
                action: cancelAction
                KeyNavigation.up: selectionType
                KeyNavigation.left: openButton
                KeyNavigation.right: fileTableView.contentItem
                Keys.onReturnPressed: { canceled(); root.enabled = false }
            }
        }

    }

    Keys.onPressed: {
        switch (event.key) {
        case Qt.Key_Backspace:
            event.accepted = d.navigateUp();
            break;

        case Qt.Key_Home:
            event.accepted = d.navigateHome();
            break;

        }
    }
}
