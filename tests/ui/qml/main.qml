import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../../../interface/resources/qml"
import "../../../interface/resources/qml/windows"
import "../../../interface/resources/qml/dialogs"
import "../../../interface/resources/qml/hifi"

ApplicationWindow {
    id: appWindow
    visible: true
    width: 1280
    height: 720
    title: qsTr("Scratch App")

    Desktop {
        id: desktop
        anchors.fill: parent
        rootMenu: StubMenu { id: rootMenu }
        Component.onCompleted: offscreenWindow = appWindow

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: desktop.popupMenu(Qt.vector2d(mouseX, mouseY));
        }

        Row {
            id: testButtons
            anchors { margins: 8; left: parent.left; top: parent.top }
            spacing: 8
            property int count: 0

            property var tabs: [];
            property var urls: [];
            /*
            Button {
                text: "restore all"
                onClicked: {
                    for (var i = 0; i < desktop.windows.length; ++i) {
                        desktop.windows[i].visible = true
                    }
                }
            }
            Button {
                text: "toggle blue visible"
                onClicked: {
                    blue.visible = !blue.visible
                }
            }
            Button {
                text: "toggle blue enabled"
                onClicked: {
                    blue.enabled = !blue.enabled
                }
            }
            */
            Button {
                text: "Show Long Error"
                onClicked: {
                    desktop.messageBox({
                                           informativeText: "Diagnostic cycle will be complete in 30 seconds Diagnostic cycle will be complete in 30 seconds  Diagnostic cycle will be complete in 30 seconds  Diagnostic cycle will be complete in 30 seconds Diagnostic cycle will be complete in 30 seconds Diagnostic cycle will be complete in 30 seconds  Diagnostic cycle will be complete in 30 seconds  Diagnostic cycle will be complete in 30 seconds ",
                                           text: "Baloney",
                                           icon: OriginalDialogs.StandardIcon.Warning,
                                           detailedText: "sakjd;laskj dksa;dl jka;lsd j;lkjas ;dlkaj s;dlakjd ;alkjda; slkjda; lkjda;lksjd ;alksjd; alksjd ;alksjd; alksjd; alksdjas;ldkjas;lkdja ;kj ;lkasjd; lkj as;dlka jsd;lka jsd;laksjd a"
                                       });
                }
            }
            Button {
                text: "Show Error"
                onClicked: {
                    var messageBox = desktop.messageBox({
                                                            text: "Diagnostic cycle will be complete in 30 seconds",
                                                            icon: OriginalDialogs.StandardIcon.Critical,
                                                        });
                    messageBox.selected.connect(function(button) {
                        console.log("You clicked " + button)
                    })
                }
            }
            Button {
                text: "Show Query"
                onClicked: {
                    var queryBox = desktop.queryBox({
                                                          text: "Have you stopped beating your wife?",
                                                          placeholderText: "Are you sure?",
                                                         // icon: OriginalDialogs.StandardIcon.Critical,
                                                      });
                    queryBox.selected.connect(function(result) {
                        console.log("User responded with " + result);
                    });

                    queryBox.canceled.connect(function() {
                        console.log("User cancelled query box ");
                    })
                }
            }
            Button {
                text: "Open Directory"
                property var builder: Component {
                    FileDialog { selectDirectory: true }
                }

                onClicked: {
                    var fileDialog = builder.createObject(desktop);
                    fileDialog.canceled.connect(function(){
                        console.log("Cancelled")
                    })
                    fileDialog.selectedFile.connect(function(file){
                        console.log("Selected " + file)
                    })
                }
            }

            Button {
                text: "Open File"
                property var builder: Component {
                    FileDialog {
                        folder: "file:///C:/users/bdavis";
                        filterModel: ListModel {
                            ListElement { text: "Javascript Files (*.js)"; filter: "*.js" }
                            ListElement { text: "All Files (*.*)"; filter: "*.*" }
                        }
                    }
                }

                onClicked: {
                    var fileDialog = builder.createObject(desktop);
                    fileDialog.canceled.connect(function(){
                        console.log("Cancelled")
                    })
                    fileDialog.selectedFile.connect(function(file){
                        console.log("Selected " + file)
                    })
                }
            }
        }

        Window {
            id: blue
            closable: true
            visible: true
            resizable: true
            destroyOnInvisible: false

            width: 100; height: 100
            x: 1280 / 2; y: 720 / 2
            Settings {
                category: "TestWindow.Blue"
                property alias x: blue.x
                property alias y: blue.y
                property alias width: blue.width
                property alias height: blue.height
            }

            Rectangle {
                anchors.fill: parent
                visible: true
                color: "blue"
                Component.onDestruction: console.log("Blue destroyed")
            }
        }
        /*
        Window {
            id: green
            alwaysOnTop: true
            closable: true
            visible: true
            resizable: false
            x: 1280 / 2; y: 720 / 2
            Settings {
                category: "TestWindow.Green"
                property alias x: green.x
                property alias y: green.y
                property alias width: green.width
                property alias height: green.height
            }
            width: 100; height: 100
            Rectangle { anchors.fill: parent; color: "green" }
        }

        Window {
            id: yellow
            objectName: "Yellow"
            closable: true
            visible: true
            resizable: true
            width: 100; height: 100
            Rectangle {
                anchors.fill: parent
                visible: true
                color: "yellow"
            }
        }
        */
    }
}
