//
//  RunningScripts.qml
//
//  Created by Bradley Austin Davis on 12 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../../styles-uit"
import "../../controls-uit" as HifiControls
import "../../windows"
import "../"

Rectangle {
    id: root
    objectName: "RunningScripts"
    property string title: "Running Scripts"
    HifiConstants { id: hifi }
    signal sendToScript(var message);
    property var scripts: ScriptDiscoveryService;
    property var scriptsModel: scripts.scriptsModelFilter
    property var runningScriptsModel: ListModel { }
    property bool developerMenuEnabled: false
    property bool isHMD: false

    color: hifi.colors.baseGray


    LetterboxMessage {
        id: letterBoxMessage
        z: 999
        visible: false
    }

    function letterBox(glyph, text, message) {
        letterBoxMessage.headerGlyph = glyph;
        letterBoxMessage.headerText = text;
        letterBoxMessage.text = message;
        letterBoxMessage.visible = true;
        letterBoxMessage.popupRadius = 0;
        letterBoxMessage.headerGlyphSize = 20
        letterBoxMessage.headerTextMargin = 2
        letterBoxMessage.headerGlyphMargin = -3
    }

    Timer {
        id: refreshTimer
        interval: 100
        repeat: false
        running: false
        onTriggered: updateRunningScripts();
    }

    Timer {
        id: checkMenu
        interval: 1000
        repeat: true
        running: false
        onTriggered: developerMenuEnabled = MenuInterface.isMenuEnabled("Developer Menus");
    }

    Component {
        id: listModelBuilder
        ListModel {}
    }
    
    Connections {
        target: ScriptDiscoveryService
        onScriptCountChanged: {
            runningScriptsModel = listModelBuilder.createObject(root);
            refreshTimer.restart();
        }
    }

    Component.onCompleted: {
        isHMD = HMD.active;
        updateRunningScripts();
        developerMenuEnabled = MenuInterface.isMenuEnabled("Developer Menus");
        checkMenu.restart();
    }

    function updateRunningScripts() {
        function simplify(path) {
            // trim URI querystring/fragment
            path = (path+'').replace(/[#?].*$/,'');
            // normalize separators and grab last path segment (ie: just the filename)
            path = path.replace(/\\/g, '/').split('/').pop();
            // return lowercased because we want to sort mnemonically
            return path.toLowerCase();
        }
        var runningScripts = ScriptDiscoveryService.getRunning();
        runningScripts.sort(function(a,b) {
            a = simplify(a.path);
            b = simplify(b.path);
            return a < b ? -1 : a > b ? 1 : 0;
        });
        // Calling  `runningScriptsModel.clear()` here instead of creating a new object
        // triggers some kind of weird heap corruption deep inside Qt.  So instead of
        // modifying the model in place, possibly triggering behaviors in the table
        // instead we create a new `ListModel`, populate it and update the 
        // existing model atomically.
        var newRunningScriptsModel = listModelBuilder.createObject(root);
        for (var i = 0; i < runningScripts.length; ++i) {
            newRunningScriptsModel.append(runningScripts[i]);
        }
        runningScriptsModel = newRunningScriptsModel;
    }

    function loadScript(script) {
        console.log("Load script " + script);
        scripts.loadOneScript(script);
    }

    function reloadScript(script) {
        console.log("Reload script " + script);
        scripts.stopScript(script, true);
    }

    function stopScript(script) {
        console.log("Stop script " + script);
        scripts.stopScript(script);
    }

    function reloadAll() {
        console.log("Reload all scripts");
        if (!developerMenuEnabled) {
            for (var index = 0; index < runningScriptsModel.count; index++) {
                var url = runningScriptsModel.get(index).url;
                var fileName = url.substring(url.lastIndexOf('/')+1);
                if (canEditScript(fileName)) {
                    scripts.stopScript(url, true);
                }
            }
        } else {
            scripts.reloadAllScripts();
        }
    }

    function loadDefaults() {
        console.log("Load default scripts");
        scripts.loadOneScript(scripts.defaultScriptsPath + "/defaultScripts.js");
    }

    function stopAll() {
        console.log("Stop all scripts");
        for (var index = 0; index < runningScriptsModel.count; index++) {
            var url = runningScriptsModel.get(index).url;
            console.log(url);
            var fileName = url.substring(url.lastIndexOf('/')+1);
            if (canEditScript(fileName)) {
                scripts.stopScript(url);
            }
        }
    }

    function canEditScript(script) {
        if ((script === "controllerScripts.js") || (script === "defaultScripts.js")) {
            return developerMenuEnabled;
        }

        return true;
    }

    Flickable {
        id: flickable
        width: tabletRoot.width
        height: parent.height - (keyboard.raised ? keyboard.raisedHeight : 0)
        contentWidth: column.width
        contentHeight: column.childrenRect.height
        clip: true

        Column {
            id: column
            width: parent.width
            HifiControls.TabletContentSection {
                id: firstSection
                name: "Currently Running"
                isFirst: true

                HifiControls.VerticalSpacer {}

                Row {
                    spacing: hifi.dimensions.contentSpacing.x

                    HifiControls.Button {
                        text: "Reload All"
                        color: hifi.buttons.black
                        onClicked: reloadAll()
                    }

                    HifiControls.Button {
                        text: "Remove All"
                        color: hifi.buttons.red
                        onClicked: stopAll()
                    }

                    HifiControls.Button {
                        text: "Load Defaults"
                        color: hifi.buttons.black
                        height: 26
                        visible: root.developerMenuEnabled;
                        onClicked: loadDefaults()
                    }
                }

                HifiControls.VerticalSpacer {
                    height: hifi.dimensions.controlInterlineHeight + 2  // Add space for border
                }

                HifiControls.Table {
                    model: runningScriptsModel
                    id: table
                    height: 185
                    width: parent.width
                    colorScheme: hifi.colorSchemes.dark
                    expandSelectedRow: true

                    itemDelegate: Item {
                        property bool canEdit: canEditScript(styleData.value);
                        anchors {
                            left: parent ? parent.left : undefined
                            leftMargin: hifi.dimensions.tablePadding
                            right: parent ? parent.right : undefined
                            rightMargin: hifi.dimensions.tablePadding
                        }

                        FiraSansSemiBold {
                            id: textItem
                            text: styleData.value
                            size: hifi.fontSizes.tableText
                            color: table.colorScheme == hifi.colorSchemes.light
                                ? (styleData.selected ? hifi.colors.black : hifi.colors.baseGrayHighlight)
                            : (styleData.selected ? hifi.colors.black : hifi.colors.lightGrayText)
                            anchors {
                                left: parent.left
                                right: parent.right
                                top: parent.top
                                topMargin: 3
                            }

                            HiFiGlyphs {
                                id: reloadButton
                                text: ((canEditScript(styleData.value)) ? hifi.glyphs.reload : hifi.glyphs.lock)
                                color: reloadButtonArea.pressed ? hifi.colors.white : parent.color
                                size: 21
                                anchors {
                                    top: parent.top
                                    right: stopButton.left
                                    verticalCenter: parent.verticalCenter
                                }
                                MouseArea {
                                    id: reloadButtonArea
                                    anchors { fill: parent; margins: -2 }
                                    onClicked: {
                                        if (canEdit) {
                                            reloadScript(model.url)
                                        } else {
                                            letterBox(hifi.glyphs.lock,
                                                      "Developer Mode only",
                                                      "In order to edit, delete or reload this script," +
                                                      " turn on Developer Mode by going to:" +
                                                      " Menu > Settings > Developer Menus");
                                        }
                                    }
                                }
                            }

                            HiFiGlyphs {
                                id: stopButton
                                text: hifi.glyphs.closeSmall
                                color: stopButtonArea.pressed ? hifi.colors.white : parent.color
                                visible: canEditScript(styleData.value)
                                anchors {
                                    top: parent.top
                                    right: parent.right
                                    verticalCenter: parent.verticalCenter
                                }
                                MouseArea {
                                    id: stopButtonArea
                                    anchors { fill: parent; margins: -2 }
                                    onClicked: {
                                        if (canEdit) {
                                            stopScript(model.url)
                                        }
                                    }
                                }
                            }

                        }

                        FiraSansSemiBold {
                            text: runningScriptsModel.get(styleData.row) ? runningScriptsModel.get(styleData.row).url : ""
                            elide: Text.ElideMiddle
                            size: hifi.fontSizes.tableText
                            color: table.colorScheme == hifi.colorSchemes.light
                                ? (styleData.selected ? hifi.colors.black : hifi.colors.lightGray)
                            : (styleData.selected ? hifi.colors.black : hifi.colors.lightGrayText)
                            anchors {
                                top: textItem.bottom
                                left: parent.left
                                right: parent.right
                            }
                            visible: styleData.selected
                        }
                    }

                    TableViewColumn {
                        role: "name"
                    }
                }

                HifiControls.VerticalSpacer {
                    height: hifi.dimensions.controlInterlineHeight + 2  // Add space for border
                }
            }

            HifiControls.TabletContentSection {
                name: "Load Scripts"

                HifiControls.VerticalSpacer {}

                Row {
                    spacing: hifi.dimensions.contentSpacing.x

                    HifiControls.Button {
                        text: "from URL"
                        color: hifi.buttons.black
                        height: 26
                        onClicked: fromUrlTimer.running = true

                        // For some reason trigginer an API that enters
                        // an internal event loop directly from the button clicked
                        // trigger below causes the appliction to behave oddly.
                        // Most likely because the button onClicked handling is never
                        // completed until the function returns.
                        // FIXME find a better way of handling the input dialogs that
                        // doesn't trigger this.
                        Timer {
                            id: fromUrlTimer
                            interval: 5
                            repeat: false
                            running: false
                            onTriggered: ApplicationInterface.loadScriptURLDialog();
                        }
                    }

                    HifiControls.Button {
                        text: "from Disk"
                        color: hifi.buttons.black
                        height: 26
                        onClicked: fromDiskTimer.running = true

                        Timer {
                            id: fromDiskTimer
                            interval: 5
                            repeat: false
                            running: false
                            onTriggered: ApplicationInterface.loadDialog();
                        }
                    }
                }

                HifiControls.VerticalSpacer {}

                HifiControls.TextField {
                    id: filterEdit
                    isSearchField: true
                    anchors.left: parent.left
                    anchors.right: parent.right
                    colorScheme: hifi.colorSchemes.dark
                    placeholderText: "Filter"
                    onTextChanged: scriptsModel.filterRegExp = new RegExp("^.*" + text + ".*$", "i")
                    Component.onCompleted: scriptsModel.filterRegExp = new RegExp("^.*$", "i")
                    onActiveFocusChanged: {
                        // raise the keyboard
                        keyboard.raised = activeFocus;

                        // scroll to the bottom of the content area.
                        if (activeFocus) {
                            flickable.contentY = (flickable.contentHeight - flickable.height);
                        }
                    }
                }

                HifiControls.VerticalSpacer {
                    height: hifi.dimensions.controlInterlineHeight + 2  // Add space for border
                }

                HifiControls.Tree {
                    id: treeView
                    height: 155
                    treeModel: scriptsModel
                    colorScheme: hifi.colorSchemes.dark
                    anchors.left: parent.left
                    anchors.right: parent.right
                }

                HifiControls.VerticalSpacer {
                    height: hifi.dimensions.controlInterlineHeight + 2  // Add space for border
                }

                HifiControls.TextField {
                    id: selectedScript
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.rightMargin: loadButton.width + hifi.dimensions.contentSpacing.x

                    colorScheme: hifi.colorSchemes.dark
                    readOnly: true

                    Connections {
                        target: treeView.selection
                        onCurrentIndexChanged: {
                            var path = scriptsModel.data(treeView.selection.currentIndex, 0x100)
                            if (path) {
                                selectedScript.text = path
                            } else {
                                selectedScript.text = ""
                            }
                        }
                    }
                }

                Item {
                    // Take the loadButton out of the column flow.
                    id: loadButtonContainer
                    anchors.top: selectedScript.top
                    anchors.right: parent.right

                    HifiControls.Button {
                        id: loadButton
                        anchors.right: parent.right

                        text: "Load"
                        color: hifi.buttons.blue
                        enabled: selectedScript.text != ""
                        onClicked: root.loadScript(selectedScript.text)
                    }
                }

                HifiControls.VerticalSpacer {
                    height: hifi.dimensions.controlInterlineHeight - (!isHMD ? 3 : 0)
                }

                HifiControls.TextAction {
                    id: directoryButton
                    icon: hifi.glyphs.script
                    iconSize: 24
                    text: "Reveal Scripts Folder"
                    onClicked: fileDialogHelper.openDirectory(scripts.defaultScriptsPath)
                    colorScheme: hifi.colorSchemes.dark
                    anchors.left: parent.left
                    visible: !isHMD
                }

                HifiControls.VerticalSpacer {
                    height: hifi.dimensions.controlInterlineHeight - 3
                    visible: !isHMD
                }
            }
        }
    }

    HifiControls.Keyboard {
        id: keyboard
        raised: false
        numeric: false
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
    }
}

