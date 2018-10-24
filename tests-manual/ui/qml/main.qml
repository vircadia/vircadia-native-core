import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../../../interface/resources/qml"
//import "../../../interface/resources/qml/windows"
import "../../../interface/resources/qml/windows"
import "../../../interface/resources/qml/dialogs"
import "../../../interface/resources/qml/hifi"
import "../../../interface/resources/qml/hifi/dialogs"
import "../../../interface/resources/qml/styles-uit"

ApplicationWindow {
    id: appWindow
    objectName: "MainWindow"
    visible: true
    width: 1280
    height: 800
    title: qsTr("Scratch App")
    toolBar: Row {
        id: testButtons
        anchors { margins: 8; left: parent.left; top: parent.top }
        spacing: 8
        property int count: 0

        property var tabs: [];
        property var urls: [];
        property var toolbar;
        property var lastButton;

        Button {
            text: HMD.active ? "Disable HMD" : "Enable HMD"
            onClicked: HMD.active = !HMD.active
        }

        Button {
            text: desktop.hmdHandMouseActive ? "Disable HMD HandMouse" : "Enable HMD HandMouse"
            onClicked: desktop.hmdHandMouseActive = !desktop.hmdHandMouseActive
        }

        // Window visibility
        Button {
            text: "toggle desktop"
            onClicked: desktop.togglePinned()
        }

        Button {
            text: "Create Toolbar"
            onClicked: testButtons.toolbar = desktop.getToolbar("com.highfidelity.interface.toolbar.system");
        }

        Button {
            text: "Toggle Toolbar Direction"
            onClicked: testButtons.toolbar.horizontal = !testButtons.toolbar.horizontal
        }

        Button {
            readonly property var icons: [
                "edit-01.svg",
                "model-01.svg",
                "cube-01.svg",
                "sphere-01.svg",
                "light-01.svg",
                "text-01.svg",
                "web-01.svg",
                "zone-01.svg",
                "particle-01.svg",
            ]
            property int iconIndex: 0
            readonly property string toolIconUrl: "../../../../../scripts/system/assets/images/tools/"
            text: "Create Button"
            onClicked: {
                var name = icons[iconIndex];
                var url = toolIconUrl + name;
                iconIndex = (iconIndex + 1) % icons.length;
                var button = testButtons.lastButton = testButtons.toolbar.addButton({
                    imageURL: url,
                    objectName: name,
                    subImage: {
                        y: 50,
                    },
                    alpha: 0.9
                });

                button.clicked.connect(function(){
                    console.log("Clicked on button " + button.imageURL + " alpha " + button.alpha)
                });
            }
        }

        Button {
            text: "Toggle Button Visible"
            onClicked: testButtons.lastButton.visible = !testButtons.lastButton.visible
        }

        // Error alerts
        /*
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
        */

        // query
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

        // Browser
        /*
        Button {
            text: "Open Browser"
            onClicked: builder.createObject(desktop);
            property var builder: Component {
                Browser {}
            }
        }
        */


        // file dialog
        /*

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
                    title: "Open File"
                    filter: "All Files (*.*)"
                    //filter: "HTML files (*.html);;Other(*.png)"
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
        */

        // tabs
        /*
        Button {
            text: "Add Tab"
            onClicked: {
                console.log(desktop.toolWindow);
                desktop.toolWindow.addWebTab({ source: "Foo" });
                desktop.toolWindow.showTabForUrl("Foo", true);
            }
        }

        Button {
            text: "Add Tab 2"
            onClicked: {
                console.log(desktop.toolWindow);
                desktop.toolWindow.addWebTab({ source: "Foo 2" });
                desktop.toolWindow.showTabForUrl("Foo 2", true);
            }
        }

        Button {
            text: "Add Tab 3"
            onClicked: {
                console.log(desktop.toolWindow);
                desktop.toolWindow.addWebTab({ source: "Foo 3" });
                desktop.toolWindow.showTabForUrl("Foo 3", true);
            }
        }

        Button {
            text: "Destroy Tab"
            onClicked: {
                console.log(desktop.toolWindow);
                desktop.toolWindow.removeTabForUrl("Foo");
            }
        }
        */

        // Hifi specific stuff
        /*
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
        */
        // bookmarks
        /*
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
        */

    }


    HifiConstants { id: hifi }

    Desktop {
        id: desktop
        anchors.fill: parent

        //rootMenu: StubMenu { id: rootMenu }
        //Component.onCompleted: offscreenWindow = appWindow

        /*
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: desktop.popupMenu(Qt.vector2d(mouseX, mouseY));
        }
        */

        Browser {
            url: "http://s3.amazonaws.com/DreamingContent/testUiDelegates.html"
        }


        Window {
            id: blue
            closable: true
            visible: true
            resizable: true
            destroyOnHidden: false
            title: "Blue"

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

        Window {
            id: green
            closable: true
            visible: true
            resizable: true
            title: "Green"
            destroyOnHidden: false

            width: 100; height: 100
            x: 1280 / 2; y: 720 / 2
            Settings {
                category: "TestWindow.Green"
                property alias x: green.x
                property alias y: green.y
                property alias width: green.width
                property alias height: green.height
            }

            Rectangle {
                anchors.fill: parent
                visible: true
                color: "green"
                Component.onDestruction: console.log("Green destroyed")
            }
        }

/*
        Rectangle { width: 100; height: 100; x: 100; y: 100; color: "#00f" }

        Window {
            id: green
            alwaysOnTop: true
            frame: HiddenFrame{}
            hideBackground: true
            closable: true
            visible: true
            resizable: false
            x: 1280 / 2; y: 720 / 2
            width: 100; height: 100
            Rectangle {
                color: "#0f0"
                width: green.width;
                height: green.height;
            }
        }
        */

/*
        Window {
            id: yellow
            closable: true
            visible: true
            resizable: true
            x: 100; y: 100
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
        id: openBrowserAction
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




