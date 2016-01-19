import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../../../interface/resources/qml"
import "../../../interface/resources/qml/windows"
import "../../../interface/resources/qml/dialogs"

ApplicationWindow {
    id: appWindow
    visible: true
    width: 1280
    height: 720
    title: qsTr("Scratch App")

    Component { id: listModelBuilder; ListModel{} }

    function menuItemsToModel(menu) {
        var items = menu.items
        var newListModel = listModelBuilder.createObject(desktop);
        for (var i = 0; i < items.length; ++i) {
            var item = items[i];
            switch (item.type) {
            case 2:
                newListModel.append({"type":item.type, "name": item.title, "item": item})
                break;
            case 1:
                newListModel.append({"type":item.type, "name": item.text, "item": item})
                break;
            case 0:
                newListModel.append({"type":item.type, "name": "-----", "item": item})
                break;
            }
        }
        return newListModel;
    }

    Root {
        id: desktop
        anchors.fill: parent
        StubMenu { id: stubMenu }
        Component.onCompleted: offscreenWindow = appWindow

        Row {
            id: testButtons
            anchors { margins: 8; left: parent.left; top: parent.top }
            spacing: 8
            property int count: 0

            property var tabs: [];
            property var urls: [];
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
//            Button {
//                text: "add web tab"
//                onClicked: {
//                    testButtons.urls.push("http://slashdot.org?" + testButtons.count++);
//                    testButtons.tabs.push(desktop.toolWindow.addWebTab({ title: "test", source: testButtons.urls[testButtons.urls.length - 1], width: 500, height: 720 }))
//                }
//            }
//            Button {
//                text: "toggle tab visible"
//                onClicked: {
//                    var lastUrl = testButtons.urls[testButtons.urls.length - 1];
//                    var tab = desktop.toolWindow.findTabForUrl(lastUrl);
//                    desktop.toolWindow.showTabForUrl(lastUrl, !tab.enabled)
//                }
//            }
//            Button {
//                text: "Remove last tab"
//                onClicked: {
//                    testButtons.tabs.pop();
//                    desktop.toolWindow.removeTabForUrl(testButtons.urls.pop());
//                }
//            }
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
                    desktop.messageBox({
                                           text: "Diagnostic cycle will be complete in 30 seconds",
                                           icon: OriginalDialogs.StandardIcon.Critical,
                                       });
                }
            }
            Button {
                text: "Open File"
                property var builder: Component {
                    FileDialog { }
                }

                ListModel {
                    id: jsFilters
                    ListElement { text: "Javascript Files (*.js)"; filter: "*.js" }
                    ListElement { text: "All Files (*.*)"; filter: "*.*" }
                }
                onClicked: {
                    var fileDialogProperties = {
                        filterModel: jsFilters
                    }
                    var fileDialog = builder.createObject(desktop, fileDialogProperties);
                    fileDialog.canceled.connect(function(){
                        console.log("Cancelled")
                    })
                    fileDialog.selectedFile.connect(function(file){
                        console.log("Selected " + file)
                    })
                }
            }
            Button {
                text: "Focus Test"
                onClicked: {
                    var item = desktop;
                    while (item) {
                        console.log(item);
                        item = item.parent;
                    }
                    item = appWindow
                    while (item) {
                        console.log(item);
                        item = item.parent;
                    }
                    console.log(appWindow.activeFocusItem);
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

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton

            Component {
                id: menuBuilder
                VrMenuView { }
            }

            onClicked: {
                console.log("zzz")
                var menuItems = menuItemsToModel(stubMenu);
                var newMenu = menuBuilder.createObject(desktop, {  source: stubMenu, items: menuItems });
                newMenu.x = mouseX
                newMenu.y = mouseY
            }

        }
    }

    /*
    Arcane.Test {
        anchors.centerIn: parent
        height: 600; width: 600
    }


    Item {
        id: desktop
        anchors.fill: parent
        objectName: Desktop._OFFSCREEN_ROOT_OBJECT_NAME
        property bool uiVisible: true
        property variant toolbars: { "_root" : null }
        focus: true



        Rectangle {
            id: root
            Vr.Constants { id: vr }
            implicitWidth: 384; implicitHeight: 640
            anchors.centerIn: parent
            color: vr.windows.colors.background
            border.color: vr.controls.colors.background
            border.width: vr.styles.borderWidth
            radius: vr.styles.borderRadius
            RunningScripts { }
        }

        FileDialog {
            id: fileDialog
            width: 800; height: 600
            anchors.centerIn: parent
            onSelectedFile: console.log("Chose file " + file)
        }
        Timer {
            id: timer
            running: false
            interval: 100
            onTriggered: wireFrameContainer.enabled = true
        }

        Item {
            id: wireFrameContainer
            objectName: Desktop._OFFSCREEN_DIALOG_OBJECT_NAME
            anchors.fill: parent
            onEnabledChanged: if (!enabled) timer.running = true

            NewUi.Main {
                id: wireFrame
                anchors.fill: parent

                property var offscreenFlags: Item {
                    property bool navigationFocused: false
                }

                property var urlHandler: Item {
                    function fixupUrl(url) {
                        var urlString = url.toString();
                        if (urlString.indexOf("https://metaverse.highfidelity.com/") !== -1 &&
                            urlString.indexOf("access_token") === -1) {
                            console.log("metaverse URL, fixing")
                            return urlString + "?access_token=875885020b1d5f1ea694ce971c8601fa33ffd77f61851be01ed1e3fde8cabbe9"
                        }
                        return url
                    }

                    function canHandleUrl(url) {
                        var urlString = url.toString();
                        if (urlString.indexOf("hifi://") === 0) {
                            console.log("Can handle hifi addresses: " + urlString)
                            return true;
                        }

                        if (urlString.indexOf(".svo.json?") !== -1) {
                            console.log("Can handle svo json addresses: " + urlString)
                            return true;
                        }

                        if (urlString.indexOf(".js?") !== -1) {
                            console.log("Can handle javascript addresses: " + urlString)
                            return true;
                        }

                        return false
                    }

                    function handleUrl(url) {
                        return true
                    }
                }

                property var addressManager: Item {
                    function navigate(url) {
                        console.log("Navigate to: " + url);
                    }
                }
            }
        }
        Keys.onMenuPressed: desktop.uiVisible = !desktop.uiVisible
        Keys.onEscapePressed: desktop.uiVisible = !desktop.uiVisible
        }
*/
}
