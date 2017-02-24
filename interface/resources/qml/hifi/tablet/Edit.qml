import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.1
import QtWebChannel 1.0
import QtQuick.Controls.Styles 1.4
import "../../controls"
import "../toolbars"
import HFWebEngineProfile 1.0
import QtGraphicalEffects 1.0
import "../../styles-uit"

StackView {
    id: editRoot
    objectName: "stack"
    initialItem: editBasePage

    property var eventBridge;
    signal sendToScript(var message);

    function pushSource(path) {
        editRoot.push(Qt.resolvedUrl(path));
        editRoot.currentItem.eventBridge = editRoot.eventBridge;
        editRoot.currentItem.sendToScript.connect(editRoot.sendToScript);
    }

    function popSource() {
        editRoot.pop();
    }


    Component {
        id: editBasePage
        TabView {
            id: editTabView
            // anchors.fill: parent
            height: 60

            Tab {
                title: "CREATE"
                active: true
                enabled: true
                property string originalUrl: ""

                Rectangle { 
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
                                editTabView.currentIndex = 2
                            }
                        }
                    }

                    Item {
                        id: assetServerButton
                        width: 370
                        height: 38
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: createEntitiesFlow.bottom
                        anchors.topMargin: 35
                        
                        Rectangle {
                            id: assetServerButtonBg
                            color: "black"
                            opacity: 1
                            radius: 6
                            anchors.right: parent.right
                            anchors.rightMargin: 0
                            anchors.left: parent.left
                            anchors.leftMargin: 0
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 0
                            anchors.top: parent.top
                            anchors.topMargin: 0
                        }

                        Rectangle {
                            id: assetServerButtonGradient
                            gradient: Gradient {
                                GradientStop {
                                    position: 0
                                    color: "#383838"
                                }

                                GradientStop {
                                    position: 1
                                    color: "black"
                                }
                            }
                            opacity: 1
                            radius: 6
                            anchors.right: parent.right
                            anchors.rightMargin: 0
                            anchors.left: parent.left
                            anchors.leftMargin: 0
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 0
                            anchors.top: parent.top
                            anchors.topMargin: 0
                        }

                        Text {
                            color: "#ffffff"
                            text: "OPEN THIS DOMAIN'S ASSET SERVER"
                            font.bold: true
                            font.pixelSize: 14
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            enabled: true
                            onClicked: {
                                editRoot.sendToScript({
                                    method: "newEntityButtonClicked", params: { buttonName: "openAssetBrowserButton" }
                                });
                            }
                            onEntered: {
                                assetServerButton.state = "hover state";
                            }
                            onExited: {
                                assetServerButton.state = "base state";
                            }
                        }

                        states: [
                            State {
                                name: "hover state"

                                PropertyChanges {
                                    target: assetServerButtonGradient
                                    opacity: 0
                                }
                            },
                            State {
                                name: "base state"

                                PropertyChanges {
                                    target: assetServerButtonGradient
                                    opacity: 1
                                }
                            }
                        ]
                    }
                }
            }

            Tab {
                title: "LIST"
                active: true
                enabled: true
                property string originalUrl: ""

                WebView {
                    id: entityListToolWebView
                    url: "../../../../../scripts/system/html/entityList.html"
                    eventBridge: editRoot.eventBridge
                    anchors.fill: parent
                    enabled: true
                }
            }

            Tab {
                title: "PROPERTIES"
                active: true
                enabled: true
                property string originalUrl: ""

                WebView {
                    id: entityPropertiesWebView
                    url: "../../../../../scripts/system/html/entityProperties.html"
                    eventBridge: editRoot.eventBridge
                    anchors.fill: parent
                    enabled: true
                }
            }

            Tab {
                title: "GRID"
                active: true
                enabled: true
                property string originalUrl: ""

                WebView {
                    id: gridControlsWebView
                    url: "../../../../../scripts/system/html/gridControls.html"
                    eventBridge: editRoot.eventBridge
                    anchors.fill: parent
                    enabled: true
                }
            }

            Tab {
                title: "P"
                active: true
                enabled: true
                property string originalUrl: ""

                WebView {
                    id: particleExplorerWebView
                    url: "../../../../../scripts/system/particle_explorer/particleExplorer.html"
                    eventBridge: editRoot.eventBridge
                    anchors.fill: parent
                    enabled: true
                }
            }


            style: TabViewStyle {
                frameOverlap: 1
                tab: Rectangle {
                    color: styleData.selected ? "#404040" :"black"
                    implicitWidth: text.width + 42
                    implicitHeight: 40
                    Text {
                        id: text
                        anchors.centerIn: parent
                        text: styleData.title
                        font.pixelSize: 16
                        font.bold: true
                        color: styleData.selected ? "white" : "white"
                        property string glyphtext: ""
                        HiFiGlyphs {
                            anchors.centerIn: parent
                            size: 30
                            color: "#ffffff"
                            text: text.glyphtext
                        }
                        Component.onCompleted: if (styleData.title == "P") {
                            text.text = "   ";
                            text.glyphtext = "\ue004";
                        }
                    }
                }
                tabBar: Rectangle {
                    color: "black"
                    anchors.right: parent.right
                    anchors.rightMargin: 0
                    anchors.left: parent.left
                    anchors.leftMargin: 0
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 0
                    anchors.top: parent.top
                    anchors.topMargin: 0
                }
            }
        }
    }
}
