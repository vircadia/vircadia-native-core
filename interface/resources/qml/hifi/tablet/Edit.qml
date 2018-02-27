import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../../styles-uit"
import "../../controls-uit" as HifiControls
import "../../controls"
import "../toolbars"

ColumnLayout {
    id: editRoot
    objectName: "stack"
    anchors.fill: parent

    readonly property var webTabsLinks: [
        "",
        "/system/html/entityList.html",
        "/system/html/entityProperties.html",
        "/system/html/gridControls.html",
        "/system/particle_explorer/particleExplorer.html"
    ]

    signal sendToScript(var message);

    function pushSource(path) {
        editStack.push(Qt.resolvedUrl("../../" + path));
        editStack.currentItem.sendToScript.connect(editRoot.sendToScript);
    }

    function popSource() {
        editStack.pop();
    }

    function fromScript(message) {
        console.error("from script", JSON.stringify(message))
        switch (message.method) {
        case 'selectTab':
            selectTab(message.params.id);
            break;
        default:
            var currentItem = editStack.currentItem;
            if (currentItem && currentItem.fromScript) {
                currentItem.fromScript(message);
            } else {
                console.warn('Unrecognized message:', JSON.stringify(message));
            }
        }
    }

    // Changes the current tab based on tab index or title as input
    function selectTab(id) {
        console.error("selecting tab", id)
        if (typeof id === 'number') {
            if (id >= 0 && id <= 4) {
                editTabView.currentIndex = id;
            } else {
                console.warn('Attempt to switch to invalid tab:', id);
            }
        } else if (typeof id === 'string'){
            switch (id.toLowerCase()) {
            case 'create':
                editTabView.currentIndex = 0;
                break;
            case 'list':
                editTabView.currentIndex = 1;
                break;
            case 'properties':
                editTabView.currentIndex = 2;
                break;
            case 'grid':
                editTabView.currentIndex = 3;
                break;
            case 'particle':
                editTabView.currentIndex = 4;
                break;
            default:
                console.warn('Attempt to switch to invalid tab:', id);
            }
        } else {
            console.warn('Attempt to switch tabs with invalid input:', JSON.stringify(id));
        }
    }
    spacing: 0

    HifiConstants { id: hifi }

    TabBar {
        id: editTabView
        height: 60
        width: parent.width
        contentWidth: parent.width
        currentIndex: 0
        padding: 0
        spacing: 0

        onCurrentIndexChanged: {
            if (currentIndex === 0) {
                editStack.replace(null, mainTab, {}, StackView.Immediate)
            } else {
                editStack.replace(null, webViewTab,
                                  {url: Paths.defaultScripts + webTabsLinks[currentIndex]},
                                  StackView.Immediate)
            }
        }

        EditTabButton {
            text: "CREATE"
        }

        EditTabButton {
            text: "LIST"
        }

        EditTabButton {
            text: "PROPERTIES"
        }

        EditTabButton {
            text: "GRID"
        }

        EditTabButton {
            text: "P"
        }
    }

    StackView {
        id: editStack

        Layout.fillHeight: true
        Layout.fillWidth: true
        initialItem: mainTab//Qt.resolvedUrl('EditTabView.qml')
    }

    Component {
        id: mainTab


        Rectangle { //1st tab
            color: "#404040"

            Text {
                color: "#ffffff"
                text: "Choose an Entity Type to Create:"
                font.pixelSize: 14
                font.bold: true
                anchors.top: parent.top
                anchors.topMargin: 28
                anchors.left: parent.left
                anchors.leftMargin: 28
            }

            Flow {
                id: createEntitiesFlow
                spacing: 35
                anchors.right: parent.right
                anchors.rightMargin: 55
                anchors.left: parent.left
                anchors.leftMargin: 55
                anchors.top: parent.top
                anchors.topMargin: 70


                NewEntityButton {
                    icon: "icons/create-icons/94-model-01.svg"
                    text: "MODEL"
                    onClicked: {
                        editRoot.sendToScript({
                                                  method: "newEntityButtonClicked", params: { buttonName: "newModelButton" }
                                              });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/21-cube-01.svg"
                    text: "CUBE"
                    onClicked: {
                        editRoot.sendToScript({
                                                  method: "newEntityButtonClicked", params: { buttonName: "newCubeButton" }
                                              });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/22-sphere-01.svg"
                    text: "SPHERE"
                    onClicked: {
                        editRoot.sendToScript({
                                                  method: "newEntityButtonClicked", params: { buttonName: "newSphereButton" }
                                              });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/24-light-01.svg"
                    text: "LIGHT"
                    onClicked: {
                        editRoot.sendToScript({
                                                  method: "newEntityButtonClicked", params: { buttonName: "newLightButton" }
                                              });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/20-text-01.svg"
                    text: "TEXT"
                    onClicked: {
                        editRoot.sendToScript({
                                                  method: "newEntityButtonClicked", params: { buttonName: "newTextButton" }
                                              });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/25-web-1-01.svg"
                    text: "WEB"
                    onClicked: {
                        editRoot.sendToScript({
                                                  method: "newEntityButtonClicked", params: { buttonName: "newWebButton" }
                                              });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/23-zone-01.svg"
                    text: "ZONE"
                    onClicked: {
                        editRoot.sendToScript({
                                                  method: "newEntityButtonClicked", params: { buttonName: "newZoneButton" }
                                              });
                        editTabView.currentIndex = 2
                    }
                }

                NewEntityButton {
                    icon: "icons/create-icons/90-particles-01.svg"
                    text: "PARTICLE"
                    onClicked: {
                        editRoot.sendToScript({
                                                  method: "newEntityButtonClicked", params: { buttonName: "newParticleButton" }
                                              });
                        editTabView.currentIndex = 4
                    }
                }
            }

            HifiControls.Button {
                id: assetServerButton
                text: "Open This Domain's Asset Server"
                color: hifi.buttons.black
                colorScheme: hifi.colorSchemes.dark
                anchors.right: parent.right
                anchors.rightMargin: 55
                anchors.left: parent.left
                anchors.leftMargin: 55
                anchors.top: createEntitiesFlow.bottom
                anchors.topMargin: 35
                onClicked: {
                    editRoot.sendToScript({
                                              method: "newEntityButtonClicked", params: { buttonName: "openAssetBrowserButton" }
                                          });
                }
            }

            HifiControls.Button {
                text: "Import Entities (.json)"
                color: hifi.buttons.black
                colorScheme: hifi.colorSchemes.dark
                anchors.right: parent.right
                anchors.rightMargin: 55
                anchors.left: parent.left
                anchors.leftMargin: 55
                anchors.top: assetServerButton.bottom
                anchors.topMargin: 20
                onClicked: {
                    editRoot.sendToScript({
                                              method: "newEntityButtonClicked", params: { buttonName: "importEntitiesButton" }
                                          });
                }
            }
        }
    }

    Component {
        id: webViewTab
        WebView {}
    }
}

