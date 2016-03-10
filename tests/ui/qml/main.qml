import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../../../interface/resources/qml"
//import "../../../interface/resources/qml/windows"
import "../../../interface/resources/qml/windows-uit"
import "../../../interface/resources/qml/dialogs"
import "../../../interface/resources/qml/hifi"
import "../../../interface/resources/qml/hifi/dialogs"
import "../../../interface/resources/qml/styles-uit"

ApplicationWindow {
    id: appWindow
    visible: true
    width: 1280
    height: 800
    title: qsTr("Scratch App")

    HifiConstants { id: hifi }

    Desktop {
        id: desktop
        anchors.fill: parent
        rootMenu: StubMenu { id: rootMenu }
        //Component.onCompleted: offscreenWindow = appWindow

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

            Button {
                // Shows the dialog with preferences sections but not each section's preference items
                // because Preferences.preferencesByCategory() method is not stubbed out.
                text: "Settings > General..."
                property var builder: Component {
                    GeneralPreferencesDialog { }
                }
                onClicked: {
                    var runningScripts = builder.createObject(desktop);
                }
            }

            Button {
                text: "Running Scripts"
                property var builder: Component {
                    RunningScripts { }
                }
                onClicked: {
                    var runningScripts = builder.createObject(desktop);
                }
            }

            Button {
                text: "Attachments"
                property var builder: Component {
                    AttachmentsDialog { }
                }
                onClicked: {
                    var attachmentsDialog = builder.createObject(desktop);
                }
            }

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
                // Replicates message box that pops up after selecting new avatar. Includes title.
                text: "Confirm Avatar"
                onClicked: {
                    var messageBox = desktop.messageBox({
                                           title: "Set Avatar",
                                           text: "Would you like to use 'Albert' for your avatar?",
                                           icon: hifi.icons.question,         // Test question icon
                                           //icon: hifi.icons.information,    // Test informaton icon
                                           //icon: hifi.icons.warning,        // Test warning icon
                                           //icon: hifi.icons.critical,       // Test critical icon
                                           //icon: hifi.icons.none,         // Test no icon
                                           buttons: OriginalDialogs.StandardButton.Ok + OriginalDialogs.StandardButton.Cancel,
                                           defaultButton: OriginalDialogs.StandardButton.Ok
                                       });
                    messageBox.selected.connect(function(button) {
                        console.log("You clicked " + button)
                    })
                }
            }
            Button {
                // Message without title.
                text: "Show Error"
                onClicked: {
                    var messageBox = desktop.messageBox({
                                                            text: "Diagnostic cycle will be complete in 30 seconds",
                                                            icon: hifi.icons.critical,
                                                        });
                    messageBox.selected.connect(function(button) {
                        console.log("You clicked " + button)
                    })
                }
            }
            Button {
                // detailedText is not currently used anywhere in Interface but it is easier to leave in and style good enough.
                text: "Show Long Error"
                onClicked: {
                    desktop.messageBox({
                                           informativeText: "Diagnostic cycle will be complete in 30 seconds Diagnostic cycle will be complete in 30 seconds  Diagnostic cycle will be complete in 30 seconds  Diagnostic cycle will be complete in 30 seconds Diagnostic cycle will be complete in 30 seconds Diagnostic cycle will be complete in 30 seconds  Diagnostic cycle will be complete in 30 seconds  Diagnostic cycle will be complete in 30 seconds ",
                                           text: "Baloney",
                                           icon: hifi.icons.warning,
                                           detailedText: "sakjd;laskj dksa;dl jka;lsd j;lkjas ;dlkaj s;dlakjd ;alkjda; slkjda; lkjda;lksjd ;alksjd; alksjd ;alksjd; alksjd; alksdjas;ldkjas;lkdja ;kj ;lkasjd; lkj as;dlka jsd;lka jsd;laksjd a"
                                       });
                }
            }
            Button {
                text: "Bookmark Location"
                onClicked: {
                    desktop.inputDialog({
                                           title: "Bookmark Location",
                                           icon: hifi.icons.placemark,
                                           label: "Name"
                                       });
                }
            }
            Button {
                text: "Delete Bookmark"
                onClicked: {
                    desktop.inputDialog({
                                           title: "Delete Bookmark",
                                           icon: hifi.icons.placemark,
                                           label: "Select the bookmark to delete",
                                           items: ["Bookmark A", "Bookmark B", "Bookmark C"]
                                       });
                }
            }
            Button {
                text: "Duplicate Bookmark"
                onClicked: {
                    desktop.messageBox({
                                           title: "Duplicate Bookmark",
                                           icon: hifi.icons.warning,
                                           text: "The bookmark name you entered alread exists in yoru list.",
                                           informativeText: "Would you like to overwrite it?",
                                           buttons: OriginalDialogs.StandardButton.Yes + OriginalDialogs.StandardButton.No,
                                           defaultButton: OriginalDialogs.StandardButton.Yes
                                       });
                }
            }
            /*
            // There is no such desktop.queryBox() function; may need to update test to cover QueryDialog.qml?
            Button {
                text: "Show Query"
                onClicked: {
                    var queryBox = desktop.queryBox({
                                                          text: "Have you stopped beating your wife?",
                                                          placeholderText: "Are you sure?",
                                                         // icon: hifi.icons.critical,
                                                      });
                    queryBox.selected.connect(function(result) {
                        console.log("User responded with " + result);
                    });

                    queryBox.canceled.connect(function() {
                        console.log("User cancelled query box ");
                    })
                }
            }
            */
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
                text: "Add Tab"
                onClicked: {
                    console.log(desktop.toolWindow);
                    desktop.toolWindow.addWebTab({ source: "Foo" });
                    desktop.toolWindow.showTabForUrl("Foo", true);
                }
            }

            Button {
                text: "Destroy Tab"
                onClicked: {
                    console.log(desktop.toolWindow);
                    desktop.toolWindow.removeTabForUrl("Foo");
                }
            }

            Button {
                text: "Open File"
                property var builder: Component {
                    FileDialog { }
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

        /*
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
        */
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

    Action {
        text: "Open Browser"
        shortcut: "Ctrl+Shift+X"
        onTriggered: {
            builder.createObject(desktop);
        }
        property var builder: Component {
            ModelBrowserDialog{}
        }
    }
}
