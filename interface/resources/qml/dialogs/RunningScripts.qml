import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../styles" as Hifi
import "../controls" as HifiControls
import "../windows"

Window {
    id: root
    objectName: "RunningScripts"
    title: "Running Scripts"
    resizable: true
    destroyOnInvisible: true
    x: 40; y: 40
    implicitWidth: 384; implicitHeight: 640

    property var scripts: ScriptDiscoveryService;
    property var scriptsModel: scripts.scriptsModelFilter
    property var runningScriptsModel: ListModel { }
    property var fileFilters: ListModel {
        id: jsFilters
        ListElement { text: "Javascript Files (*.js)"; filter: "*.js" }
        ListElement { text: "All Files (*.*)"; filter: "*.*" }
    }


    Settings {
        category: "Overlay.RunningScripts"
        property alias x: root.x
        property alias y: root.y
    }

    Connections {
        target: ScriptDiscoveryService
        onScriptCountChanged: updateRunningScripts();
    }

    Component.onCompleted: updateRunningScripts()

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

    function stopAll() {
        console.log("Stop all scripts");
        scripts.stopAllScripts();
    }

    Component {
        id: fileDialogBuilder
        FileDialog { }
    }

    function loadFromFile() {
        var fileDialog = fileDialogBuilder.createObject(desktop, { filterModel: fileFilters });
        fileDialog.canceled.connect(function(){
            console.debug("Cancelled file open")
        })

        fileDialog.selectedFile.connect(function(file){
            console.debug("Selected " + file)
            scripts.loadOneScript(file);
        })
    }

    Rectangle {
        color: "white"
        anchors.fill: parent

        Item {
            anchors { fill: parent; margins: 8 }
            Text {
                id: title
                font.bold: true
                font.pointSize: 16
                color: "#0e7077"
                text: "Currently Running"
            }

            Row {
                id: allButtons
                anchors.top: title.bottom
                anchors.topMargin: 8
                spacing: 8
                Button { text: "Reload all"; onClicked: reloadAll() }
                Button { text: "Stop all"; onClicked: stopAll() }
            }

            ScrollView {
                anchors {
                    top: allButtons.bottom;
                    left: parent.left;
                    right: parent.right;
                    topMargin: 8
                    bottom: row1.top
                    bottomMargin: 8
                }

                ListView {
                    clip: true
                    anchors { fill: parent; margins: 0 }

                    model: runningScriptsModel

                    delegate: Rectangle {
                        radius: 3
                        anchors { left: parent.left; right: parent.right }

                        height: scriptName.height + 12
                        color: index % 2 ? "#ddd" : "#eee"

                        Text {
                            anchors { left: parent.left; leftMargin: 4; verticalCenter: parent.verticalCenter }
                            id: scriptName
                            text: name
                        }

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 4
                            spacing: 4
                            HifiControls.FontAwesome {
                                text: "\uf021"; size: scriptName.height;
                                MouseArea {
                                    anchors { fill: parent; margins: -2; }
                                    onClicked: reloadScript(model.url)
                                }
                            }
                            HifiControls.FontAwesome {
                                size: scriptName.height; text: "\uf00d"
                                MouseArea {
                                    anchors { fill: parent; margins: -2; }
                                    onClicked: stopScript(model.url)
                                }
                            }
                        }
                    }
                }
            }

            Text {
                id: loadLabel
                text: "Load Scripts"
                font.bold: true
                font.pointSize: 16
                color: "#0e7077"

                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.bottom: filterEdit.top
                anchors.bottomMargin: 8
            }

            Row {
                id: row1
                spacing: 8
                anchors.bottom: filterEdit.top
                anchors.bottomMargin: 8
                anchors.right: parent.right
                Button {
                    text: "from URL";
                    onClicked: ApplicationInterface.loadScriptURLDialog();
                }
                Button {
                    text: "from Disk"
                    onClicked: loadFromFile();
                }
            }

            TextField {
                id: filterEdit
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: treeView.top
                anchors.bottomMargin: 8
                placeholderText: "filter"
                onTextChanged: scriptsModel.filterRegExp =  new RegExp("^.*" + text + ".*$", "i")
                Component.onCompleted: scriptsModel.filterRegExp = new RegExp("^.*$", "i")
            }

            TreeView {
                id: treeView
                height: 128
                anchors.bottom: loadButton.top
                anchors.bottomMargin: 8
                anchors.left: parent.left
                anchors.right: parent.right
                headerVisible: false
                focus: true
                // FIXME doesn't work?
                onDoubleClicked: isExpanded(index) ? collapse(index) : expand(index)
                // FIXME not triggered by double click?
                onActivated: {
                    var path = scriptsModel.data(index, 0x100)
                    if (path) {
                        loadScript(path)
                    }
                }
                model: scriptsModel
                TableViewColumn { title: "Name"; role: "display"; }
            }

            TextField {
                id: selectedScript
                readOnly: true
                anchors.left: parent.left
                anchors.right: loadButton.left
                anchors.rightMargin: 8
                anchors.bottom: loadButton.bottom
                anchors.top: loadButton.top
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

            Button {
                id: loadButton
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                text: "Load"
                enabled: selectedScript.text != ""
                onClicked: root.loadScript(selectedScript.text)
            }
        }
    }
}

