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
import "../../controls"

ScrollingWindow {
    id: root
    objectName: "RunningScripts"
    title: "Running Scripts"
    resizable: true
    destroyOnHidden: true
    implicitWidth: 424
    implicitHeight: isHMD ? 695 : 728
    minSize: Qt.vector2d(424, 300)

    HifiConstants { id: hifi }

    property var scripts: ScriptDiscoveryService;
    property var scriptsModel: scripts.scriptsModelFilter
    property var runningScriptsModel: ListModel { }
    property bool isHMD: false

    onVisibleChanged: console.log("Running scripts visible changed to " + visible)
    onShownChanged: console.log("Running scripts visible changed to " + visible)

    Settings {
        category: "Overlay.RunningScripts"
        property alias x: root.x
        property alias y: root.y
    }

    Connections {
        target: ScriptDiscoveryService
        onScriptCountChanged: updateRunningScripts();
    }

    Component.onCompleted: {
        isHMD = HMD.active;
        updateRunningScripts();
    }

    function updateRunningScripts() {
        var runningScripts = ScriptDiscoveryService.getRunning();
        runningScriptsModel.clear()
        for (var i = 0; i < runningScripts.length; ++i) {
            runningScriptsModel.append(runningScripts[i]);
        }
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
        scripts.reloadAllScripts();
    }

    function loadDefaults() {
        console.log("Load default scripts");
        scripts.loadOneScript(scripts.defaultScriptsPath + "/defaultScripts.js");
    }

    function stopAll() {
        console.log("Stop all scripts");
        scripts.stopAllScripts();
    }

    Column {
        property bool keyboardRaised: false
        property bool punctuationMode: false

        width: pane.contentWidth

        HifiControls.ContentSection {
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
            }

            HifiControls.VerticalSpacer {
                height: hifi.dimensions.controlInterlineHeight + 2  // Add space for border
            }

            HifiControls.Table {
                model: runningScriptsModel
                id: table
                height: 185
                colorScheme: hifi.colorSchemes.dark
                anchors.left: parent.left
                anchors.right: parent.right
                expandSelectedRow: true

                itemDelegate: Item {
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
                            text: hifi.glyphs.reloadSmall
                            color: reloadButtonArea.pressed ? hifi.colors.white : parent.color
                            anchors {
                                top: parent.top
                                right: stopButton.left
                                verticalCenter: parent.verticalCenter
                            }
                            MouseArea {
                                id: reloadButtonArea
                                anchors { fill: parent; margins: -2 }
                                onClicked: reloadScript(model.url)
                            }
                        }

                        HiFiGlyphs {
                            id: stopButton
                            text: hifi.glyphs.closeSmall
                            color: stopButtonArea.pressed ? hifi.colors.white : parent.color
                            anchors {
                                top: parent.top
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                            }
                            MouseArea {
                                id: stopButtonArea
                                anchors { fill: parent; margins: -2 }
                                onClicked: stopScript(model.url)
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

        HifiControls.ContentSection {
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

                HifiControls.Button {
                    text: "Load Defaults"
                    color: hifi.buttons.black
                    height: 26
                    onClicked: loadDefaults()
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
                onTextChanged: scriptsModel.filterRegExp =  new RegExp("^.*" + text + ".*$", "i")
                Component.onCompleted: scriptsModel.filterRegExp = new RegExp("^.*$", "i")
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
                    target: treeView
                    onCurrentIndexChanged: {
                        var path = scriptsModel.data(treeView.currentIndex, 0x100)
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

        // virtual keyboard, letters
        Keyboard {
            id: keyboard1
            height: parent.keyboardRaised ? 200 : 0
            visible: parent.keyboardRaised && !parent.punctuationMode
            enabled: parent.keyboardRaised && !parent.punctuationMode
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
        }

        KeyboardPunctuation {
            id: keyboard2
            height: parent.keyboardRaised ? 200 : 0
            visible: parent.keyboardRaised && parent.punctuationMode
            enabled: parent.keyboardRaised && parent.punctuationMode
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
        }
    }
}
