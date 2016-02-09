import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../../styles" as Hifi
import "../../controls" as HifiControls
import "../../windows"

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
                onActiveFocusChanged: if (activeFocus && listView.currentItem) { listView.currentItem.forceActiveFocus(); }
                anchors {
                    top: allButtons.bottom;
                    left: parent.left;
                    right: parent.right;
                    topMargin: 8
                    bottom: row1.top
                    bottomMargin: 8
                }

                ListView {
                    id: listView
                    clip: true
                    anchors { fill: parent; margins: 0 }
                    model: runningScriptsModel
                    delegate: FocusScope {
                        id: scope
                        anchors { left: parent.left; right: parent.right }
                        height: scriptName.height + 12 + (ListView.isCurrentItem ? scriptName.height + 6 : 0)
                        Keys.onDownPressed: listView.incrementCurrentIndex()
                        Keys.onUpPressed: listView.decrementCurrentIndex()
                        Rectangle {
                            id: rectangle
                            anchors.fill: parent
                            clip: true
                            radius: 3
                            color: scope.ListView.isCurrentItem ? "#79f" :
                                                            index % 2 ? "#ddd" : "#eee"

                            Text {
                                id: scriptName
                                anchors { left: parent.left; leftMargin: 4; top: parent.top; topMargin:6 }
                                text: name
                            }

                            Text {
                                id: scriptUrl
                                anchors { left: scriptName.left; right: parent.right; rightMargin: 4; top: scriptName.bottom; topMargin: 6 }
                                text: url
                                elide: Text.ElideMiddle
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: { listView.currentIndex = index; scope.forceActiveFocus(); }
                            }

                            Row {
                                anchors.verticalCenter: scriptName.verticalCenter
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
                    onClicked: fromUrlTimer.running = true;

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

                Button {
                    text: "from Disk"
                    onClicked: fromDiskTimer.running = true;

                    Timer {
                        id: fromDiskTimer
                        interval: 5
                        repeat: false
                        running: false
                        onTriggered: ApplicationInterface.loadDialog();
                    }
                }
            }

            HifiControls.TextField {
                id: filterEdit
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: treeView.top
                anchors.bottomMargin: 8
                placeholderText: "filter"
                focus: true
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

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: treeView.foo();
                }

                function foo() {
                    var localRect = Qt.rect(0, 0, width, height);
                    var rect = desktop.mapFromItem(treeView, 0, 0, width, height)
                    console.log("Local Rect " + localRect)
                    console.log("Rect " + rect)
                    console.log("Desktop size " + Qt.size(desktop.width, desktop.height));
                }

                TableViewColumn {
                    title: "Name";
                    role: "display";
                    //                    delegate: Text {
                    //                        text: styleData.value
                    //                        renderType: Text.QtRendering
                    //                        elite: styleData.elideMode
                    //                    }
                }
            }

            HifiControls.TextField {
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

